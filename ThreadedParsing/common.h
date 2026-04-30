#ifndef COMMON_H
#define COMMON_H

#include <rte_ring.h>
#include <rte_mbuf.h>
#include <stdint.h>

#define BURST_SIZE 32

extern struct rte_ring *rx_ring;
extern struct rte_ring *tx_ring;
extern uint16_t port_id;

#endif
