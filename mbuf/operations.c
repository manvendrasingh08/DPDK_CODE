#include <stdio.h>
#include <stdint.h>

#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>

#define NUM_MBUFS 1024
#define CACHE_SIZE 256
#define BUF_SIZE RTE_MBUF_DEFAULT_BUF_SIZE

static void dump_mbuf(const char *stage, struct rte_mbuf *m)
{
    uint8_t *buf  = (uint8_t *)m->buf_addr;
    uint8_t *data = rte_pktmbuf_mtod(m, uint8_t *);

    printf("\n=== %s ===\n", stage);
    printf("buf_addr   = %p\n", buf);
    printf("data_ptr   = %p\n", data);
    printf("data_off   = %u\n", m->data_off);
    printf("data_len   = %u\n", m->data_len);
    printf("pkt_len    = %u\n", m->pkt_len);
    printf("headroom   = %u\n", rte_pktmbuf_headroom(m));
    printf("tailroom   = %u\n", rte_pktmbuf_tailroom(m));
}

int main(int argc, char **argv)
{
    struct rte_mempool *pool;
    struct rte_mbuf *m;

    if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    pool = rte_pktmbuf_pool_create("MBUF_POOL",
                                   NUM_MBUFS,
                                   CACHE_SIZE,
                                   0,
                                   BUF_SIZE,
                                   rte_socket_id());

    if (!pool)
        rte_exit(EXIT_FAILURE, "mempool create failed\n");

    m = rte_pktmbuf_alloc(pool);
    if (!m)
        rte_exit(EXIT_FAILURE, "mbuf alloc failed\n");

    /*  Fresh mbuf */
    dump_mbuf("After allocation", m);

    /*  Append 100 bytes (payload) */
    uint8_t *p = rte_pktmbuf_append(m, 100);
    if (!p)
        rte_exit(EXIT_FAILURE, "append failed\n");

    for (int i = 0; i < 100; i++)
        p[i] = i;

    dump_mbuf("After append 100 bytes", m);

    /* Trim 40 bytes from end */
    if (rte_pktmbuf_trim(m, 40) < 0)
        rte_exit(EXIT_FAILURE, "trim failed\n");

    dump_mbuf("After trim 40 bytes", m);

    /*  Prepend 20 bytes (header) */
    uint8_t *h = rte_pktmbuf_prepend(m, 20);
    if (!h)
        rte_exit(EXIT_FAILURE, "prepend failed\n");

    for (int i = 0; i < 20; i++)
        h[i] = 0xAA;

    dump_mbuf("After prepend 20 bytes", m);

    /*  Remove 10 bytes from start */
    if (!rte_pktmbuf_adj(m, 10))
        rte_exit(EXIT_FAILURE, "adj failed\n");

    dump_mbuf("After adj 10 bytes", m);

    rte_pktmbuf_free(m);

    printf("\n Demo completed successfully\n");
    return 0;
}
