#include <unistd.h>
#include "stats.h"

void rx_loop(uint16_t port_id)
{
    while (1) {
        g_port_stats[port_id].rx_pkts += 10;
        usleep(100000); // simulate delay
    }
}