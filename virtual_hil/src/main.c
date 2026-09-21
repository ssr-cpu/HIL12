#include "hil_cli.h"

#include <stdio.h>
#include <string.h>
//
//
int main(int argc, char **argv)
{
    hil_logger_t logger;
    hil_cli_options_t options;
    hil_status_t status;
    hil_logger_init(&logger, stderr, HIL_LOG_INFO);
    hil_cli_options_init(&options);

    if (argc >= 2 &&
        (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
         strcmp(argv[1], "-h") == 0)) {
        hil_cli_print_help(&options, stdout);
        return 0;
    }

    status = hil_cli_parse(&options, argc, argv, &logger);
    if (status == HIL_ERR_UNSUPPORTED) {
        return 0;
    }
    if (status != HIL_OK) {
        hil_log_error_status(&logger, "main", status,
                             "command line parsing failed");
        return 1;
    }
    if (options.has_query) {
        status = hil_cli_query(&options, &logger);
    } else {
        status = hil_cli_run(&options, &logger);
    }
    if (status != HIL_OK) {
        hil_log_error_status(&logger, "main", status,
                             "operation failed");
        return 1;
    }
    return 0;
}
