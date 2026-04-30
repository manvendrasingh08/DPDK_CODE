    #include <rte_ring.h>
    #include <rte_mbuf.h>

    #include <rte_ether.h>
    #include <rte_ip.h>
    #include <rte_tcp.h>
    #include <rte_udp.h>
    #include <rte_icmp.h>

    #include "common.h"


    void __parser(struct rte_mbuf* mbuf){

        uint8_t* data = rte_pktmbuf_mtod(mbuf, uint8_t*);
        uint16_t len = rte_pktmbuf_data_len(mbuf);

        struct rte_ether_hdr* eth = ( struct rte_ether_hdr*)data;
        printf("Ethernet : \n");
        printf("Src MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                            eth->src_addr.addr_bytes[0],
                            eth->src_addr.addr_bytes[1],
                            eth->src_addr.addr_bytes[2],
                            eth->src_addr.addr_bytes[3],
                            eth->src_addr.addr_bytes[4],
                            eth->src_addr.addr_bytes[5]
        );

        printf("Dst MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                            eth->dst_addr.addr_bytes[0],
                            eth->dst_addr.addr_bytes[1],
                            eth->dst_addr.addr_bytes[2],
                            eth->dst_addr.addr_bytes[3],
                            eth->dst_addr.addr_bytes[4],
                            eth->dst_addr.addr_bytes[5]);

            uint16_t ethType = rte_be_to_cpu_16(eth->ether_type);

            data += sizeof(struct rte_ether_hdr);

            //IPV4
            if(ethType == RTE_ETHER_TYPE_IPV4){

                struct rte_ipv4_hdr* ip =  (struct rte_ipv4_hdr*)data;
                uint8_t ihl = (ip->version_ihl & 0x0F) * 4; 
    //in dpdk the version & ihl are stored in version_ihl feild so to get ihl we do & with 0x0f to get ihl

                uint32_t src_ip = rte_be_to_cpu_32(ip->src_addr);
                uint32_t dst_ip = rte_be_to_cpu_32(ip->dst_addr);

                printf("IPv4:\n");
                printf("  Src IP: %u.%u.%u.%u\n",
                    (src_ip >> 24) & 0xff,
                    (src_ip >> 16) & 0xff,
                    (src_ip >> 8) & 0xff,
                    src_ip & 0xff);

                printf("  Dst IP: %u.%u.%u.%u\n",
                    (dst_ip >> 24) & 0xff,
                    (dst_ip >> 16) & 0xff,
                    (dst_ip >> 8) & 0xff,
                    dst_ip & 0xff);

                data += ihl;
                len  -= ihl;

                        /* -------- Transport -------- */
                if (ip->next_proto_id == IPPROTO_TCP) {
                    struct rte_tcp_hdr *tcp =
                        (struct rte_tcp_hdr *)data;

                    printf("TCP:\n");
                    printf("  Src Port: %u\n",
                        rte_be_to_cpu_16(tcp->src_port));
                    printf("  Dst Port: %u\n",
                        rte_be_to_cpu_16(tcp->dst_port));

                } else if (ip->next_proto_id == IPPROTO_UDP) {
                    struct rte_udp_hdr *udp =
                        (struct rte_udp_hdr *)data;

                    printf("UDP:\n");
                    printf("  Src Port: %u\n",
                        rte_be_to_cpu_16(udp->src_port));
                    printf("  Dst Port: %u\n",
                        rte_be_to_cpu_16(udp->dst_port));

                } else if (ip->next_proto_id == IPPROTO_ICMP) {
                    printf("ICMP packet\n");
                } else {
                    printf("Other IPv4 protocol: %u\n",
                        ip->next_proto_id);
                }

            }        

            //IPV6
        else if(ethType == RTE_ETHER_TYPE_IPV6){
                
            struct rte_ipv6_hdr *ip6 =
                (struct rte_ipv6_hdr *)data;

            printf("IPv6:\n");

    printf("  Src IP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:"
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
        ip6->src_addr.a[0],  ip6->src_addr.a[1],
        ip6->src_addr.a[2],  ip6->src_addr.a[3],
        ip6->src_addr.a[4],  ip6->src_addr.a[5],
        ip6->src_addr.a[6],  ip6->src_addr.a[7],
        ip6->src_addr.a[8],  ip6->src_addr.a[9],
        ip6->src_addr.a[10], ip6->src_addr.a[11],
        ip6->src_addr.a[12], ip6->src_addr.a[13],
        ip6->src_addr.a[14], ip6->src_addr.a[15]);

    printf("  Dst IP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:"
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
        ip6->dst_addr.a[0],  ip6->dst_addr.a[1],
        ip6->dst_addr.a[2],  ip6->dst_addr.a[3],
        ip6->dst_addr.a[4],  ip6->dst_addr.a[5],
        ip6->dst_addr.a[6],  ip6->dst_addr.a[7],
        ip6->dst_addr.a[8],  ip6->dst_addr.a[9],
        ip6->dst_addr.a[10], ip6->dst_addr.a[11],
        ip6->dst_addr.a[12], ip6->dst_addr.a[13],
        ip6->dst_addr.a[14], ip6->dst_addr.a[15]);

            data += sizeof(struct rte_ipv6_hdr);
            len  -= sizeof(struct rte_ipv6_hdr);

                        /* -------- Transport -------- */
            if (ip6->proto == IPPROTO_TCP) {
                struct rte_tcp_hdr *tcp =
                    (struct rte_tcp_hdr *)data;

                printf("TCP:\n");
                printf("  Src Port: %u\n",
                    rte_be_to_cpu_16(tcp->src_port));
                printf("  Dst Port: %u\n",
                    rte_be_to_cpu_16(tcp->dst_port));

            } else if (ip6->proto == IPPROTO_UDP) {
                struct rte_udp_hdr *udp =
                    (struct rte_udp_hdr *)data;

                printf("UDP:\n");
                printf("  Src Port: %u\n",
                    rte_be_to_cpu_16(udp->src_port));
                printf("  Dst Port: %u\n",
                    rte_be_to_cpu_16(udp->dst_port));

            } else if (ip6->proto == IPPROTO_ICMPV6) {
                printf("ICMPv6 packet\n");
            } else {
                printf("Other IPv6 protocol: %u\n",
                    ip6->proto);
            }
            }

            else {
            printf("Unknown EtherType: 0x%04x\n", ethType);
        }
    }

    int parser(void *args)
{
    printf("Parser running on lcore %u\n", rte_lcore_id());
    fflush(stdout);

    struct rte_mbuf *buff[BURST_SIZE];

    while (1) {
        unsigned dq =
            rte_ring_dequeue_burst(rx_ring, (void **)buff, BURST_SIZE, NULL);

        if (dq == 0)
            continue;

        for (unsigned i = 0; i < dq; i++) {
            __parser(buff[i]);
        }

         //at this moment the decision will be made in real __parser that it should be frwded or not
            // The case we are seeing here is that the packets are frwded and will be enq in tx_ring

            unsigned enq = rte_ring_enqueue_burst(
                tx_ring, (void **)buff, dq, NULL);

            if (enq < dq) {
                for (unsigned i = enq; i < dq; i++) {
                    rte_pktmbuf_free(buff[i]);
                }
            }      
    }

    return 0;
}
