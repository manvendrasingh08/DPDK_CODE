#include "cli.h"
#include "stddef.h"

cmdline_parse_ctx_t *get_cli_ctx(void)
{
    static cmdline_parse_ctx_t ctx[] = {
        &cmd_show_port_stats,
        &cmd_set_port,
        &cmd_exit,
        NULL,
    };

    return ctx;
}