
#include <stdio.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_ring.h>
#include <rte_mempool.h>

#include "common.h"
#include "rx.h"
#include "parser.h"
#include "tx.h"

#define NUM_BUF 8192
#define CACHE_SIZE 256


struct rte_ring *rx_ring = NULL;
struct rte_ring *tx_ring = NULL;
uint16_t port_id = 0;

int main(int argc, char** argv){

    int ret = rte_eal_init(argc, argv);
    if(ret<0){
        rte_exit(EXIT_FAILURE, "eal init failed");
    }

    struct rte_mempool* mbuf_pool;
    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL",NUM_BUF, CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    uint16_t nb_port = rte_eth_dev_count_avail();
    printf("available ports: %u\n", nb_port);

    if(nb_port == 0){
        rte_exit(EXIT_FAILURE,"No port found\n");
    }

    struct rte_eth_conf port_conf = {0};
    rte_eth_dev_configure(port_id, 1, 1, &port_conf);

    rte_eth_rx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool);
    rte_eth_tx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL);

    rte_eth_dev_start(port_id);
    printf("Port %u started\n", port_id);

    rte_eth_promiscuous_enable(port_id);
    
    //ring creation
    rx_ring = rte_ring_create("Rx_ring", 1024, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);

    tx_ring = rte_ring_create("TX_ring", 1024, rte_socket_id(), RING_F_MP_HTS_ENQ|RING_F_SC_DEQ);
                        // MP in tx ring cox multiple parser lcores can enqueue in the ring.

    
    unsigned lcore_id;

    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        if (lcore_id == 1)
            rte_eal_remote_launch(rx_handler, NULL, lcore_id);
        else if (lcore_id == 2)
            rte_eal_remote_launch(parser, NULL, lcore_id);
        else if(lcore_id == 3) 
            rte_eal_remote_launch(tx_handler, NULL, lcore_id);
    }

    rte_eal_mp_wait_lcore();


    return 0;    
}