#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_graph.h>
#include <rte_graph_worker.h>
#include <rte_malloc.h>

/* ===================== RX (SOURCE) NODE ===================== */

static uint16_t
rx_node_process(struct rte_graph *graph,
                struct rte_node *node,
                void **objs,
                uint16_t nb_objs)
{

    printf("In src node's process fn\n");

    // DPDK uses this macro which is equalent to (void)(x). This is done so that the compiler deos not give
    //unused parameter warnings.
    RTE_SET_USED(objs);       
    RTE_SET_USED(nb_objs);

    static int called;

    /* Run only once for demo */
    if (called)
        return 0;

    called = 1;

    /* Source nodes MUST create their own objects */
    void *pkts[4];
    for (int i = 0; i < 4; i++)
        pkts[i] = (void *)(uintptr_t)(i + 1);

    /* Send all packets to edge 0 → PRINT node */
    rte_node_enqueue(graph, node, 0, pkts, 4);

    /* Return value ignored for source nodes */
    return 0;
}

static struct rte_node_register rx_node = {
    .name = "RX",
    .flags = RTE_NODE_SOURCE_F,   /* REQUIRED so that dpdk knows that there is no input node for this node*/ 
    .process = rx_node_process,
    .nb_edges = 1,
    .next_nodes = {
        "PRINT",
    },
};



/* Manual registration (equivalent to RTE_NODE_REGISTER) */
static void __attribute__((constructor))
rx_node_constructor(void)
{
    __rte_node_register(&rx_node);
}

/* ===================== PRINT NODE ===================== */

static uint16_t
print_node_process(struct rte_graph *graph,
                   struct rte_node *node,
                   void **objs,
                   uint16_t nb_objs)
{
    RTE_SET_USED(graph);
    RTE_SET_USED(node);

    printf("PRINT node received %u packets:\n", nb_objs);

    for (uint16_t i = 0; i < nb_objs; i++)
        printf("  packet id = %lu\n", (uintptr_t)objs[i]);

    return 0;
}

static struct rte_node_register print_node = {
    .name = "PRINT",
    .process = print_node_process,
    .nb_edges = 0,
};

static void __attribute__((constructor))
print_node_constructor(void)
{
    __rte_node_register(&print_node);
}

/* ===================== MAIN ===================== */

int
main(int argc, char **argv)
{
    struct rte_graph_param graph_prm = {0};
    struct rte_graph *graph;
    rte_graph_t graph_id;

    rte_eal_init(argc, argv);

    /* Select which nodes belong to this graph */
    graph_prm.nb_node_patterns = 1;
    graph_prm.node_patterns = (const char *[]) {
        "rx_node"
    };

    /* Create graph template */
    graph_id = rte_graph_create("demo_graph", &graph_prm);
    if (graph_id == RTE_GRAPH_ID_INVALID) {
        printf("Graph creation failed\n");
        return -1;
    }



    /* Create per-lcore graph instance */
    graph = rte_graph_lookup("demo_graph");
    if (!graph) {
        printf("Graph lookup failed\n");
        return -1;
    }

    printf("Graph created, starting walk\n");

    /* Walk graph */
    rte_graph_walk(graph);

    return 0;
}
