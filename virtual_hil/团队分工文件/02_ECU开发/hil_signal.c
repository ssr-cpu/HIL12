#include "hil_signal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hil_csv.h"

void hil_signal_init(hil_signal_t *signal, const char *name, const char *unit,
                     double min_value, double max_value, double initial_value)
{
    if (signal == NULL) {
        return;
    }
    (void)memset(signal, 0, sizeof(*signal));
    if (name != NULL) {
        (void)strncpy(signal->name, name, sizeof(signal->name) - 1U);
    } else {
        signal->name[0] = '\0';
    }
    if (unit != NULL) {
        (void)strncpy(signal->unit, unit, sizeof(signal->unit) - 1U);
    } else {
        signal->unit[0] = '\0';
    }
    signal->min_value = min_value;
    signal->max_value = max_value;
    signal->initial_value = initial_value;
    signal->value = initial_value;
    signal->raw_value = initial_value;
    signal->valid = true;
    signal->open_circuit = false;
}

hil_status_t hil_signal_clamp_value(const hil_signal_t *signal, double value,
                                    double *clamped)
{
    if (signal == NULL || clamped == NULL) {
        return HIL_ERR_NULL;
    }
    if (!hil_double_is_finite(value)) {
        return HIL_ERR_VALUE;
    }
    if (signal->max_value < signal->min_value) {
        return HIL_ERR_RANGE;
    }
    if (value < signal->min_value) {
        *clamped = signal->min_value;
    } else if (value > signal->max_value) {
        *clamped = signal->max_value;
    } else {
        *clamped = value;
    }
    return HIL_OK;
}

void hil_signal_registry_init(hil_signal_registry_t *registry)
{
    if (registry != NULL) {
        registry->items = NULL;
        registry->count = 0U;
        registry->capacity = 0U;
    }
}

void hil_signal_registry_deinit(hil_signal_registry_t *registry)
{
    if (registry != NULL) {
        free(registry->items);
        registry->items = NULL;
        registry->count = 0U;
        registry->capacity = 0U;
    }
}

static hil_status_t ensure_capacity(hil_signal_registry_t *registry,
                                    size_t required)
{
    hil_signal_t *new_items;
    size_t new_capacity;
    if (registry == NULL) {
        return HIL_ERR_NULL;
    }
    if (required <= registry->capacity) {
        return HIL_OK;
    }
    new_capacity = registry->capacity == 0U ? 8U : registry->capacity;
    while (new_capacity < required) {
        if (new_capacity > (SIZE_MAX / 2U)) {
            return HIL_ERR_OVERFLOW;
        }
        new_capacity *= 2U;
    }
    if (new_capacity > SIZE_MAX / sizeof(*new_items)) {
        return HIL_ERR_OVERFLOW;
    }
    new_items = (hil_signal_t *)realloc(registry->items,
                                        new_capacity * sizeof(*new_items));
    if (new_items == NULL) {
        return HIL_ERR_NOMEM;
    }
    registry->items = new_items;
    registry->capacity = new_capacity;
    return HIL_OK;
}

hil_status_t hil_signal_registry_add(hil_signal_registry_t *registry,
                                     const hil_signal_t *signal,
                                     const hil_logger_t *logger)
{
    hil_status_t status;
    if (registry == NULL || signal == NULL || signal->name[0] == '\0') {
        return HIL_ERR_NULL;
    }
    if (hil_signal_registry_find(registry, signal->name) != NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "signal",
                        "duplicate signal name: %s", signal->name);
        return HIL_ERR_DUPLICATE;
    }
    status = ensure_capacity(registry, registry->count + 1U);
    if (status != HIL_OK) {
        return status;
    }
    registry->items[registry->count] = *signal;
    registry->count++;
    return HIL_OK;
}

hil_signal_t *hil_signal_registry_find(hil_signal_registry_t *registry,
                                       const char *name)
{
    size_t index;
    if (registry == NULL || name == NULL) {
        return NULL;
    }
    for (index = 0U; index < registry->count; index++) {
        if (strcmp(registry->items[index].name, name) == 0) {
            return &registry->items[index];
        }
    }
    return NULL;
}

const hil_signal_t *hil_signal_registry_find_const(
    const hil_signal_registry_t *registry, const char *name)
{
    size_t index;
    if (registry == NULL || name == NULL) {
        return NULL;
    }
    for (index = 0U; index < registry->count; index++) {
        if (strcmp(registry->items[index].name, name) == 0) {
            return &registry->items[index];
        }
    }
    return NULL;
}

hil_status_t hil_signal_registry_set(hil_signal_registry_t *registry,
                                     const char *name, double value,
                                     const hil_logger_t *logger)
{
    hil_signal_t *signal = hil_signal_registry_find(registry, name);
    if (signal == NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "signal",
                        "unknown signal: %s", name);
        return HIL_ERR_NOT_FOUND;
    }
    if (!hil_double_is_finite(value)) {
        hil_log_message(logger, HIL_LOG_ERROR, "signal",
                        "signal '%s' got non-finite value", name);
        return HIL_ERR_VALUE;
    }
    signal->value = value;
    signal->raw_value = value;
    return HIL_OK;
}
hil_status_t hil_signal_registry_set_valid(hil_signal_registry_t *registry,
                                           const char *name, bool valid)
{
    hil_signal_t *signal = hil_signal_registry_find(registry, name);
    if (signal == NULL) {
        return HIL_ERR_NOT_FOUND;
    }
    signal->valid = valid;
    if (!valid) {
        signal->open_circuit = true;
    }
    return HIL_OK;
}

size_t hil_signal_registry_count(const hil_signal_registry_t *registry)
{
    return registry != NULL ? registry->count : 0U;
}

hil_status_t hil_signal_registry_reset(hil_signal_registry_t *registry)
{
    size_t index;
    if (registry == NULL) {
        return HIL_ERR_NULL;
    }
    for (index = 0U; index < registry->count; index++) {
        registry->items[index].value = registry->items[index].initial_value;
        registry->items[index].raw_value = registry->items[index].initial_value;
        registry->items[index].valid = true;
        registry->items[index].open_circuit = false;
    }
    return HIL_OK;
}

static int find_header_index(const hil_csv_reader_t *reader, const char *name)
{
    size_t index;
    for (index = 0U; index < hil_csv_reader_field_count(reader); index++) {
        if (strcmp(hil_csv_reader_field(reader, index), name) == 0) {
            return (int)index;
        }
    }
    return -1;
}

hil_status_t hil_signal_registry_load_csv(hil_signal_registry_t *registry,
                                          const char *path,
                                          const hil_logger_t *logger)
{
    hil_csv_reader_t reader;
    hil_status_t status;
    int name_index;
    int unit_index;
    int min_index;
    int max_index;
    int initial_index;
    size_t row_count = 0U;

    if (registry == NULL || path == NULL) {
        return HIL_ERR_NULL;
    }
    status = hil_csv_reader_open(&reader, path, logger);
    if (status != HIL_OK) {
        return status;
    }
    status = hil_csv_reader_read_row(&reader, logger);
    if (status == HIL_EOF) {
        hil_csv_reader_close(&reader);
        return HIL_ERR_FORMAT;
    }
    if (status != HIL_OK) {
        hil_csv_reader_close(&reader);
        return status;
    }
    name_index = find_header_index(&reader, "name");
    unit_index = find_header_index(&reader, "unit");
    min_index = find_header_index(&reader, "min");
    max_index = find_header_index(&reader, "max");
    initial_index = find_header_index(&reader, "initial");
    if (name_index < 0 || unit_index < 0 || min_index < 0 ||
        max_index < 0 || initial_index < 0) {
        hil_log_message(logger, HIL_LOG_ERROR, "signal",
                        "signals CSV header is missing required columns");
        hil_csv_reader_close(&reader);
        return HIL_ERR_FORMAT;
    }

    while ((status = hil_csv_reader_read_row(&reader, logger)) == HIL_OK) {
        const char *name = hil_csv_reader_field(&reader, (size_t)name_index);
        const char *unit = hil_csv_reader_field(&reader, (size_t)unit_index);
        const char *min_text = hil_csv_reader_field(&reader, (size_t)min_index);
        const char *max_text = hil_csv_reader_field(&reader, (size_t)max_index);
        const char *initial_text =
            hil_csv_reader_field(&reader, (size_t)initial_index);
        hil_signal_t signal;
        double min_value;
        double max_value;
        double initial_value;
        if (name == NULL || unit == NULL || min_text == NULL ||
            max_text == NULL || initial_text == NULL ||
            !hil_parse_double(min_text, &min_value) ||
            !hil_parse_double(max_text, &max_value) ||
            !hil_parse_double(initial_text, &initial_value)) {
            hil_log_message(logger, HIL_LOG_ERROR, "signal",
                            "signals CSV row %llu is invalid",
                            (unsigned long long)hil_csv_reader_line_number(
                                &reader));
            hil_csv_reader_close(&reader);
            return HIL_ERR_FORMAT;
        }
        if (min_value > max_value || initial_value < min_value ||
            initial_value > max_value) {
            hil_log_message(logger, HIL_LOG_ERROR, "signal",
                            "signal '%s' has invalid range", name);
            hil_csv_reader_close(&reader);
            return HIL_ERR_RANGE;
        }
        hil_signal_init(&signal, name, unit, min_value, max_value,
                        initial_value);
        status = hil_signal_registry_add(registry, &signal, logger);
        if (status != HIL_OK) {
            hil_csv_reader_close(&reader);
            return status;
        }
        row_count++;
    }
    hil_csv_reader_close(&reader);
    if (status != HIL_EOF) {
        return status;
    }
    if (row_count < 8U) {
        hil_log_message(logger, HIL_LOG_ERROR, "signal",
                        "at least 8 signals are required, found %llu",
                        (unsigned long long)row_count);
        return HIL_ERR_CONFIG;
    }
    return HIL_OK;
}
