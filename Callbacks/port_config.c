


#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_ethdev.h>
void port_config(int port_id, struct rte_mempool* mem_pool){


    struct rte_eth_conf port_conf  = {0};

    rte_eth_dev_configure(port_id, 1, 1, &port_conf );

    rte_eth_rx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL, mem_pool);

    rte_eth_tx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL);

    rte_eth_dev_start(port_id);

    rte_eth_promiscuous_enable(port_id);

    

}