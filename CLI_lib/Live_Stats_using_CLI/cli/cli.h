#ifndef CLI_H
#define CLI_H

#include <cmdline_parse.h>

extern cmdline_parse_inst_t cmd_show_port_stats;
extern cmdline_parse_inst_t cmd_set_port;
extern cmdline_parse_inst_t cmd_exit;

cmdline_parse_ctx_t *get_cli_ctx(void);

#endif