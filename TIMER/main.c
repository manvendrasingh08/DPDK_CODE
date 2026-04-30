#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <rte_eal.h>
#include <rte_timer.h>
#include <rte_cycles.h>
#include <rte_lcore.h>

/* Timer object */
static struct rte_timer my_timer;

/* Callback function */
static void
timer_cb(struct rte_timer *tim, void *arg)
{
    printf("✅ Timer callback triggered on lcore %u!\n", rte_lcore_id());
}

/* Main */
int main(int argc, char **argv)
{
    uint64_t hz;

    /* Init DPDK EAL */
    if (rte_eal_init(argc, argv) < 0)
        rte_panic("EAL init failed\n");

    printf("Starting simple timer demo...\n");

    /* Init timer subsystem */
    rte_timer_subsystem_init();

    /* Init timer */
    rte_timer_init(&my_timer);

    /* Get timer frequency */
    hz = rte_get_timer_hz();

    /* Set one-shot timer (fires after 3 seconds) */
    rte_timer_reset(&my_timer,
                    3 * hz,              // delay = 3 seconds
                    SINGLE,              // one-shot
                    rte_lcore_id(),      // current core
                    timer_cb,
                    NULL);

    printf("⏳ Timer set for 3 seconds...\n");

    /* Loop to drive timer */
    while (1) {
        rte_timer_manage();    //walks until first non-expired timer
    }

    return 0;
}