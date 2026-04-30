    #include <rte_eal.h>
    #include <rte_ethdev.h>
    #include <rte_common.h>
    #include <rte_mbuf.h>
    #include <rte_lcore.h>
    #include <rte_errno.h>
    #include <rte_mempool.h>
    #include <rte_reorder.h>
    #include <stdio.h>
    #include <stdint.h>

    extern void packet_parsing(struct rte_mbuf *mbuf);

    #define BURST_SIZE 32

    int main(int argc, char** argv){

        int ret = rte_eal_init(argc, argv);
        if(ret<0){
            rte_exit(EXIT_FAILURE, "eal init failed\n");
        }

        struct rte_mempool* mbuf_pool;
        mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", 8192, 250, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

        uint16_t nb_port = rte_eth_dev_count_avail();
        printf("Available ports : %u\n", nb_port);

        u_int16_t port_id = 0;
        struct rte_eth_conf port_conf = {0};
        rte_eth_dev_configure(port_id, 1 ,1, &port_conf);

        rte_eth_rx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id),NULL, mbuf_pool );
        rte_eth_tx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL);

        rte_eth_dev_start(port_id);
        printf("Port %u started\n", port_id);

        rte_eth_promiscuous_enable(port_id);


        struct rte_mbuf* buff[BURST_SIZE];

        while(1){

            u_int16_t nb_rx = rte_eth_rx_burst(port_id, 0, buff,BURST_SIZE);
            
            if(nb_rx == 0){
                continue;
            }

            printf("Number of pkts rcvd is %u\n", nb_rx);

           for (uint16_t i = 0; i < nb_rx; i++) {
            packet_parsing(buff[i]);
            rte_pktmbuf_free(buff[i]);   // ✅ REQUIRED
             }

        }

    return 0;
    }