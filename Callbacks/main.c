#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_common.h>
#include <rte_mbuf.h>

#include <stdio.h>
#include <stdint.h>

#define NUM_MBUF 8192
#define CACHE_SIZE 256
int port_id = 0;

int main(int argc, char** argv){

    int ret = rte_eal_init(argc, argv);
    if(ret < 0){
        rte_exit(EXIT_FAILURE, "eal init failed");
    }

    struct rte_mempool* mem_pool;
    mem_pool = rte_pktmbuf_pool_create("MemPool",NUM_MBUF, CACHE_SIZE, 0,
                                        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    
    if(!mem_pool){
        rte_exit(EXIT_FAILURE, "mempool creatrion failed");
    }

    printf("mem pool creation successfull\n");

    int avail_port = rte_eth_dev_count_avail();
    if(avail_port == 0){
        rte_exit(EXIT_FAILURE, "no ports availale");
    }

    printf("available ports are %d\n", avail_port);
    
    port_init(port_id, mem_pool);

    
    // ****registering the callbacks

    int ret = rte_eth_add_rx_callback(port_id,0, rx_callback_fn, NULL );

    
    return 0;
}