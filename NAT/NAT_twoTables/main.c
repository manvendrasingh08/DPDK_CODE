#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <rte_eal.h>
#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_malloc.h>
#include <rte_cycles.h>
#include <rte_timer.h>
#include <rte_lcore.h>

/* ---------------- CONFIG ---------------- */

#define PUBLIC_IP 0x0A000001
#define MAX_FLOWS 5

/* ---------------- PACKET ---------------- */

struct packet {
    uint32_t src_ip;
    uint16_t src_port;
    uint32_t dst_ip;
    uint16_t dst_port;
};

/* ---------------- KEYS ---------------- */

struct fwd_key {
    uint32_t src_ip;
    uint16_t src_port;
    uint32_t dst_ip;
    uint16_t dst_port;
};

struct rev_key {
    uint32_t public_ip;
    uint16_t public_port;
};

/* ---------------- ENTRY ---------------- */

struct nat_entry {
    struct fwd_key fwd;
    struct rev_key rev;

    uint32_t private_ip;
    uint16_t private_port;

    uint32_t public_ip;
    uint16_t public_port;

    struct rte_timer timer;
};

/* ---------------- GLOBAL ---------------- */

struct rte_hash *fwd_table;
struct rte_hash *rev_table;

uint16_t next_port = 10000;
uint64_t hz;

/* ---------------- UTIL ---------------- */

void print_ip(uint32_t ip) {
    printf("%d.%d.%d.%d",
        (ip>>24)&255,(ip>>16)&255,(ip>>8)&255,ip&255);
}

void print_pkt(const char *msg, struct packet *p) {
    printf("%s ", msg);
    print_ip(p->src_ip);
    printf(":%d → ", p->src_port);
    print_ip(p->dst_ip);
    printf(":%d\n", p->dst_port);
}

/* ---------------- TIMER ---------------- */

void flow_timeout_cb(struct rte_timer *tim, void *arg) {

    struct nat_entry *e = arg;

    printf("\n[EXPIRE] Flow: ");
    print_ip(e->private_ip);
    printf(":%d\n", e->private_port);

    rte_hash_del_key(fwd_table, &e->fwd);
    rte_hash_del_key(rev_table, &e->rev);

    rte_free(e);
}

void refresh_timer(struct nat_entry *e) {
    rte_timer_reset(&e->timer,
                    5 * hz,
                    SINGLE,
                    rte_lcore_id(),
                    flow_timeout_cb,
                    e);
}

/* ---------------- EGRESS (TX) ---------------- */

void process_egress(struct packet *pkt) {

    print_pkt("\nBEFORE NAT (TX):", pkt);

    struct fwd_key key = {0};
    key.src_ip = pkt->src_ip;
    key.src_port = pkt->src_port;
    key.dst_ip = pkt->dst_ip;
    key.dst_port = pkt->dst_port;

    struct nat_entry *entry;

    if (rte_hash_lookup_data(fwd_table, &key, (void**)&entry) < 0) {

        printf("Creating NAT mapping\n");

        entry = rte_malloc(NULL, sizeof(*entry), 0);

        entry->fwd = key;

        entry->private_ip = pkt->src_ip;
        entry->private_port = pkt->src_port;

        entry->public_ip = PUBLIC_IP;
        entry->public_port = next_port++;

        entry->rev.public_ip = entry->public_ip;
        entry->rev.public_port = entry->public_port;

        rte_timer_init(&entry->timer);

        rte_hash_add_key_data(fwd_table, &entry->fwd, entry);
        rte_hash_add_key_data(rev_table, &entry->rev, entry);
    }

    refresh_timer(entry);

    /*  NAT REWRITE (IMPORTANT) */
    pkt->src_ip = entry->public_ip;
    pkt->src_port = entry->public_port;

    print_pkt("AFTER NAT (TX):", pkt);
}

/* ---------------- INGRESS (RX) ---------------- */

void process_ingress(struct packet *pkt) {

    print_pkt("\n BEFORE NAT (RX):", pkt);

    struct rev_key key = {0};
    key.public_ip = pkt->dst_ip;
    key.public_port = pkt->dst_port;

    struct nat_entry *entry;

    if (rte_hash_lookup_data(rev_table, &key, (void**)&entry) < 0) {

        printf("No mapping → DROP\n");
        return;
    }

    refresh_timer(entry);

    /*  NAT REWRITE */
    pkt->dst_ip = entry->private_ip;
    pkt->dst_port = entry->private_port;

    print_pkt("AFTER NAT (RX):", pkt);
}

/* ---------------- MAIN ---------------- */

int main(int argc, char **argv) {

    rte_eal_init(argc, argv);
    rte_timer_subsystem_init();

    hz = rte_get_timer_hz();

    /* -------- HASH TABLES -------- */

    struct rte_hash_parameters fwd_params = {
        .name = "fwd",
        .entries = 1024,
        .key_len = sizeof(struct fwd_key),
        .hash_func = rte_jhash,
        .socket_id = rte_socket_id(),
    };

    struct rte_hash_parameters rev_params = {
        .name = "rev",
        .entries = 1024,
        .key_len = sizeof(struct rev_key),
        .hash_func = rte_jhash,
        .socket_id = rte_socket_id(),
    };

    fwd_table = rte_hash_create(&fwd_params);
    rev_table = rte_hash_create(&rev_params);

    printf("========= NAT Simulation with Packet Rewrite============\n");

    /* -------- TX PACKETS -------- */

    struct packet tx_pkts[MAX_FLOWS];

    for (int i = 0; i < MAX_FLOWS; i++) {
        tx_pkts[i].src_ip = 0xC0A80001 + i;  //192.168.0.1 + i
        tx_pkts[i].src_port = 1000 + i;
        tx_pkts[i].dst_ip = 0x08080808;   //8.8.8.8 --> google DNS
        tx_pkts[i].dst_port = 80;
    }

    /* -------- PROCESS TX -------- */
    printf("\n--- Processing Egress (TX) Packets ---\n");

    for (int i = 0; i < MAX_FLOWS; i++) {
        process_egress(&tx_pkts[i]);
        sleep(1);
    }

    sleep(2);

    /* -------- RX PACKETS -------- */
    printf("\n--- Processing Ingress (RX) Packets ---\n");
    
    struct packet rx_pkts[MAX_FLOWS];

    for (int i = 0; i < MAX_FLOWS; i++) {
        rx_pkts[i].src_ip = 0x08080808;  // 8.8.8.8 --> google DNS
        rx_pkts[i].src_port = 80;
        rx_pkts[i].dst_ip = PUBLIC_IP;
        rx_pkts[i].dst_port = 10000 + i;
    }

    /* -------- PROCESS RX -------- */

    for (int i = 0; i < MAX_FLOWS; i++) {
        process_ingress(&rx_pkts[i]);
        sleep(1);
    }

    printf("\nWaiting for expiry...\n");

    while (1) {
        rte_timer_manage();
        sleep(1);
    }

    return 0;
}