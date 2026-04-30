#include <unistd.h>
#include "stats.h"

void tx_loop(uint16_t port_id)
{
    while (1) {
        g_port_stats[port_id].tx_pkts += 8;
        usleep(100000);
    }
}