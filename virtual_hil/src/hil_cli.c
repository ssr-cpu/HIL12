#include "hil_cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hil_config.h"
#include "hil_data_logger.h"
#include "hil_executor.h"
#include "hil_report.h"
#include "hil_signal.h"

#if defined(_WIN32)
#include <direct.h>
#define HIL_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define HIL_MKDIR(path) mkdir(path, 0755)
#endif

void hil_cli_options_init(hil_cli_options_t *options)
{
    if (options == NULL) {
        return;
    }
    (void)memset(options, 0, sizeof(*options));
    (void)strncpy(options->output_dir, "out", sizeof(options->output_dir) - 1U);
    options->from_ms = 0U;
    options->to_ms = UINT64_MAX;
    options->language = HIL_LANG_ZH;
    options->has_query = false;
}

const char *hil_cli_text(const hil_cli_options_t *options, const char *zh,
                         const char *en)
{
    if (options != NULL && options->language == HIL_LANG_EN) {
        return en;
    }
    return zh;
}

static hil_status_t copy_option(char *destination, size_t destination_size,
                                const char *value)
{
    if (value == NULL || value[0] == '\0') {
        return HIL_ERR_NULL;
    }
    if (strlen(value) >= destination_size) {
        return HIL_ERR_OVERFLOW;
    }
    (void)strncpy(destination, value, destination_size - 1U);
    destination[destination_size - 1U] = '\0';
    return HIL_OK;
}

static hil_status_t require_value(int argc, char **argv, int *index,
                                  const char **value)
{
    if (*index + 1 >= argc) {
        return HIL_ERR_NULL;
    }
    (*index)++;
    *value = argv[*index];
    return HIL_OK;
}

hil_status_t hil_cli_parse(hil_cli_options_t *options, int argc, char **argv,
                           const hil_logger_t *logger)
{
    int index;
    const char *command;
    if (options == NULL || argv == NULL || argc < 2) {
        return HIL_ERR_NULL;
    }
    command = argv[1];
    if (strcmp(command, "query") == 0) {
        options->has_query = true;
    } else if (strcmp(command, "run") == 0) {
        options->has_query = false;
    } else if (strcmp(command, "version") == 0) {
        (void)printf("virtual_hil %s\n", hil_version_string());
        return HIL_ERR_UNSUPPORTED;
    } else {
        hil_cli_print_help(options, stderr);
        return HIL_ERR_UNSUPPORTED;
    }

    for (index = 2; index < argc; index++) {
        const char *arg = argv[index];
        const char *value = NULL;
        uint64_t number = 0U;
        if (strcmp(arg, "--config") == 0 || strcmp(arg, "-c") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK) {
                return HIL_ERR_NULL;
            }
            status = copy_option(options->config_path,
                                 sizeof(options->config_path), value);
            if (status != HIL_OK) {
                return status;
            }
        } else if (strcmp(arg, "--signals") == 0 ||
                   strcmp(arg, "-s") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK) {
                return HIL_ERR_NULL;
            }
            status = copy_option(options->signals_path,
                                 sizeof(options->signals_path), value);
            if (status != HIL_OK) {
                return status;
            }
        } else if (strcmp(arg, "--script") == 0 ||
                   strcmp(arg, "-t") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK) {
                return HIL_ERR_NULL;
            }
            status = copy_option(options->script_path,
                                 sizeof(options->script_path), value);
            if (status != HIL_OK) {
                return status;
            }
        } else if (strcmp(arg, "--out") == 0 || strcmp(arg, "-o") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK) {
                return HIL_ERR_NULL;
            }
            status = copy_option(options->output_dir,
                                 sizeof(options->output_dir), value);
            if (status != HIL_OK) {
                return status;
            }
        } else if (strcmp(arg, "--csv") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK) {
                return HIL_ERR_NULL;
            }
            status = copy_option(options->csv_path, sizeof(options->csv_path),
                                 value);
            if (status != HIL_OK) {
                return status;
            }
        } else if (strcmp(arg, "--signal") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK) {
                return HIL_ERR_NULL;
            }
            status = copy_option(options->signal_name,
                                 sizeof(options->signal_name), value);
            if (status != HIL_OK) {
                return status;
            }
        } else if (strcmp(arg, "--from") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK || !hil_parse_u64(value, &number)) {
                return HIL_ERR_FORMAT;
            }
            options->from_ms = number;
        } else if (strcmp(arg, "--to") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK || !hil_parse_u64(value, &number)) {
                return HIL_ERR_FORMAT;
            }
            options->to_ms = number;
        } else if (strcmp(arg, "--lang") == 0) {
            hil_status_t status = require_value(argc, argv, &index, &value);
            if (status != HIL_OK) {
                return HIL_ERR_NULL;
            }
            if (strcmp(value, "zh") == 0) {
                options->language = HIL_LANG_ZH;
            } else if (strcmp(value, "en") == 0) {
                options->language = HIL_LANG_EN;
            } else {
                return HIL_ERR_FORMAT;
            }
        } else {
            hil_log_message(logger, HIL_LOG_ERROR, "cli",
                            "unknown option: %s", arg);
            return HIL_ERR_UNSUPPORTED;
        }
    }

    if (options->has_query) {
        if (options->csv_path[0] == '\0' || options->signal_name[0] == '\0') {
            hil_log_message(logger, HIL_LOG_ERROR, "cli",
                            "query requires --csv and --signal");
            return HIL_ERR_NULL;
        }
        if (options->from_ms > options->to_ms) {
            return HIL_ERR_RANGE;
        }
    } else {
        if (options->script_path[0] == '\0') {
            hil_log_message(logger, HIL_LOG_ERROR, "cli",
                            "run requires --script");
            return HIL_ERR_NULL;
        }
    }
    return HIL_OK;
}

static hil_status_t create_output_dir(const char *path,
                                      const hil_logger_t *logger)
{
    if (path == NULL || path[0] == '\0') {
        return HIL_ERR_NULL;
    }
    if (HIL_MKDIR(path) != 0) {
        /* EEXIST is acceptable. Standard C cannot inspect errno portably. */
        hil_log_message(logger, HIL_LOG_DEBUG, "cli",
                        "output directory already exists or cannot be created: %s",
                        path);
    }
    return HIL_OK;
}

static hil_status_t build_path(const char *directory, const char *name,
                               char *buffer, size_t buffer_size)
{
    int written;
    if (directory == NULL || name == NULL || buffer == NULL) {
        return HIL_ERR_NULL;
    }
    written = snprintf(buffer, buffer_size, "%s/%s", directory, name);
    if (written < 0 || (size_t)written >= buffer_size) {
        return HIL_ERR_OVERFLOW;
    }
    return HIL_OK;
}

static hil_status_t load_default_signals(hil_signal_registry_t *registry,
                                         const hil_logger_t *logger)
{
    static const hil_signal_t defaults[] = {
        {"battery_voltage", "V", 9.0, 16.0, 12.5, 12.5, 12.5, true, false},
        {"engine_speed", "rpm", 0.0, 8000.0, 0.0, 0.0, 0.0, true, false},
        {"vehicle_speed", "km/h", 0.0, 260.0, 0.0, 0.0, 0.0, true, false},
        {"coolant_temp", "degC", -40.0, 140.0, 25.0, 25.0, 25.0, true, false},
        {"throttle_position", "%", 0.0, 100.0, 0.0, 0.0, 0.0, true, false},
        {"brake_pressure", "bar", 0.0, 250.0, 0.0, 0.0, 0.0, true, false},
        {"fuel_level", "%", 0.0, 100.0, 60.0, 60.0, 60.0, true, false},
        {"oil_pressure", "bar", 0.0, 8.0, 1.2, 1.2, 1.2, true, false},
        {"gear_position", "", 0.0, 8.0, 0.0, 0.0, 0.0, true, false},
        {"ambient_temp", "degC", -40.0, 80.0, 25.0, 25.0, 25.0, true, false},
        {"ecu_state", "", 0.0, 6.0, 0.0, 0.0, 0.0, true, false},
        {"ecu_fault_latched", "", 0.0, 1.0, 0.0, 0.0, 0.0, true, false}
    };
    size_t index;
    for (index = 0U; index < HIL_ARRAY_LEN(defaults); index++) {
        hil_status_t status =
            hil_signal_registry_add(registry, &defaults[index], logger);
        if (status != HIL_OK) {
            return status;
        }
    }
    return HIL_OK;
}

hil_status_t hil_cli_run(const hil_cli_options_t *options,
                         const hil_logger_t *logger)
{
    hil_config_t config;
    hil_signal_registry_t signals;
    hil_data_logger_t data_logger;
    hil_report_builder_t report;
    hil_executor_t executor;
    char log_path[HIL_PATH_MAX];
    char report_path[HIL_PATH_MAX];
    hil_status_t status;
    size_t passed;
    size_t failed;
    size_t skipped;
    size_t errors;

    if (options == NULL) {
        return HIL_ERR_NULL;
    }
    hil_config_init(&config);
    if (options->config_path[0] != '\0') {
        status = hil_config_load(&config, options->config_path, logger);
        if (status != HIL_OK) {
            return status;
        }
    }
    if (options->output_dir[0] != '\0') {
        status = hil_config_set_default_paths(&config, options->output_dir);
        if (status != HIL_OK) {
            return status;
        }
    }
    status = create_output_dir(config.output_dir, logger);
    if (status != HIL_OK) {
        return status;
    }
    status = build_path(config.output_dir, config.log_file, log_path,
                        sizeof(log_path));
    if (status != HIL_OK) {
        return status;
    }
    status = build_path(config.output_dir, config.report_file, report_path,
                        sizeof(report_path));
    if (status != HIL_OK) {
        return status;
    }

    hil_report_init(&report);
    hil_signal_registry_init(&signals);
    if (options->signals_path[0] != '\0') {
        status = hil_signal_registry_load_csv(&signals, options->signals_path,
                                              logger);
    } else {
        status = load_default_signals(&signals, logger);
    }
    if (status != HIL_OK) {
        goto cleanup;
    }

    status = hil_data_logger_open(&data_logger, log_path, &signals, logger);
    if (status != HIL_OK) {
        goto cleanup;
    }
    status = hil_executor_init(&executor, &config, &signals,
                               options->script_path, &data_logger, &report,
                               logger);
    if (status != HIL_OK) {
        hil_executor_deinit(&executor);
        (void)hil_data_logger_close(&data_logger, logger);
        goto cleanup;
    }
    (void)hil_executor_run(&executor, logger);
    hil_executor_deinit(&executor);
    status = hil_data_logger_close(&data_logger, logger);
    if (status != HIL_OK) {
        goto cleanup;
    }
    status = hil_report_write_markdown(&report, report_path, logger);
    hil_report_summary(&report, &passed, &failed, &skipped, &errors);
    (void)printf("%s: %llu, %s: %llu, %s: %llu, %s: %llu\n",
                 hil_cli_text(options, "通过", "passed"),
                 (unsigned long long)passed,
                 hil_cli_text(options, "失败", "failed"),
                 (unsigned long long)failed,
                 hil_cli_text(options, "跳过", "skipped"),
                 (unsigned long long)skipped,
                 hil_cli_text(options, "错误", "errors"),
                 (unsigned long long)errors);
    if (!hil_report_all_passed(&report)) {
        status = HIL_ERR_ASSERT;
    }

cleanup:
    hil_report_deinit(&report);
    hil_signal_registry_deinit(&signals);
    return status;
}

hil_status_t hil_cli_query(const hil_cli_options_t *options,
                           const hil_logger_t *logger)
{
    if (options == NULL) {
        return HIL_ERR_NULL;
    }
    return hil_data_logger_query(options->csv_path, options->signal_name,
                                 options->from_ms, options->to_ms, logger);
}

void hil_cli_print_help(const hil_cli_options_t *options, FILE *stream)
{
    (void)options;
    (void)fprintf(stream,
                  "virtual_hil %s\n"
                  "run   : virtual_hil run --config <ini> --signals <csv> "
                  "--script <hil> --out <dir> [--lang zh|en]\n"
                  "query : virtual_hil query --csv <csv> --signal <name> "
                  "--from <ms> --to <ms>\n"
                  "version: virtual_hil version\n",
                  hil_version_string());
}
