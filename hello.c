#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_launch.h>
#include <rte_common.h>


static int lcore_hello(void* arg){
    unsigned int lcore_id = rte_lcore_id();

    printf("Hello from lcore %u\n", lcore_id);
    return 0;
}


int main(int argc, char** argv){

    int ret;
    unsigned int lcore_id;

    //Intializing EAL
    ret = rte_eal_init(argc, argv);

    printf("Hello world code started\n");

    RTE_LCORE_FOREACH_WORKER(lcore_id){
        rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
    }

    lcore_hello(NULL);

    /* Wait for all lcores to finish */
    rte_eal_mp_wait_lcore();


    printf("DPDK Hello World finished\n");

    return 0; 
}