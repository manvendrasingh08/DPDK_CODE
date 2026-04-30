#include <stdio.h>
#include <cmdline.h>
#include <cmdline_parse.h>
#include <cmdline_parse_string.h>
#include "cli.h"
#include "../stats.h"

struct cmd_show_port_stats_result {
    cmdline_fixed_string_t show;
    cmdline_fixed_string_t port;
    cmdline_fixed_string_t stats;
};

static void
cmd_show_port_stats_parsed(void *parsed_result,
                          struct cmdline *cl,
                          void *data)
{
    (void)parsed_result;
    (void)data;

    cmdline_printf(cl, "\n==== PORT STATS ====\n");

    for (int i = 0; i < MAX_PORTS; i++) {
        cmdline_printf(cl,
            "Port %d: RX=%lu TX=%lu DROP=%lu\n",
            i,
            g_port_stats[i].rx_pkts,
            g_port_stats[i].tx_pkts,
            g_port_stats[i].rx_drop
        );
    }
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

/* Command */
cmdline_parse_inst_t cmd_show_port_stats = {
    .f = cmd_show_port_stats_parsed,
    .data = NULL,
    .help_str = "show port stats",
    .tokens = {
    (void *)&cmd_show,
    (void *)&cmd_port,
    (void *)&cmd_stats,
    NULL,
}
};