#include "hil_data_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void format_number(double value, bool valid, char *buffer,
                          size_t buffer_size)
{
    if (!valid) {
        (void)snprintf(buffer, buffer_size, "NA");
    } else {
        (void)snprintf(buffer, buffer_size, "%.6g", value);
    }
}

hil_status_t hil_data_logger_open(hil_data_logger_t *logger,
                                  const char *path,
                                  hil_signal_registry_t *signals,
                                  const hil_logger_t *app_logger)
{
    size_t column_count;
    char **fields;
    size_t index;
    hil_status_t status;
    if (logger == NULL || path == NULL || signals == NULL) {
        return HIL_ERR_NULL;
    }
    (void)memset(logger, 0, sizeof(*logger));
    logger->signals = signals;
    if (strlen(path) >= sizeof(logger->path)) {
        return HIL_ERR_OVERFLOW;
    }
    (void)strncpy(logger->path, path, sizeof(logger->path) - 1U);
    column_count = hil_signal_registry_count(signals) + 1U;
    status = hil_csv_writer_open(&logger->writer, path, column_count,
                                 app_logger);
    if (status != HIL_OK) {
        return status;
    }
    fields = (char **)calloc(column_count, sizeof(*fields));
    if (fields == NULL) {
        (void)hil_csv_writer_close(&logger->writer, app_logger);
        return HIL_ERR_NOMEM;
    }
    fields[0] = "timestamp";
    for (index = 0U; index < signals->count; index++) {
        fields[index + 1U] = signals->items[index].name;
    }
    status = hil_csv_writer_write_row(&logger->writer,
                                      (const char *const *)fields,
                                      column_count, app_logger);
    free(fields);
    if (status != HIL_OK) {
        (void)hil_csv_writer_close(&logger->writer, app_logger);
        return status;
    }
    logger->open = true;
    return HIL_OK;
}

hil_status_t hil_data_logger_log_sample(hil_data_logger_t *logger,
                                        uint64_t timestamp_ms,
                                        const hil_logger_t *app_logger)
{
    size_t column_count;
    size_t index;
    char **fields;
    char (*storage)[64];
    char timestamp[32];
    hil_status_t status;
    if (logger == NULL || logger->signals == NULL || !logger->open) {
        return HIL_ERR_STATE;
    }
    column_count = hil_signal_registry_count(logger->signals) + 1U;
    if (column_count > SIZE_MAX / sizeof(*fields) ||
        column_count > SIZE_MAX / sizeof(*storage)) {
        return HIL_ERR_OVERFLOW;
    }
    fields = (char **)calloc(column_count, sizeof(*fields));
    storage = (char(*)[64])calloc(column_count, sizeof(*storage));
    if (fields == NULL || storage == NULL) {
        free(fields);
        free(storage);
        return HIL_ERR_NOMEM;
    }
    (void)snprintf(timestamp, sizeof(timestamp), "%llu",
                   (unsigned long long)timestamp_ms);
    (void)strncpy(storage[0], timestamp, sizeof(storage[0]) - 1U);
    fields[0] = storage[0];
    for (index = 0U; index < logger->signals->count; index++) {
        format_number(logger->signals->items[index].value,
                      logger->signals->items[index].valid,
                      storage[index + 1U], sizeof(storage[index + 1U]));
        fields[index + 1U] = storage[index + 1U];
    }
    status = hil_csv_writer_write_row(&logger->writer,
                                      (const char *const *)fields,
                                      column_count, app_logger);
    free(fields);
    free(storage);
    return status;
}

hil_status_t hil_data_logger_close(hil_data_logger_t *logger,
                                   const hil_logger_t *app_logger)
{
    hil_status_t status;
    if (logger == NULL) {
        return HIL_ERR_NULL;
    }
    if (!logger->open) {
        return HIL_OK;
    }
    status = hil_csv_writer_close(&logger->writer, app_logger);
    logger->open = false;
    return status;
}

static int csv_find_column(const hil_csv_reader_t *reader, const char *name)
{
    size_t index;
    for (index = 0U; index < hil_csv_reader_field_count(reader); index++) {
        if (strcmp(hil_csv_reader_field(reader, index), name) == 0) {
            return (int)index;
        }
    }
    return -1;
}

hil_status_t hil_data_logger_query(const char *path, const char *signal_name,
                                   uint64_t from_ms, uint64_t to_ms,
                                   const hil_logger_t *logger)
{
    hil_csv_reader_t reader;
    hil_status_t status;
    int time_index;
    int signal_index;
    size_t matched = 0U;
    if (path == NULL || signal_name == NULL) {
        return HIL_ERR_NULL;
    }
    status = hil_csv_reader_open(&reader, path, logger);
    if (status != HIL_OK) {
        return status;
    }
    status = hil_csv_reader_read_row(&reader, logger);
    if (status != HIL_OK) {
        hil_csv_reader_close(&reader);
        return status == HIL_EOF ? HIL_ERR_FORMAT : status;
    }
    time_index = csv_find_column(&reader, "timestamp");
    signal_index = csv_find_column(&reader, signal_name);
    if (time_index < 0 || signal_index < 0) {
        hil_csv_reader_close(&reader);
        return HIL_ERR_NOT_FOUND;
    }
    (void)printf("timestamp_ms,%s\n", signal_name);
    while ((status = hil_csv_reader_read_row(&reader, logger)) == HIL_OK) {
        const char *time_text =
            hil_csv_reader_field(&reader, (size_t)time_index);
        const char *value_text =
            hil_csv_reader_field(&reader, (size_t)signal_index);
        uint64_t timestamp;
        double value;
        if (time_text == NULL || value_text == NULL ||
            !hil_parse_u64(time_text, &timestamp) ||
            strcmp(value_text, "NA") == 0 ||
            !hil_parse_double(value_text, &value)) {
            continue;
        }
        if (timestamp < from_ms || timestamp > to_ms) {
            continue;
        }
        (void)printf("%llu,%.6g\n", (unsigned long long)timestamp, value);
        matched++;
    }
    hil_csv_reader_close(&reader);
    if (status != HIL_EOF) {
        return status;
    }
    if (matched == 0U) {
        hil_log_message(logger, HIL_LOG_WARN, "data",
                        "no records matched the query");
        return HIL_ERR_NOT_FOUND;
    }
    return HIL_OK;
}
