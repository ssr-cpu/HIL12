#include "hil_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hil_config_init(hil_config_t *config)
{
    if (config == NULL) {
        return;
    }
    (void)memset(config, 0, sizeof(*config));
    config->tick_ms = 10U;
    config->duration_ms = 3000U;
    config->ecu_power_on_delay_ms = 80U;
    config->ecu_comm_timeout_ms = 500U;
    config->ecu_recovery_delay_ms = 250U;
    config->bus_queue_capacity = 64U;
    config->bus_loss_probability_percent = 0U;
    config->bus_delay_ms = 0U;
    config->bus_tamper_enabled = false;
    config->random_seed = 20260919U;
    config->stop_on_failure = false;
    config->verbose = false;
    (void)strncpy(config->output_dir, "out", sizeof(config->output_dir) - 1U);
    (void)strncpy(config->log_file, "run.csv", sizeof(config->log_file) - 1U);
    (void)strncpy(config->report_file, "report.md",
                  sizeof(config->report_file) - 1U);
}


hil_status_t hil_config_set_default_paths(hil_config_t *config,
                                         const char *output_dir)
{
    if (config == NULL || output_dir == NULL || output_dir[0] == '\0') {
        return HIL_ERR_NULL;
    }
    if (strlen(output_dir) >= sizeof(config->output_dir)) {
        return HIL_ERR_OVERFLOW;
    }
    (void)strncpy(config->output_dir, output_dir, sizeof(config->output_dir) - 1U);
    config->output_dir[sizeof(config->output_dir) - 1U] = '\0';
    return HIL_OK;
}

static char *trim_line(char *text)
{
    char *end;
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        text++;
    }
    if (*text == '\0') {
        return text;
    }
    end = text + strlen(text) - 1U;
    while (end > text &&
           (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return text;
}

static hil_status_t parse_key_value(char *line, char **key, char **value)
{
    char *eq = strchr(line, '=');
    if (eq == NULL) {
        return HIL_ERR_FORMAT;
    }
    *eq = '\0';
    *key = trim_line(line);
    *value = trim_line(eq + 1);
    if (**key == '\0' || **value == '\0') {
        return HIL_ERR_FORMAT;
    }
    return HIL_OK;
}

static hil_status_t apply_value(hil_config_t *config, const char *key,
                                const char *value)
{
    uint64_t u64 = 0U;
    bool boolean = false;
    unsigned long parsed_uint;
    char *end = NULL;

    if (strcmp(key, "runtime.tick_ms") == 0) {
        if (!hil_parse_u64(value, &u64) || u64 == 0U) {
            return HIL_ERR_RANGE;
        }
        config->tick_ms = u64;
        return HIL_OK;
    }
    if (strcmp(key, "runtime.duration_ms") == 0) {
        if (!hil_parse_u64(value, &u64)) {
            return HIL_ERR_RANGE;
        }
        config->duration_ms = u64;
        return HIL_OK;
    }
    if (strcmp(key, "runtime.random_seed") == 0) {
        if (!hil_parse_u64(value, &u64)) {
            return HIL_ERR_RANGE;
        }
        config->random_seed = (unsigned int)(u64 % 4294967295ULL);
        if (config->random_seed == 0U) {
            config->random_seed = 1U;
        }
        return HIL_OK;
    }
    if (strcmp(key, "runtime.stop_on_failure") == 0) {
        if (!hil_parse_bool(value, &boolean)) {
            return HIL_ERR_FORMAT;
        }
        config->stop_on_failure = boolean;
        return HIL_OK;
    }
    if (strcmp(key, "runtime.verbose") == 0) {
        if (!hil_parse_bool(value, &boolean)) {
            return HIL_ERR_FORMAT;
        }
        config->verbose = boolean;
        return HIL_OK;
    }
    if (strcmp(key, "ecu.power_on_delay_ms") == 0) {
        if (!hil_parse_u64(value, &u64)) {
            return HIL_ERR_RANGE;
        }
        config->ecu_power_on_delay_ms = u64;
        return HIL_OK;
    }
    if (strcmp(key, "ecu.comm_timeout_ms") == 0) {
        if (!hil_parse_u64(value, &u64)) {
            return HIL_ERR_RANGE;
        }
        config->ecu_comm_timeout_ms = u64;
        return HIL_OK;
    }
    if (strcmp(key, "ecu.recovery_delay_ms") == 0) {
        if (!hil_parse_u64(value, &u64)) {
            return HIL_ERR_RANGE;
        }
        config->ecu_recovery_delay_ms = u64;
        return HIL_OK;
    }
    if (strcmp(key, "bus.queue_capacity") == 0) {
        if (!hil_parse_u64(value, &u64) || u64 == 0U) {
            return HIL_ERR_RANGE;
        }
        config->bus_queue_capacity = (size_t)u64;
        return HIL_OK;
    }
    if (strcmp(key, "bus.loss_probability_percent") == 0) {
        parsed_uint = strtoul(value, &end, 10);
        if (end == value || *end != '\0' || parsed_uint > 100UL) {
            return HIL_ERR_RANGE;
        }
        config->bus_loss_probability_percent = (unsigned int)parsed_uint;
        return HIL_OK;
    }
    if (strcmp(key, "bus.delay_ms") == 0) {
        if (!hil_parse_u64(value, &u64)) {
            return HIL_ERR_RANGE;
        }
        config->bus_delay_ms = u64;
        return HIL_OK;
    }
    if (strcmp(key, "bus.tamper_enabled") == 0) {
        if (!hil_parse_bool(value, &boolean)) {
            return HIL_ERR_FORMAT;
        }
        config->bus_tamper_enabled = boolean;
        return HIL_OK;
    }
    if (strcmp(key, "data.log_file") == 0) {
        if (strlen(value) >= sizeof(config->log_file)) {
            return HIL_ERR_OVERFLOW;
        }
        (void)strncpy(config->log_file, value, sizeof(config->log_file) - 1U);
        config->log_file[sizeof(config->log_file) - 1U] = '\0';
        return HIL_OK;
    }
    if (strcmp(key, "data.report_file") == 0) {
        if (strlen(value) >= sizeof(config->report_file)) {
            return HIL_ERR_OVERFLOW;
        }
        (void)strncpy(config->report_file, value,
                      sizeof(config->report_file) - 1U);
        config->report_file[sizeof(config->report_file) - 1U] = '\0';
        return HIL_OK;
    }
    return HIL_ERR_CONFIG;
}

hil_status_t hil_config_load(hil_config_t *config, const char *path,
                             const hil_logger_t *logger)
{
    FILE *fp;
    char line[HIL_LINE_MAX];
    size_t line_number = 0U;
    hil_status_t status = HIL_OK;

    if (config == NULL || path == NULL) {
        return HIL_ERR_NULL;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "config",
                        "cannot open config file: %s", path);
        return HIL_ERR_IO;
    }
    while (fgets(line, (int)sizeof(line), fp) != NULL) {
        char *text;
        char *key;
        char *value;
        line_number++;
        text = trim_line(line);
        if (*text == '\0' || *text == '#' || *text == ';') {
            continue;
        }
        if (parse_key_value(text, &key, &value) != HIL_OK) {
            hil_log_message(logger, HIL_LOG_ERROR, "config",
                            "%s:%llu: invalid key=value line", path,
                            (unsigned long long)line_number);
            status = HIL_ERR_FORMAT;
            break;
        }
        status = apply_value(config, key, value);
        if (status != HIL_OK) {
            hil_log_message(logger, HIL_LOG_ERROR, "config",
                            "%s:%llu: invalid value for '%s'", path,
                            (unsigned long long)line_number, key);
            break;
        }
    }
    (void)fclose(fp);
    return status;
}
