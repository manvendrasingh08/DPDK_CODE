#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <cmdline.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_parse_string.h>
#include <cmdline_parse_num.h>
#include <cmdline_socket.h>

/* =========================================================
   COMMAND 1: show port stats
   ========================================================= */

/* Result structure */
struct cmd_show_port_stats_result {
    cmdline_fixed_string_t show;
    cmdline_fixed_string_t port;
    cmdline_fixed_string_t stats;
};

/* Callback */
static void
cmd_show_port_stats_parsed(void *parsed_result,
                          struct cmdline *cl,
                          void *data)
{
    (void)parsed_result;
    (void)data;

    cmdline_printf(cl, "Showing port stats...\n");
    cmdline_printf(cl, "Port 0: RX=1000 TX=900 DROP=10\n");
}

/* Tokens */
cmdline_parse_token_string_t cmd_show =
    TOKEN_STRING_INITIALIZER(struct cmd_show_port_stats_result,
                             show, "show");

cmdline_parse_token_string_t cmd_port =
    TOKEN_STRING_INITIALIZER(struct cmd_show_port_stats_result,
                             port, "port");

cmdline_parse_token_string_t cmd_stats =
    TOKEN_STRING_INITIALIZER(struct cmd_show_port_stats_result,
                             stats, "stats");

/* Command instance */
cmdline_parse_inst_t cmd_show_port_stats = {
    .f = cmd_show_port_stats_parsed,
    .data = NULL,
    .help_str = "show port stats",
    .tokens = {
        (void *)&cmd_show,
        (void *)&cmd_port,
        (void *)&cmd_stats,
        NULL,
    },
};


/* =========================================================
   COMMAND 2: set port <id> up/down
   ========================================================= */

/* Result structure */
struct cmd_set_port_result {
    cmdline_fixed_string_t set;
    cmdline_fixed_string_t port;
    uint16_t port_id;
    cmdline_fixed_string_t status;
};

/* Callback */
static void
cmd_set_port_parsed(void *parsed_result,
                    struct cmdline *cl,
                    void *data)
{
    struct cmd_set_port_result *res = parsed_result;
    (void)data;

    cmdline_printf(cl, "Setting port %u to %s\n",
                   res->port_id,
                   res->status);

    if (!strcmp(res->status, "up")) {
        cmdline_printf(cl, "Port %u is now UP\n", res->port_id);
    } else {
        cmdline_printf(cl, "Port %u is now DOWN\n", res->port_id);
    }
}

/* Tokens */
cmdline_parse_token_string_t cmd_set =
    TOKEN_STRING_INITIALIZER(struct cmd_set_port_result,
                             set, "set");

cmdline_parse_token_string_t cmd_port_kw =
    TOKEN_STRING_INITIALIZER(struct cmd_set_port_result,
                             port, "port");

/* Number token */
cmdline_parse_token_num_t cmd_port_id =
    TOKEN_NUM_INITIALIZER(struct cmd_set_port_result,
                          port_id, RTE_UINT16);

/* ENUM token (auto-complete works here) */
cmdline_parse_token_string_t cmd_status =
    TOKEN_STRING_INITIALIZER(struct cmd_set_port_result,
                             status, "up#down");

/* Command instance */
cmdline_parse_inst_t cmd_set_port = {
    .f = cmd_set_port_parsed,
    .data = NULL,
    .help_str = "set port <port_id> up|down",
    .tokens = {
        (void *)&cmd_set,
        (void *)&cmd_port_kw,
        (void *)&cmd_port_id,
        (void *)&cmd_status,
        NULL,
    },
};


/* =========================================================
   MAIN FUNCTION
   ========================================================= */

int main(int argc __attribute__((unused)),
         char **argv __attribute__((unused)))
{
    struct cmdline *cl;

    /* Command list */
    cmdline_parse_ctx_t main_ctx[] = {
        (cmdline_parse_inst_t *)&cmd_show_port_stats,
        (cmdline_parse_inst_t *)&cmd_set_port,
        NULL,
    };

    printf("Starting DPDK CLI...\n");

    /* Create CLI */
    cl = cmdline_stdin_new(main_ctx, "dpdk> ");
    if (cl == NULL)
        return -1;

    /* Start CLI loop */
    cmdline_interact(cl);

    /* Cleanup */
    cmdline_stdin_exit(cl);

    return 0;
}