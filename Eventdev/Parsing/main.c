

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_eventdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS    4096
#define MBUF_CACHE   250
#define BURST_SIZE   32

#define RX_Q 0
#define TX_Q 1

static uint8_t evdev_id;
static uint16_t eth_port_id = 0;
static struct rte_mempool *mbuf_pool;

/* worker lcore → event port */
static uint8_t lcore_to_evport[RTE_MAX_LCORE];
static unsigned rx_lcore;

/* ---------------- ETHDEV port INIT ---------------- */

static void
port_init(void)
{
    struct rte_eth_conf port_conf = {0};

    if (rte_eth_dev_count_avail() == 0)
        rte_exit(EXIT_FAILURE, "No ethdev found\n");

    if (rte_eth_dev_configure(eth_port_id, 1, 1, &port_conf) < 0)
        rte_exit(EXIT_FAILURE, "eth configure failed\n");

    if (rte_eth_rx_queue_setup(
            eth_port_id, 0, RX_RING_SIZE,
            rte_eth_dev_socket_id(eth_port_id),
            NULL, mbuf_pool) < 0)
        rte_exit(EXIT_FAILURE, "rx queue setup failed\n");

    if (rte_eth_tx_queue_setup(
            eth_port_id, 0, TX_RING_SIZE,
            rte_eth_dev_socket_id(eth_port_id),
            NULL) < 0)
        rte_exit(EXIT_FAILURE, "tx queue setup failed\n");

    if (rte_eth_dev_start(eth_port_id) < 0)
        rte_exit(EXIT_FAILURE, "eth start failed\n");
}

/* ---------------- EVENTDEV INIT ---------------- */

static void
eventdev_init(uint8_t nb_workers)
{
    evdev_id = rte_event_dev_get_dev_id("event_sw0");
    if (evdev_id < 0)
        rte_exit(EXIT_FAILURE, "event_sw0 not found\n");

    struct rte_event_dev_config dev_conf = {
        .nb_event_queues = 2,
        .nb_event_ports  = nb_workers,
        .nb_events_limit = 4096,
        .nb_event_queue_flows = 1024,
        .nb_event_port_dequeue_depth = 32,
        .nb_event_port_enqueue_depth = 32,
    };

    if (rte_event_dev_configure(evdev_id, &dev_conf) < 0)
        rte_exit(EXIT_FAILURE, "eventdev configure failed\n");

    struct rte_event_queue_conf qconf = {
        .nb_atomic_flows = 1024,
        .nb_atomic_order_sequences = 1024,
        .schedule_type = RTE_SCHED_TYPE_PARALLEL,
        .priority = RTE_EVENT_DEV_PRIORITY_NORMAL,
    };

    if (rte_event_queue_setup(evdev_id, RX_Q, &qconf) < 0)
        rte_exit(EXIT_FAILURE, "RX_Q setup failed\n");

    if (rte_event_queue_setup(evdev_id, TX_Q, &qconf) < 0)
        rte_exit(EXIT_FAILURE, "TX_Q setup failed\n");

    struct rte_event_port_conf pconf = {
        .new_event_threshold = 4096,
        .dequeue_depth = 32,
        .enqueue_depth = 32,
    };

    uint8_t ev_port = 0;
    unsigned lcore;

    RTE_LCORE_FOREACH_WORKER(lcore) {
        

        if (rte_event_port_setup(evdev_id, ev_port, &pconf) < 0)
            rte_exit(EXIT_FAILURE, "event port setup failed\n");

        uint8_t queues[] = { RX_Q, TX_Q };
        uint8_t prios[]  = {
            RTE_EVENT_DEV_PRIORITY_NORMAL,
            RTE_EVENT_DEV_PRIORITY_NORMAL
        };

        if (rte_event_port_link(evdev_id, ev_port,
                                queues, prios, 2) != 2)
            rte_exit(EXIT_FAILURE, "event port link failed\n");

        lcore_to_evport[lcore] = ev_port++;
    }

    if (rte_event_dev_start(evdev_id) < 0)
        rte_exit(EXIT_FAILURE, "eventdev start failed\n");

    printf("Eventdev %u started successfully\n", evdev_id);
}

/* ---------------- RX LOOP ---------------- */

static int
rx_loop(__rte_unused void *arg)
{
    struct rte_mbuf *pkts[BURST_SIZE];
    struct rte_event ev[BURST_SIZE];

    while (1) {
        uint16_t nb = rte_eth_rx_burst(
            eth_port_id, 0, pkts, BURST_SIZE);

        if (nb == 0)
            continue;

        for (int i = 0; i < nb; i++) {
            ev[i].event_ptr = pkts[i];
            ev[i].queue_id  = RX_Q;
            ev[i].sched_type = RTE_SCHED_TYPE_PARALLEL;
            ev[i].flow_id = 0;
        }

        uint16_t ret = rte_event_enqueue_burst(
            evdev_id, 0 /* unused for enqueue */, ev, nb);

        if (ret < nb) {
            for (uint16_t i = ret; i < nb; i++)
                rte_pktmbuf_free(pkts[i]);
        }
    }
}

/* ---------------- WORKER LOOP ---------------- */

static int
worker_loop(__rte_unused void *arg)
{
    struct rte_event ev;
    uint8_t ev_port = lcore_to_evport[rte_lcore_id()];

    while (1) {
        if (rte_event_dequeue_burst(
                evdev_id, ev_port, &ev, 1, 0) == 0)
            continue;

        struct rte_mbuf *m = ev.event_ptr;

        if (ev.queue_id == RX_Q) {
            ev.queue_id  = TX_Q;
            ev.sched_type = RTE_SCHED_TYPE_PARALLEL;

            rte_event_enqueue_burst(evdev_id, ev_port, &ev, 1);
        } else {
            rte_eth_tx_burst(eth_port_id, 0, &m, 1);
        }
    }
}

/* ---------------- MAIN ---------------- */

int
main(int argc, char **argv)
{
    rte_eal_init(argc, argv);

    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NUM_MBUFS,
        MBUF_CACHE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "mbuf pool create failed\n");

    rx_lcore = rte_get_next_lcore(-1, 1, 0);

    port_init();

    uint8_t nb_workers = rte_lcore_count() - 2;
    eventdev_init(nb_workers);

    unsigned lcore;

    RTE_LCORE_FOREACH_WORKER(lcore) {
        if (lcore == rx_lcore)
            rte_eal_remote_launch(rx_loop, NULL, lcore);
        else
            rte_eal_remote_launch(worker_loop, NULL, lcore);
    }

    rte_eal_mp_wait_lcore();
    return 0;
}
