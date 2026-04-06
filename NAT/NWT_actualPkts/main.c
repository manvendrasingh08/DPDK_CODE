#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_malloc.h>
#include <rte_cycles.h>
#include <rte_timer.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_ether.h>

/* ---------------- CONFIG ---------------- */

#define PUBLIC_IP 0x0A000001

/* ---------------- KEY ---------------- */

struct nat_key {
    uint32_t src_ip;
    uint16_t src_port;
    uint32_t dst_ip;
    uint16_t dst_port;
};

/* ---------------- ENTRY ---------------- */

struct nat_entry {
    struct nat_key key;

    uint32_t private_ip;
    uint16_t private_port;

    uint32_t public_ip;
    uint16_t public_port;

    struct rte_timer timer;
};

/* ---------------- GLOBAL ---------------- */

struct rte_hash *nat_table;

uint16_t next_port = 10000;
uint64_t hz;

uint64_t global_last_pkt;
int table_cleared = 0;

/* ---------------- UTIL ---------------- */

void print_ip(uint32_t ip) {
    printf("%d.%d.%d.%d",
        (ip>>24)&255,(ip>>16)&255,(ip>>8)&255,ip&255);
}

/* ---------------- TIMER CALLBACK ---------------- */

void flow_timeout_cb(struct rte_timer *tim, void *arg) {

    struct nat_entry *e = arg;

    printf("\n❌ [TIMER EXPIRED] Removing flow: ");
    print_ip(e->private_ip);
    printf(":%d → ", e->private_port);
    print_ip(e->key.dst_ip);
    printf(":%d\n", e->key.dst_port);

    rte_hash_del_key(nat_table, &e->key);
    rte_free(e);
}

/* ---------------- RESET TIMER ---------------- */

void refresh_timer(struct nat_entry *e) {

    printf("🔄 [TIMER RESET] Flow refreshed\n");

    rte_timer_reset(&e->timer,
                    5 * hz,
                    SINGLE,
                    rte_lcore_id(),
                    flow_timeout_cb,
                    e);
}

/* ---------------- GLOBAL CLEAN ---------------- */

void check_global_idle() {

    uint64_t now = rte_rdtsc();

    if (!table_cleared && (now - global_last_pkt) > (10 * hz)) {

        printf("\n🔥 [GLOBAL TIMEOUT] No traffic → clearing ALL flows\n");

        const void *k;
        void *data;
        uint32_t iter = 0;

        while (rte_hash_iterate(nat_table, &k, &data, &iter) >= 0) {
            struct nat_entry *e = data;
            rte_free(e);
        }

        rte_hash_reset(nat_table);

        table_cleared = 1;
    }
}

/* ---------------- MAIN ---------------- */

int main(int argc, char **argv) {

    rte_eal_init(argc, argv);
    rte_timer_subsystem_init();

    hz = rte_get_timer_hz();

    /* -------- MBUF POOL -------- */

    struct rte_mempool *mbuf_pool =
        rte_pktmbuf_pool_create("MBUF_POOL",
            8192, 256, 0,
            RTE_MBUF_DEFAULT_BUF_SIZE,
            rte_socket_id());

    /* -------- PORT INIT -------- */

    uint16_t port_id = 0;
    struct rte_eth_conf port_conf = {0};

    rte_eth_dev_configure(port_id, 1, 0, &port_conf);

    rte_eth_rx_queue_setup(port_id, 0, 1024,
        rte_eth_dev_socket_id(port_id),
        NULL,
        mbuf_pool);

    rte_eth_dev_start(port_id);

    /* -------- HASH TABLE -------- */

    struct rte_hash_parameters params = {
        .name = "nat",
        .entries = 1024,
        .key_len = sizeof(struct nat_key),
        .hash_func = rte_jhash,
        .socket_id = rte_socket_id(),
    };

    nat_table = rte_hash_create(&params);

    global_last_pkt = rte_rdtsc();

    printf("🚀 NAT Started (Single Table, RX only)\n");

    /* -------- RX LOOP -------- */

    struct rte_mbuf *bufs[32];

    while (1) {

        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, 32);

        if (nb_rx > 0) {

            global_last_pkt = rte_rdtsc();
            table_cleared = 0;

            for (int i = 0; i < nb_rx; i++) {

                struct rte_mbuf *m = bufs[i];
                uint8_t *pkt = rte_pktmbuf_mtod(m, uint8_t *);

                struct rte_ether_hdr *eth =
                    (struct rte_ether_hdr *)pkt;

                if (eth->ether_type !=
                    rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                    rte_pktmbuf_free(m);
                    continue;
                }

                struct rte_ipv4_hdr *ip =
                    (struct rte_ipv4_hdr *)(pkt + sizeof(*eth));

                uint32_t src_ip = rte_be_to_cpu_32(ip->src_addr);
                uint32_t dst_ip = rte_be_to_cpu_32(ip->dst_addr);

                uint8_t *l4 = (uint8_t *)ip + sizeof(*ip);

                uint16_t src_port = (l4[0] << 8) | l4[1];
                uint16_t dst_port = (l4[2] << 8) | l4[3];

                struct nat_key key;
                memset(&key, 0, sizeof(key));

                key.src_ip = src_ip;
                key.src_port = src_port;
                key.dst_ip = dst_ip;
                key.dst_port = dst_port;

                struct nat_entry *entry;

                int ret = rte_hash_lookup_data(nat_table, &key, (void**)&entry);

                if (ret < 0) {

                    printf("\n🆕 [NEW FLOW] ");
                    print_ip(src_ip);
                    printf(":%d → ", src_port);
                    print_ip(dst_ip);
                    printf(":%d\n", dst_port);

                    entry = rte_malloc(NULL, sizeof(*entry), 0);

                    entry->key = key;

                    entry->private_ip = src_ip;
                    entry->private_port = src_port;
                    entry->public_ip = PUBLIC_IP;
                    entry->public_port = next_port++;

                    rte_timer_init(&entry->timer);

                    rte_hash_add_key_data(nat_table, &key, entry);

                } else {
                    printf("\n✅ [FLOW HIT] Refreshing existing flow\n");
                }

                refresh_timer(entry);

                printf("📦 Packet: ");
                print_ip(src_ip);
                printf(":%d → ", src_port);
                print_ip(dst_ip);
                printf(":%d\n", dst_port);

                rte_pktmbuf_free(m);
            }
        }

        /* 🔥 TIMER ENGINE */
        rte_timer_manage();

        /* 🔥 GLOBAL CLEAN */
        check_global_idle();
    }

    return 0;
}