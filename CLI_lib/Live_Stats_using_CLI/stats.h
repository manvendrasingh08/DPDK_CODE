#ifndef STATS_H
#define STATS_H

#include <stdint.h>

#define MAX_PORTS 4

struct port_stats {
    uint64_t rx_pkts;
    uint64_t tx_pkts;
    uint64_t rx_drop;
};

extern struct port_stats g_port_stats[MAX_PORTS];

#endif