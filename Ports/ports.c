#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_common.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>
#include <rte_errno.h>
#include <rte_mempool.h>
#include <stdio.h>
#include <stdint.h>

#define BURST_SIZE 32


int main(int argc, char** argv){

    int ret = rte_eal_init(argc, argv);
    if(ret < 0){
        rte_exit(EXIT_FAILURE, "eal init failed");
    }

    struct rte_mempool* mbuf_pool;
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",2048, 250, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    //available ports
    uint16_t nb_port = rte_eth_dev_count_avail();
    printf("Available ports are %u\n", nb_port);
    
    if(nb_port == 0){
        return 1;
    }
    uint16_t port_id = 0;
     
    //port configuration
    struct rte_eth_conf port_conf = {0};
    rte_eth_dev_configure(port_id, 1, 1, &port_conf);

    //setup rx and tx queues
    rte_eth_rx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool);

    rte_eth_tx_queue_setup(port_id, 0 , 1024, rte_eth_dev_socket_id(port_id),NULL);

    //starting the port
    rte_eth_dev_start(port_id);
    printf("Port %u started\n", port_id);

    rte_eth_promiscuous_enable(port_id);



    struct rte_mbuf* buff[BURST_SIZE];

    while(1){

           uint16_t nb_rx =  rte_eth_rx_burst(port_id, 0, buff, BURST_SIZE);

           if(nb_rx == 0){
            continue;
           }

           printf("Number of pkts rcvd is %u\n", nb_rx);

           //prijnthing raw packets content
           for(int i = 0 ; i< nb_rx; i++){
            struct rte_mbuf* curr_mbuf = buff[i];
            uint8_t* data = rte_pktmbuf_mtod(curr_mbuf, uint8_t*);
            uint16_t data_len = rte_pktmbuf_data_len(curr_mbuf);

            printf("Packet %d: len=%u : ", i, data_len);
            for (int j = 0; j < data_len; j++)
                printf("%02x ", data[j]);
            printf("\n");
           }

           /* Echo packets back (send RX → TX) */
        uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, buff, nb_rx);

        /* Free unsent packets */
        if (nb_tx < nb_rx) {
            for (int i = nb_tx; i < nb_rx; i++)
                rte_pktmbuf_free(buff[i]);
        }
      }    
    
    return 0;
}