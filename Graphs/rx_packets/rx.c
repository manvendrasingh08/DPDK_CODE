#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_graph.h>
#include <rte_graph_worker.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>

#define RX_BURST_SIZE 32
#define NB_MBUFS 8192
#define MBUF_CACHE_SIZE 250

/* ================= RX NODE ================= */

static uint16_t
rx_node_process(struct rte_graph *graph,
                struct rte_node *node,
                void **objs,
                uint16_t nb_objs)
{
    static bool rx_done;
    struct rte_mbuf *pkts[RX_BURST_SIZE];
    uint16_t nb_rx;

    RTE_SET_USED(nb_objs);

    if (rx_done)
        return 0;

    nb_rx = rte_eth_rx_burst(0, 0, pkts, RX_BURST_SIZE);
    if (nb_rx == 0)
        return 0;

    for (uint16_t i = 0; i < nb_rx; i++)
        objs[i] = pkts[i];

    rte_node_enqueue(graph, node, 0, objs, nb_rx);

    rx_done = true;
    return nb_rx;
}

static struct rte_node_register rx_node = {
    .name = "RX",
    .flags = RTE_NODE_SOURCE_F,
    .process = rx_node_process,
    .nb_edges = 1,
    .next_nodes = {
        "CLASSIFY",
    },
};

static void __attribute__((constructor))
rx_node_ctor(void)
{
    __rte_node_register(&rx_node);
}

/* ================= CLASSIFY NODE ================= */

static uint16_t
classify_node_process(struct rte_graph *graph,
                      struct rte_node *node,
                      void **objs,
                      uint16_t nb_objs)
{
    struct rte_mbuf *m;
    struct rte_ether_hdr *eth;
    uint16_t ipv4_count = 0, ipv6_count = 0;
    void *ipv4_objs[RX_BURST_SIZE];
    void *ipv6_objs[RX_BURST_SIZE];

    for (uint16_t i = 0; i < nb_objs; i++) {
        m = objs[i];
        eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

        if (eth->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4))
            ipv4_objs[ipv4_count++] = m;
        else if (eth->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV6))
            ipv6_objs[ipv6_count++] = m;
        else
            rte_pktmbuf_free(m);
    }

    if (ipv4_count)
        rte_node_enqueue(graph, node, 0, ipv4_objs, ipv4_count);

    if (ipv6_count)
        rte_node_enqueue(graph, node, 1, ipv6_objs, ipv6_count);

    return nb_objs;
}

static struct rte_node_register classify_node = {
    .name = "CLASSIFY",
    .process = classify_node_process,
    .nb_edges = 2,
    .next_nodes = {
        "PRINT_IPV4",
        "PRINT_IPV6",
    },
};

static void __attribute__((constructor))
classify_node_ctor(void)
{
    __rte_node_register(&classify_node);
}

/* ================= PRINT IPV4 ================= */

static uint16_t
print_ipv4_process(struct rte_graph *graph,
                   struct rte_node *node,
                   void **objs,
                   uint16_t nb_objs)
{
    RTE_SET_USED(graph);
    RTE_SET_USED(node);

    printf("IPv4 packets: %u\n", nb_objs);

    for (uint16_t i = 0; i < nb_objs; i++) {
        struct rte_mbuf *m = objs[i];
        struct rte_ipv4_hdr *ip =
            rte_pktmbuf_mtod_offset(m, struct rte_ipv4_hdr *,
                sizeof(struct rte_ether_hdr));

        printf("  SRC=%08x DST=%08x TTL=%u\n",
               rte_be_to_cpu_32(ip->src_addr),
               rte_be_to_cpu_32(ip->dst_addr),
               ip->time_to_live);

        rte_pktmbuf_free(m);
    }

    return 0;
}

static struct rte_node_register print_ipv4_node = {
    .name = "PRINT_IPV4",
    .process = print_ipv4_process,
    .nb_edges = 0,
};

static void __attribute__((constructor))
print_ipv4_ctor(void)
{
    __rte_node_register(&print_ipv4_node);
}

/* ================= PRINT IPV6 ================= */

static uint16_t
print_ipv6_process(struct rte_graph *graph,
                   struct rte_node *node,
                   void **objs,
                   uint16_t nb_objs)
{
    RTE_SET_USED(graph);
    RTE_SET_USED(node);

    printf("IPv6 packets: %u\n", nb_objs);

    for (uint16_t i = 0; i < nb_objs; i++) {
        struct rte_mbuf *m = objs[i];
        struct rte_ipv6_hdr *ip6 =
            rte_pktmbuf_mtod_offset(m, struct rte_ipv6_hdr *,
                sizeof(struct rte_ether_hdr));

        printf("  HopLimit=%u\n", ip6->hop_limits);
        rte_pktmbuf_free(m);
    }

    return 0;
}

static struct rte_node_register print_ipv6_node = {
    .name = "PRINT_IPV6",
    .process = print_ipv6_process,
    .nb_edges = 0,
};

static void __attribute__((constructor))
print_ipv6_ctor(void)
{
    __rte_node_register(&print_ipv6_node);
}

/* ================= MAIN ================= */

int main(int argc, char **argv)
{
    struct rte_graph_param graph_param = {0};
    struct rte_graph *graph;
    rte_graph_t graph_id;
    struct rte_mempool *mp;

    rte_eal_init(argc, argv);

    mp = rte_pktmbuf_pool_create("MBUF_POOL",
                                 NB_MBUFS,
                                 MBUF_CACHE_SIZE,
                                 0,
                                 RTE_MBUF_DEFAULT_BUF_SIZE,
                                 rte_socket_id());

    struct rte_eth_conf conf = {0};
    rte_eth_dev_configure(0, 1, 1, &conf);
    rte_eth_rx_queue_setup(0, 0, 128,
                            rte_eth_dev_socket_id(0),
                            NULL, mp);
    rte_eth_dev_start(0);

    graph_param.nb_node_patterns = 1;
    graph_param.node_patterns = (const char *[]){
        "RX",
    };

    graph_id = rte_graph_create("ipv4_ipv6_graph", &graph_param);
    graph = rte_graph_lookup("ipv4_ipv6_graph");

    rte_graph_dump(stdout, graph);

    rte_graph_walk(graph);

    return 0;
}
