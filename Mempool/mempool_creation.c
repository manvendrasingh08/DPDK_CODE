#include <stdio.h>
#include <stdint.h>
#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#define NUM_MBUF 8192
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

int main(int argc, char** argv){

    int ret = rte_eal_init(argc, argv);
    if(ret < 0){
        rte_exit(EXIT_FAILURE, "EAL initialization failed");
    }

    struct rte_mempol* mbuf_pool;
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUF, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    
    printf("The defualt buf size : %d i.e 2048(2kb) + mbuf struct size(128 bytes)\n", RTE_MBUF_DEFAULT_BUF_SIZE);
    printf("MBUF pool created successfully\n");  

    return 0;   
}

