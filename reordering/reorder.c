#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_reorder.h>
#include <rte_mbuf_dyn.h>

#define RB_SIZE 16
#define NUM_PKTS 8

// static int seq_dynfield_offset;

// static const struct rte_mbuf_dynfield seq_field = {
//     .name  = "pkt_seq",
//     .size  = sizeof(uint32_t),
//     .align = __alignof__(uint32_t),
// };


#define GET_SEQ(m) \
    (*(uint32_t *)RTE_MBUF_DYNFIELD((m), rte_reorder_seqn_dynfield_offset, uint32_t *))

int main(int argc, char** argv){

    struct rte_mempool *mp;
    struct rte_reorder_buffer* rb;
    struct rte_mbuf* pkts[NUM_PKTS];
    struct rte_mbuf* out[NUM_PKTS];

    int ret =   rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

    mp = rte_pktmbuf_pool_create("MBUF_POOL",
                                 128,
                                 0,
                                 0,
                                 RTE_MBUF_DEFAULT_BUF_SIZE,
                                 rte_socket_id());

    if (!mp)
        rte_exit(EXIT_FAILURE, "mempool create failed\n");


    // regestering dynfield
    // seq_dynfield_offset = rte_mbuf_dynfield_register(&seq_field);
    //  if (seq_dynfield_offset < 0)
    //     rte_exit(EXIT_FAILURE, "dynfield register failed\n");
    
    // printf("dynfield registered at offset : %d\n", rte_reorder_seqn_dynfield_offset);

    //creating roerder buffer
    rb = rte_reorder_create("RoB", rte_socket_id(), RB_SIZE);

    printf("reorder buffer created\n");  

    //allocating pkts and assigning seq number
    for(int i = 0; i < NUM_PKTS; i++){
        
        pkts[i] = rte_pktmbuf_alloc(mp);
        GET_SEQ(pkts[i]) = i;

        printf("Allocated pkt %d  seq=%u\n", i, GET_SEQ(pkts[i]));
    }

    //inserting packets OUT OF ORDER
    int order[NUM_PKTS] = { 0, 2, 1, 4, 3, 6, 5, 7 };

    for(int i = 0; i< NUM_PKTS; i++){
        struct rte_mbuf* m = pkts[order[i]];

        //inserting

       int ret =  rte_reorder_insert(rb, m);
       if(ret<0)
        printf("insertion failed\n");
    
    printf("Inserted  pkt seq=%d\n", GET_SEQ(m));

    }

    //draining buffer
    int drain_count = rte_reorder_drain(rb, out, NUM_PKTS);
    for (int i = 0; i < drain_count; i++) {
        printf("OUT pkt seq=%u\n", GET_SEQ(out[i]));
        rte_pktmbuf_free(out[i]);
    }
}

