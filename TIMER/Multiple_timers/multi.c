#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <rte_eal.h>
#include <rte_timer.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <unistd.h>

#define NUM_TIMERS 5

static struct rte_timer timers[NUM_TIMERS];

/* Callback */
static void
timer_cb(struct rte_timer *tim, void *arg)
{
    int id = *(int *)arg;

    printf("🔥 Timer %d fired on lcore %u at TSC=%" PRIu64 "\n",
           id, rte_lcore_id(), rte_get_timer_cycles());
}

int main(int argc, char **argv)
{
    uint64_t hz;
    uint64_t start_tsc;

    int ids[NUM_TIMERS] = {0, 1, 2, 3, 4};

    if (rte_eal_init(argc, argv) < 0)
        rte_panic("EAL init failed\n");

    printf("Starting 5-timer demo...\n");

    rte_timer_subsystem_init();

    hz = rte_get_timer_hz();

    start_tsc = rte_get_timer_cycles();
    printf("Start TSC = %" PRIu64 "\n", start_tsc);

    /* Initialize timers */
    for (int i = 0; i < NUM_TIMERS; i++) {
        rte_timer_init(&timers[i]);
    }

    /* Set timers with different delays */
    rte_timer_reset(&timers[0], 1 * hz, SINGLE, rte_lcore_id(), timer_cb, &ids[0]); // 1 sec
    rte_timer_reset(&timers[1], 3 * hz, SINGLE, rte_lcore_id(), timer_cb, &ids[1]); // 3 sec
    rte_timer_reset(&timers[2], 2 * hz, SINGLE, rte_lcore_id(), timer_cb, &ids[2]); // 2 sec
    rte_timer_reset(&timers[3], 5 * hz, SINGLE, rte_lcore_id(), timer_cb, &ids[3]); // 5 sec
    rte_timer_reset(&timers[4], 4 * hz, SINGLE, rte_lcore_id(), timer_cb, &ids[4]); // 4 sec

    printf("⏳ Timers set: 1s, 3s, 2s, 5s, 4s\n");

    /* Loop */
    while (1) {
        rte_timer_manage();
        sleep(3);
    }

    return 0;
}