#include <stdio.h>
#include <string.h>
#include <cmdline.h>
#include <cmdline_parse.h>
#include <cmdline_parse_string.h>
#include <cmdline_parse_num.h>
#include "cli.h"

struct cmd_set_port_result {
    cmdline_fixed_string_t set;
    cmdline_fixed_string_t port;
    uint16_t port_id;
    cmdline_fixed_string_t status;
};

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
}

/* Tokens */
cmdline_parse_token_string_t cmd_set =
    TOKEN_STRING_INITIALIZER(struct cmd_set_port_result,
                             set, "set");

cmdline_parse_token_string_t cmd_port_kw =
    TOKEN_STRING_INITIALIZER(struct cmd_set_port_result,
                             port, "port");

cmdline_parse_token_num_t cmd_port_id =
    TOKEN_NUM_INITIALIZER(struct cmd_set_port_result,
                          port_id, RTE_UINT16);

cmdline_parse_token_string_t cmd_status =
    TOKEN_STRING_INITIALIZER(struct cmd_set_port_result,
                             status, "up#down");

/* Command */
cmdline_parse_inst_t cmd_set_port = {
    .f = cmd_set_port_parsed,
    .data = NULL,
    .help_str = "set port <id> up|down",
    .tokens = {
    (void *)&cmd_set,
    (void *)&cmd_port_kw,
    (void *)&cmd_port_id,
    (void *)&cmd_status,
    NULL,
}
};