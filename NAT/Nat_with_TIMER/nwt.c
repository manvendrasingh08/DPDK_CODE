#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_malloc.h>
#include <rte_cycles.h>
#include <rte_timer.h>
#include <rte_lcore.h>

/* ---------------- CONFIG ---------------- */

#define PUBLIC_IP 0x0A000001

/* ---------------- PACKET ---------------- */

struct packet {
    uint32_t src_ip, dst_ip;
    uint16_t src_port, dst_port;
    uint8_t proto;
    uint8_t is_ingress;
};

/* ---------------- KEY ---------------- */

struct nat_key {
    uint32_t ip;
    uint16_t port;
    uint32_t dst_ip;
    uint16_t dst_port;
};

/* ---------------- ENTRY ---------------- */

struct nat_entry {
    struct nat_key key;   // ✅ FIX: store key

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

    struct nat_entry *entry = arg;

    printf("Flow expired: ");
    print_ip(entry->private_ip);
    printf(":%d\n", entry->private_port);

    /* ✅ Correct deletion using stored key */
    rte_hash_del_key(nat_table, &entry->key);

    rte_free(entry);
}

/* ---------------- RESET TIMER ---------------- */

void refresh_timer(struct nat_entry *entry) {

    rte_timer_reset(&entry->timer,
                    5 * hz,   // 5 sec timeout
                    SINGLE,
                    rte_lcore_id(),
                    flow_timeout_cb,
                    entry);
}

/* ---------------- NAT ---------------- */

void process_packet(struct packet *pkt) {

    global_last_pkt = rte_rdtsc();
    table_cleared = 0;  //reset idle flag

    struct nat_key key;
    struct nat_entry *entry;

    if (!pkt->is_ingress) {

        /* -------- EGRESS -------- */

        key.ip = pkt->src_ip;
        key.port = pkt->src_port;
        key.dst_ip = pkt->dst_ip;
        key.dst_port = pkt->dst_port;

        int ret = rte_hash_lookup_data(nat_table, &key, (void**)&entry);

        if (ret < 0) {

            printf("[NEW FLOW]\n");

            entry = rte_malloc(NULL, sizeof(*entry), 0);

            entry->key = key;  //store key

            entry->private_ip = pkt->src_ip;
            entry->private_port = pkt->src_port;
            entry->public_ip = PUBLIC_IP;
            entry->public_port = next_port++;

            rte_timer_init(&entry->timer);

            rte_hash_add_key_data(nat_table, &key, entry);
        }

        refresh_timer(entry);

        pkt->src_ip = entry->public_ip;
        pkt->src_port = entry->public_port;

    } else {

        /* -------- INGRESS -------- */

        const void *k;
        void *data;
        uint32_t iter = 0;

        while (rte_hash_iterate(nat_table, &k, &data, &iter) >= 0) {

            entry = data;

            if (entry->public_ip == pkt->dst_ip &&
                entry->public_port == pkt->dst_port) {

                pkt->dst_ip = entry->private_ip;
                pkt->dst_port = entry->private_port;

                refresh_timer(entry);
                break;
            }
        }
    }

    printf("Packet: ");
    print_ip(pkt->src_ip);
    printf(":%d -> ", pkt->src_port);
    print_ip(pkt->dst_ip);
    printf(":%d\n", pkt->dst_port);
}

/* ---------------- GLOBAL CLEAN ---------------- */

void check_global_idle() {

    uint64_t now = rte_rdtsc();

    if (!table_cleared && (now - global_last_pkt) > (10 * hz)) {

        printf("\n🔥 No traffic → clearing NAT table\n");

        const void *key;
        void *data;
        uint32_t iter = 0;

        while (rte_hash_iterate(nat_table, &key, &data, &iter) >= 0) {
            struct nat_entry *e = data;
            rte_free(e);
        }

        rte_hash_reset(nat_table);

        table_cleared = 1;  //prevent repeated clearing
    }
}

/* ---------------- MAIN ---------------- */

int main(int argc, char **argv) {

    rte_eal_init(argc, argv);
    rte_timer_subsystem_init();

    hz = rte_get_timer_hz();

    struct rte_hash_parameters params = {
        .name = "nat",
        .entries = 1024,
        .key_len = sizeof(struct nat_key),
        .hash_func = rte_jhash,
        .socket_id = rte_socket_id(),
    };

    nat_table = rte_hash_create(&params);

    global_last_pkt = rte_rdtsc();

    /* -------- TEST TRAFFIC -------- */

    struct packet p1 = {0xC0A80001, 0x08080808, 1111, 80, 6, 0};
    struct packet p2 = {0xC0A80002, 0x08080808, 2222, 80, 6, 0};

    printf("\n=== START ===\n");

    process_packet(&p1);
    sleep(2);

    process_packet(&p2);
    sleep(2);

    process_packet(&p1); // refresh flow
    sleep(6);            // let one expire

    /* -------- LOOP -------- */

    while (1) {
        rte_timer_manage();
        check_global_idle();
        sleep(1);
    }

    return 0;
}