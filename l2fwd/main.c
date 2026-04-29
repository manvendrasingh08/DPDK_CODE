#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS    8192
#define MBUF_CACHE   256
#define BURST_SIZE   32

static volatile int force_quit = 0;
static struct rte_mempool *mbuf_pool;

/* Simple port init */
static int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
    struct rte_eth_conf port_conf = {0};
    struct rte_eth_rxconf rxconf;
    struct rte_eth_txconf txconf;
    int retval;

    retval = rte_eth_dev_configure(port, 1, 1, &port_conf);
    if (retval < 0)
        return retval;

    rxconf = (struct rte_eth_rxconf){
        .offloads = 0,
    };

    retval = rte_eth_rx_queue_setup(
        port, 0, RX_RING_SIZE,
        rte_eth_dev_socket_id(port),
        &rxconf, mbuf_pool);
    if (retval < 0)
        return retval;

    txconf = (struct rte_eth_txconf){
        .offloads = 0,
    };

    retval = rte_eth_tx_queue_setup(
        port, 0, TX_RING_SIZE,
        rte_eth_dev_socket_id(port),
        &txconf);
    if (retval < 0)
        return retval;

    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

    rte_eth_promiscuous_enable(port);

    struct rte_ether_addr mac;
    rte_eth_macaddr_get(port, &mac);
    printf("Port %u MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           port,
           mac.addr_bytes[0], mac.addr_bytes[1],
           mac.addr_bytes[2], mac.addr_bytes[3],
           mac.addr_bytes[4], mac.addr_bytes[5]);

    return 0;
}

/* Main forwarding loop */
static int
l2fwd_main_loop(__rte_unused void *arg)
{
    uint16_t port;
    struct rte_mbuf *bufs[BURST_SIZE];

    printf("Entering main forwarding loop on lcore %u\n",
           rte_lcore_id());

    while (!force_quit) {
        RTE_ETH_FOREACH_DEV(port) {

            uint16_t nb_rx =
                rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);

            if (nb_rx == 0)
                continue;

            uint16_t dst_port = (port == 0) ? 1 : 0;

            uint16_t nb_tx =
                rte_eth_tx_burst(dst_port, 0, bufs, nb_rx);

            if (nb_tx < nb_rx) {
                for (uint16_t i = nb_tx; i < nb_rx; i++)
                    rte_pktmbuf_free(bufs[i]);
            }

            printf("RX port %u: %u packets → TX port %u: %u packets\n",
                   port, nb_rx, dst_port, nb_tx);
        }
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    int ret;
    uint16_t nb_ports;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    nb_ports = rte_eth_dev_count_avail();
    if (nb_ports != 2)
        rte_exit(EXIT_FAILURE,
                 "This app requires exactly 2 ports (found %u)\n",
                 nb_ports);

    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NUM_MBUFS * nb_ports,
        MBUF_CACHE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    for (uint16_t portid = 0; portid < nb_ports; portid++) {
        if (port_init(portid, mbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %u\n", portid);
    }

    rte_eal_mp_remote_launch(l2fwd_main_loop, NULL, CALL_MAIN);
    rte_eal_mp_wait_lcore();

    rte_eal_cleanup();
    return 0;
}
        