    #include <rte_ethdev.h>
    #include <rte_ring.h>
    #include <rte_mbuf.h>

    #include "common.h"

    void rx_handler(void* args){

        printf("RX handler running on lcore %u\n", rte_lcore_id());


        struct rte_mbuf* buff[BURST_SIZE];

        while(1){

            u_int16_t nb_rx = rte_eth_rx_burst(port_id, 0, buff, BURST_SIZE);
            
            if (nb_rx == 0)
                continue;
            
            unsigned int free_space;
            unsigned enq = rte_ring_enqueue_burst(rx_ring, (void**)buff, nb_rx, &free_space);
            printf("enqueued %u pkts\n", enq);
            printf("Free space in the ring : %u\n", free_space);

            if (enq < nb_rx) {
                    for (unsigned i = enq; i < nb_rx; i++)
                        rte_pktmbuf_free(buff[i]);
                }
        }
        

    }