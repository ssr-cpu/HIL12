#ifndef HIL_CLI_H
#define HIL_CLI_H

#include "hil_common.h"
#include "hil_logger.h"

typedef enum hil_language {
    HIL_LANG_ZH = 0,
    HIL_LANG_EN
} hil_language_t;

typedef struct hil_cli_options {
    char config_path[HIL_PATH_MAX];
    char signals_path[HIL_PATH_MAX];
    char script_path[HIL_PATH_MAX];
    char output_dir[HIL_PATH_MAX];
    char csv_path[HIL_PATH_MAX];
    char signal_name[HIL_NAME_MAX];
    uint64_t from_ms;
    uint64_t to_ms;
    hil_language_t language;
    bool has_query;
} hil_cli_options_t;

void hil_cli_options_init(hil_cli_options_t *options);
const char *hil_cli_text(const hil_cli_options_t *options, const char *zh,
                         const char *en);
hil_status_t hil_cli_parse(hil_cli_options_t *options, int argc, char **argv,
                           const hil_logger_t *logger);
hil_status_t hil_cli_run(const hil_cli_options_t *options,
                         const hil_logger_t *logger);
hil_status_t hil_cli_query(const hil_cli_options_t *options,
                           const hil_logger_t *logger);
void hil_cli_print_help(const hil_cli_options_t *options, FILE *stream);

#endif
