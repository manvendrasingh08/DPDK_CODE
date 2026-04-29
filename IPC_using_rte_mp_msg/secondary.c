    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>

    #include <rte_eal.h>
    #include <rte_common.h>

    #include "common.h"

    /* This callback is called by DPDK */
    static int
    ready_handler(const struct rte_mp_msg *msg, const void *peer)
    {
        struct ready_payload payload;

        memcpy(&payload, msg->param, sizeof(payload));

        printf("CALLBACK EXECUTEDn");
        printf("Message name : %s\n", msg->name);
        printf("Port ID      : %d\n", payload.port_id);
        printf("Status       : %d\n", payload.status);
        printf("=======================\n");

        return 0;
    }

    int main(int argc, char **argv)
    {
        if (rte_eal_init(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");

        /* Register callback */
        rte_mp_action_register(MSG_NAME, ready_handler);

        printf("Secondary started, waiting for messages...\n");

        /* Keep process alive */
        while (1)
            sleep(1);

        return 0;
    }

    