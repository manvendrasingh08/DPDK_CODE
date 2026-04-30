#include <stdio.h>
#include <rte_eal.h>
#include <rte_lcore.h>

#include <cmdline.h>
#include <cmdline_socket.h>

#include "cli/cli.h"

/* Forward declarations */
void rx_loop(uint16_t port_id);
void tx_loop(uint16_t port_id);

/* Wrappers for lcore launch */
static int
rx_lcore_main(void *arg)
{
    uint16_t port_id = (uintptr_t)arg;
    rx_loop(port_id);
    return 0;
}

static int
tx_lcore_main(void *arg)
{
    uint16_t port_id = (uintptr_t)arg;
    tx_loop(port_id);
    return 0;
}

int main(int argc, char **argv)
{
    struct cmdline *cl;
    unsigned lcore_id;

    /* Initialize EAL */
    if (rte_eal_init(argc, argv) < 0) {
        printf("EAL init failed\n");
        return -1;
    }

    printf("Starting DPDK multi-core app...\n");

    /* Launch RX and TX on slave lcores */
    int launched = 0;

    RTE_LCORE_FOREACH(lcore_id) {
        
         if (lcore_id == rte_get_main_lcore())
        continue;   
        
        if (launched == 0) {
            printf("Launching RX on lcore %u\n", lcore_id);
            rte_eal_remote_launch(rx_lcore_main,
                                  (void *)(uintptr_t)0,
                                  lcore_id);
            launched++;
        } else if (launched == 1) {
            printf("Launching TX on lcore %u\n", lcore_id);
            rte_eal_remote_launch(tx_lcore_main,
                                  (void *)(uintptr_t)0,
                                  lcore_id);
            launched++;
        }
    }

    /* Run CLI on main lcore */
    cl = cmdline_stdin_new(get_cli_ctx(), "dpdk> ");
    if (!cl)
        return -1;

    cmdline_interact(cl);

    cmdline_stdin_exit(cl);

    /* Wait for lcores (won’t actually return due to infinite loops) */
    rte_eal_mp_wait_lcore();

    return 0;
}