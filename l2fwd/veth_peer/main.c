#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_ether.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS    8192
#define MBUF_CACHE   256
#define BURST_SIZE   32

static volatile int force_quit;
static struct rte_mempool *mbuf_pool;
static struct rte_hash *l2_hash;

/* Signal handler */
static void
signal_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
        force_quit = 1;
}

/* Port init */
static int
port_init(uint16_t port, struct rte_mempool *mbuf_pool)
{
    struct rte_eth_conf port_conf = {0};
    struct rte_eth_rxconf rxconf = {0};
    struct rte_eth_txconf txconf = {0};

    int ret = rte_eth_dev_configure(port, 1, 1, &port_conf);
    if (ret < 0)
        return ret;

    ret = rte_eth_rx_queue_setup(
        port, 0, RX_RING_SIZE,
        rte_eth_dev_socket_id(port),
        &rxconf, mbuf_pool);
    if (ret < 0)
        return ret;

    ret = rte_eth_tx_queue_setup(
        port, 0, TX_RING_SIZE,
        rte_eth_dev_socket_id(port),
        &txconf);
    if (ret < 0)
        return ret;

    ret = rte_eth_dev_start(port);
    if (ret < 0)
        return ret;

    rte_eth_promiscuous_enable(port);

    struct rte_ether_addr mac;
    rte_eth_macaddr_get(port, &mac);
    printf("Port %u MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
           port,
           mac.addr_bytes[0], mac.addr_bytes[1],
           mac.addr_bytes[2], mac.addr_bytes[3],
           mac.addr_bytes[4], mac.addr_bytes[5]);

    return 0;
}


/* L2 forwarding loop */
static int 
l2fwd_main_loop(__rte_unused void* arg){

    uint16_t port;
    struct rte_mbuf* buff[BURST_SIZE];

    printf("running l2fwd fn on core %d\n", rte_lcore_id());

    while(!force_quit){
        RTE_ETH_FOREACH_DEV(port){

            int nb_rx = rte_eth_rx_burst(port, 0, buff, BURST_SIZE);

             if (nb_rx == 0)
                continue;

            for(int i = 0; i < nb_rx; i++){
                
                struct rte_mbuf* m = buff[i];
                struct rte_ether_hdr *eth_hdr;

                eth_hdr = rte_pktmbuf_mtod(m, struct rte_ether_hdr*);

                struct rte_ether_addr src_mac = eth_hdr->src_addr;
                struct rte_ether_addr dst_mac = eth_hdr->dst_addr;

                //adding entry of src mac addr to the hash table (LEARNING)
                rte_hash_add_key_data(l2_hash, &src_mac, (void*)(uintptr_t)port);

                void* out_port_ptr;
                uint16_t out_port;
                
                //lookup
                if (rte_hash_lookup_data(
                        l2_hash,
                        &dst_mac,
                        &out_port_ptr) >= 0) {

                    out_port = (uint16_t)(uintptr_t)out_port_ptr;

                } else {
                    /* FLOOD */
                    out_port = (port == 0) ? 1 : 0;
                }
                /* Transmit */
                if (rte_eth_tx_burst(out_port, 0, &m, 1) < 1)
                    rte_pktmbuf_free(m);
            }
        }
    }
    return 0;
        
 }

 int
main(int argc, char *argv[])
{
    int ret;
    uint16_t nb_ports;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    nb_ports = rte_eth_dev_count_avail();
    if (nb_ports != 2)
        rte_exit(EXIT_FAILURE,
                 "Need exactly 2 ports, found %u\n", nb_ports);

    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",
        NUM_MBUFS * nb_ports,
        MBUF_CACHE,
        0,
        RTE_MBUF_DEFAULT_BUF_SIZE,
        rte_socket_id());

    if (!mbuf_pool)
        rte_exit(EXIT_FAILURE, "mbuf pool create failed\n");

    /* L2 HASH TABLE */
    struct rte_hash_parameters hash_params = {
        .name = "l2_mac_table",
        .entries = 1024,
        .key_len = RTE_ETHER_ADDR_LEN,
        .hash_func = rte_jhash,
        .hash_func_init_val = 0,
        .socket_id = rte_socket_id(),
    };

    l2_hash = rte_hash_create(&hash_params);
    if (!l2_hash)
        rte_exit(EXIT_FAILURE, "L2 hash create failed\n");

    for (uint16_t portid = 0; portid < nb_ports; portid++) {
        if (port_init(portid, mbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Port %u init failed\n", portid);
    }

    rte_eal_mp_remote_launch(l2fwd_main_loop, NULL, CALL_MAIN);
    rte_eal_mp_wait_lcore();

    rte_hash_free(l2_hash);
    rte_eal_cleanup();
    return 0;
}
