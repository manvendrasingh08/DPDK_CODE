// #include <stdio.h>
// #include <stdint.h>

// #include <rte_eal.h>
// #include <rte_ethdev.h>
// #include <rte_common.h>
// #include <rte_mbuf.h>
// #include <rte_lcore.h>
// #include <rte_errno.h>
// #include <rte_mempool.h>



// int main(int argc, char** argv){

//     int ret = rte_eal_init(argc, argv);
//     if(ret < 0)
//         rte_exit(EXIT_FAILURE, "EAL INIT FAILED");

//     struct rte_mempool* mempool;
//     mempool = rte_pktmbuf_pool_create("mempool",8192, 256, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

//     int nb_port = rte_eth_dev_count_avail();
//     printf("available ports : %d\n", nb_port);

//     int port_id = 0;
//     struct rte_eth_conf port_conf = {0};
//     rte_eth_dev_configure(port_id, 1, 1, &port_conf);

//     rte_eth_rx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id),NULL, mempool);
//     rte_eth_tx_queue_setup(port_id, 0 , 1024, rte_eth_dev_socket_id(port_id), NULL);

//     rte_eth_dev_start(port_id);
//     printf("Port %u started\n", port_id);

//     rte_eth_promiscuous_enable(port_id);


//     return 0;
// }