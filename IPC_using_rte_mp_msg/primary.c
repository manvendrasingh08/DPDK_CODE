#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <rte_eal.h>
#include <rte_common.h>

#include "common.h"

int main(int argc, char **argv)
{
    struct rte_mp_msg msg;
    struct ready_payload payload = {
        .port_id = 0,
        .status  = 1,
    };

    if (rte_eal_init(argc, argv) < 0)
    rte_exit(EXIT_FAILURE, "EAL init failed\n");
        

    memset(&msg, 0, sizeof(msg));

    snprintf(msg.name, RTE_MP_MAX_NAME_LEN, MSG_NAME);

    memcpy(msg.param, &payload, sizeof(payload));
    msg.len_param = sizeof(payload);

    printf("Primary sending message...\n");

    while(1){
    rte_mp_sendmsg(&msg);
    sleep(5);
    }

    return 0;
}
