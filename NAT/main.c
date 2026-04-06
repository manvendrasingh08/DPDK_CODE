#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_malloc.h>
#include <rte_cycles.h>
#include <rte_eal.h>

/* ---------------- CONFIG ---------------- */

#define MAX_PACKETS 10
#define PUBLIC_IP   0x0A000001  // 10.0.0.1

/* ---------------- PACKET STRUCT ---------------- */

struct packet {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;   // 6 = TCP, 17 = UDP
    uint8_t  is_ingress; // 0 = outgoing, 1 = incoming
};

/* ---------------- NAT KEY ---------------- */

struct nat_key {
    uint32_t ip;
    uint16_t port;
    uint32_t dst_ip;
    uint16_t dst_port;
    uint8_t  proto;
};

/* ---------------- NAT ENTRY ---------------- */

struct nat_entry {
    uint32_t private_ip;
    uint16_t private_port;

    uint32_t public_ip;
    uint16_t public_port;

    uint32_t dest_ip;
    uint16_t dest_port;

    uint64_t last_seen;
};

/* ---------------- GLOBALS ---------------- */

struct rte_hash *nat_table;
uint16_t next_port = 10000;

uint64_t timeout_cycles;

/* ---------------- UTIL ---------------- */

void print_ip(uint32_t ip) {
    printf("%d.%d.%d.%d",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF,
        ip & 0xFF);
}

/* ---------------- PORT ALLOC ---------------- */

uint16_t allocate_port() {
    return next_port++;
}

/* ---------------- NAT CALLBACK ---------------- */

void nat_callback(struct packet *pkt) {

    printf("\n---- Processing Packet ----\n");

    printf("Original: ");
    print_ip(pkt->src_ip);
    printf(":%d -> ", pkt->src_port);
    print_ip(pkt->dst_ip);
    printf(":%d\n", pkt->dst_port);

    struct nat_key key;
    struct nat_entry *entry;

    if (pkt->is_ingress == 0) {
        /* -------- EGRESS -------- */

        key.ip = pkt->src_ip;
        key.port = pkt->src_port;
        key.dst_ip = pkt->dst_ip;
        key.dst_port = pkt->dst_port;
        key.proto = pkt->protocol;

        int ret = rte_hash_lookup_data(nat_table, &key, (void **)&entry);

        if (ret >= 0) {
            printf("[HIT] Existing NAT mapping found\n");
            entry->last_seen = rte_rdtsc();
        } else {
            printf("[MISS] Creating new NAT mapping\n");

            entry = rte_malloc(NULL, sizeof(*entry), 0);

            entry->private_ip = pkt->src_ip;
            entry->private_port = pkt->src_port;
            entry->public_ip = PUBLIC_IP;
            entry->public_port = allocate_port();
            entry->dest_ip = pkt->dst_ip;
            entry->dest_port = pkt->dst_port;
            entry->last_seen = rte_rdtsc();

            rte_hash_add_key_data(nat_table, &key, entry);
        }

        /* Rewrite packet */
        pkt->src_ip = entry->public_ip;
        pkt->src_port = entry->public_port;

        printf("Translated (Egress): ");
        print_ip(pkt->src_ip);
        printf(":%d -> ", pkt->src_port);
        print_ip(pkt->dst_ip);
        printf(":%d\n", pkt->dst_port);

    } else {
        /* -------- INGRESS -------- */

        const void *k;
        void *data;
        uint32_t iter = 0;

        int found = 0;

        while (rte_hash_iterate(nat_table, &k, &data, &iter) >= 0) {
            entry = data;

            if (entry->public_ip == pkt->dst_ip &&
                entry->public_port == pkt->dst_port) {

                printf("[REVERSE HIT] Found mapping\n");

                pkt->dst_ip = entry->private_ip;
                pkt->dst_port = entry->private_port;

                entry->last_seen = rte_rdtsc();

                found = 1;
                break;
            }
        }

        if (!found) {
            printf("[DROP] No mapping found for ingress packet\n");
            return;
        }

        printf("Translated (Ingress): ");
        print_ip(pkt->src_ip);
        printf(":%d -> ", pkt->src_port);
        print_ip(pkt->dst_ip);
        printf(":%d\n", pkt->dst_port);
    }
}

/* ---------------- CLEANUP ---------------- */

void nat_cleanup() {

    const void *key;
    void *data;
    uint32_t iter = 0;

    uint64_t now = rte_rdtsc();

    while (rte_hash_iterate(nat_table, &key, &data, &iter) >= 0) {

        struct nat_entry *entry = data;

        if ((now - entry->last_seen) > timeout_cycles) {
            printf("[TIMEOUT] Deleting mapping\n");
            rte_hash_del_key(nat_table, key);
            rte_free(entry);
        }
    }
}

/* ---------------- MAIN ---------------- */

int main(int argc, char **argv) {

    rte_eal_init(argc, argv);

    /* Create hash */
    struct rte_hash_parameters params = {
        .name = "nat_table",
        .entries = 1024,
        .key_len = sizeof(struct nat_key),
        .hash_func = rte_jhash,
        .hash_func_init_val = 0,
        .socket_id = rte_socket_id(),
    };

    nat_table = rte_hash_create(&params);

    uint64_t hz = rte_get_tsc_hz();
    timeout_cycles = hz * 20;

    /* ---------------- TEST DATA ---------------- */

    struct packet egress_pkts[MAX_PACKETS] = {
        {0xC0A80001, 0x08080808, 1234, 80, 6, 0},
        {0xC0A80001, 0x08080808, 1234, 80, 6, 0},
    };

        struct packet ingress_pkts[MAX_PACKETS] = {
            {0x08080808, PUBLIC_IP, 80, 10000, 6, 1},
        };

    /* ---------------- PROCESS ---------------- */

    printf("\n=== EGRESS TRAFFIC ===\n");

    for (int i = 0; i < 2; i++) {
        nat_callback(&egress_pkts[i]);
    }

    printf("\n=== INGRESS TRAFFIC ===\n");

    for (int i = 0; i < 1; i++) {
        nat_callback(&ingress_pkts[i]);
    }

    printf("\n=== WAITING FOR TIMEOUT (simulate) ===\n");


    nat_cleanup();

    return 0;
}