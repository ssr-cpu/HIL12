#include "hil_fault.h"

#include <math.h>
#include <stdlib.h>
#include <string.h> 

static const char *const fault_type_names[] = {
    [HIL_FAULT_NONE] = "NONE",
    [HIL_FAULT_OFFSET] = "OFFSET",
    [HIL_FAULT_STUCK] = "STUCK",
    [HIL_FAULT_NOISE] = "NOISE",
    [HIL_FAULT_DROPOUT] = "DROPOUT",
    [HIL_FAULT_OPEN_CIRCUIT] = "OPEN_CIRCUIT"
};

const char *hil_fault_type_name(hil_fault_type_t type)
{
    if (type < HIL_FAULT_NONE || type > HIL_FAULT_OPEN_CIRCUIT) {
        return "UNKNOWN";
    }
    return fault_type_names[type];
}

bool hil_fault_type_from_name(const char *name, hil_fault_type_t *type)
{
    size_t index;
    if (name == NULL || type == NULL) {
        return false;
    }
    for (index = 0U; index < HIL_ARRAY_LEN(fault_type_names); index++) {
        if (strcmp(name, fault_type_names[index]) == 0) {
            *type = (hil_fault_type_t)index;
            return true;
        }
    }
    return false;
}

static uint64_t fault_random_next(uint64_t *state)
{
    uint64_t x = *state;
    x ^= x << 13U;
    x ^= x >> 7U;
    x ^= x << 17U;
    *state = x;
    return x;
}

void hil_fault_manager_init(hil_fault_manager_t *manager, uint64_t seed)
{
    if (manager == NULL) {
        return;
    }
    (void)memset(manager, 0, sizeof(*manager));
    manager->random_state = seed == 0U ? 1U : seed;
}

void hil_fault_manager_deinit(hil_fault_manager_t *manager)
{
    if (manager == NULL) {
        return;
    }
    free(manager->items);
    manager->items = NULL;
    manager->count = 0U;
    manager->capacity = 0U;
}

static hil_status_t fault_reserve(hil_fault_manager_t *manager, size_t required)
{
    hil_fault_spec_t *new_items;
    size_t new_capacity;
    if (required <= manager->capacity) {
        return HIL_OK;
    }
    new_capacity = manager->capacity == 0U ? 4U : manager->capacity;
    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / 2U) {
            return HIL_ERR_OVERFLOW;
        }
        new_capacity *= 2U;
    }
    if (new_capacity > SIZE_MAX / sizeof(*new_items)) {
        return HIL_ERR_OVERFLOW;
    }
    new_items = (hil_fault_spec_t *)realloc(
        manager->items, new_capacity * sizeof(*new_items));
    if (new_items == NULL) {
        return HIL_ERR_NOMEM;
    }
    manager->items = new_items;
    manager->capacity = new_capacity;
    return HIL_OK;
}

hil_status_t hil_fault_manager_add(hil_fault_manager_t *manager,
                                   const hil_fault_spec_t *spec,
                                   const hil_logger_t *logger)
{
    hil_status_t status;
    if (manager == NULL || spec == NULL || spec->signal_name[0] == '\0') {
        return HIL_ERR_NULL;
    }
    if (spec->end_ms != 0U && spec->end_ms < spec->start_ms) {
        hil_log_message(logger, HIL_LOG_ERROR, "fault",
                        "fault window is reversed for %s",
                        spec->signal_name);
        return HIL_ERR_RANGE;
    }
    status = fault_reserve(manager, manager->count + 1U);
    if (status != HIL_OK) {
        return status;
    }
    manager->items[manager->count++] = *spec;
    manager->items[manager->count - 1U].active = false;
    return HIL_OK;
}

hil_status_t hil_fault_manager_remove(hil_fault_manager_t *manager,
                                      const char *signal_name,
                                      hil_fault_type_t type)
{
    size_t index;
    if (manager == NULL || signal_name == NULL) {
        return HIL_ERR_NULL;
    }
    for (index = 0U; index < manager->count; index++) {
        hil_fault_spec_t *spec = &manager->items[index];
        if (spec->type == type &&
            strcmp(spec->signal_name, signal_name) == 0) {
            if (index + 1U < manager->count) {
                (void)memmove(&manager->items[index],
                              &manager->items[index + 1U],
                              (manager->count - index - 1U) *
                                  sizeof(*manager->items));
            }
            manager->count--;
            return HIL_OK;
        }
    }
    return HIL_ERR_NOT_FOUND;
}

hil_status_t hil_fault_manager_clear(hil_fault_manager_t *manager)
{
    if (manager == NULL) {
        return HIL_ERR_NULL;
    }
    manager->count = 0U;
    return HIL_OK;
}

static double clamp_to_signal(const hil_signal_t *signal, double value)
{
    if (value < signal->min_value) {
        return signal->min_value;
    }
    if (value > signal->max_value) {
        return signal->max_value;
    }
    return value;
}

hil_status_t hil_fault_manager_apply(hil_fault_manager_t *manager,
                                     hil_signal_registry_t *registry,
                                     uint64_t now_ms,
                                     const hil_logger_t *logger)
{
    size_t signal_index;
    size_t fault_index;
    if (manager == NULL || registry == NULL) {
        return HIL_ERR_NULL;
    }
    for (signal_index = 0U; signal_index < registry->count; signal_index++) {
        hil_signal_t *signal = &registry->items[signal_index];
        signal->value = signal->raw_value;
        signal->valid = true;
        signal->open_circuit = false;
    }
    for (fault_index = 0U; fault_index < manager->count; fault_index++) {
        hil_fault_spec_t *spec = &manager->items[fault_index];
        hil_signal_t *signal;
        double noise;
        if (now_ms < spec->start_ms ||
            (spec->end_ms != 0U && now_ms > spec->end_ms)) {
            spec->active = false;
            continue;
        }
        spec->active = true;
        signal = hil_signal_registry_find(registry, spec->signal_name);
        if (signal == NULL) {
            hil_log_message(logger, HIL_LOG_WARN, "fault",
                            "fault targets unknown signal: %s",
                            spec->signal_name);
            continue;
        }
        switch (spec->type) {
        case HIL_FAULT_OFFSET:
            signal->value =
                clamp_to_signal(signal, signal->value + spec->value);
            break;
        case HIL_FAULT_STUCK:
            signal->value = clamp_to_signal(signal, spec->value);
            break;
        case HIL_FAULT_NOISE:
            noise = (double)(fault_random_next(&manager->random_state) % 2001U);
            noise = (noise / 1000.0 - 1.0) * spec->value;
            signal->value = clamp_to_signal(signal, signal->value + noise);
            break;
        case HIL_FAULT_DROPOUT:
            signal->value = 0.0;
            signal->valid = false;
            break;
        case HIL_FAULT_OPEN_CIRCUIT:
            signal->value = 0.0;
            signal->valid = false;
            signal->open_circuit = true;
            break;
        case HIL_FAULT_NONE:
            break;
        }
    }
    return HIL_OK;
}

bool hil_fault_manager_is_active(const hil_fault_manager_t *manager,
                                 const char *signal_name, uint64_t now_ms)
{
    size_t index;
    if (manager == NULL || signal_name == NULL) {
        return false;
    }
    for (index = 0U; index < manager->count; index++) {
        const hil_fault_spec_t *spec = &manager->items[index];
        if (strcmp(spec->signal_name, signal_name) == 0 &&
            now_ms >= spec->start_ms &&
            (spec->end_ms == 0U || now_ms <= spec->end_ms)) {
            return true;
        }
    }
    return false;
}
