#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_common.h>
#include <stdio.h>

#define OBJ_COUNT 1024
#define OBJ_SIZE  128
#define CACHE_SIZE 64

int main(int argc, char **argv)
{
    rte_eal_init(argc, argv);

    struct rte_mempool *pool;

    pool = rte_mempool_create(
        "MY_CUSTOM_POOL",
        OBJ_COUNT,
        OBJ_SIZE,
        CACHE_SIZE,
        0,
        NULL, NULL,
        NULL, NULL,
        rte_socket_id(),
        0
    );

    if (!pool)
        rte_exit(EXIT_FAILURE, "Cannot create mempool\n");

    if (rte_mempool_set_ops_byname(pool, "ring_mp_mc", NULL) < 0)
        rte_exit(EXIT_FAILURE, "Cannot set memops\n");

    /* ✅ Correct way to print ops name */
    const struct rte_mempool_ops *ops = rte_mempool_get_ops(pool);
    printf("Custom mempool created with memops: %s\n", ops->name);

    return 0;
}
