#include <cmdline.h>
#include <cmdline_parse.h>
#include <cmdline_parse_string.h>

struct cmd_exit_result {
    cmdline_fixed_string_t exit;
};

static void
cmd_exit_parsed(void *parsed_result,
                struct cmdline *cl,
                void *data)
{
    (void)parsed_result;
    (void)data;

    cmdline_printf(cl, "Exiting CLI...\n");
    cmdline_quit(cl);   // 🔥 THIS exits cleanly
}

/* Token */
cmdline_parse_token_string_t cmd_exit_tok =
    TOKEN_STRING_INITIALIZER(struct cmd_exit_result,
                             exit, "exit");

/* Command */
cmdline_parse_inst_t cmd_exit = {
    .f = cmd_exit_parsed,
    .data = NULL,
    .help_str = "exit CLI",
    .tokens = {
        (void *)&cmd_exit_tok,
        NULL,
    },
};