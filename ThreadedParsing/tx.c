#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>

#include "common.h"

int tx_handler(void* args){
    
    printf("RX handler running on lcore %u\n", rte_lcore_id());

    struct rte_mbuf* buff[BURST_SIZE];
    while(1){

    int num_dequeud = rte_ring_dequeue_burst(tx_ring,(void**)buff, BURST_SIZE, NULL );

    if(num_dequeud == 0)
        continue;

    int sent = rte_eth_tx_burst(port_id, 0, buff, num_dequeud);
    
    printf("Sent %d pkts to NIC\n", sent);
    //case where let say out of num_dequeud mbufs only sent mbufs were accepted by the nic which will
    //be later be freed bu the NIC in TX complpetion, so the rest is tpo be freed by us.
    
    if(sent<num_dequeud){
         for (uint16_t i = sent; i < num_dequeud; i++)
                rte_pktmbuf_free(buff[i]);
        }
    }

    return 0;
}