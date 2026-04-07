#include <rte_eal.h>
#include <rte_ethdev.h>
#include <stdio.h>
#include <stdint.h>

int main(int argc, char** argv){

    int ret = rte_eal_init(argc, argv);
    if(ret< 0){
        rte_exit(EXIT_FAILURE, "rte_eal init failed\n");
    }

    //No of ports available
    uint16_t nb_ports = rte_eth_dev_count_avail();
    printf("There are %u ports available\n", nb_ports);
    return 0;
}
