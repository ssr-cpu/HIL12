#ifndef HIL_SIGNAL_H
#define HIL_SIGNAL_H

#include <stdbool.h>
#include <stddef.h>

#include "hil_common.h"
#include "hil_logger.h"

typedef struct hil_signal {
    char name[HIL_NAME_MAX];
    char unit[24];
    double min_value;
    double max_value;
    double initial_value;
    double value;
    double raw_value;
    bool valid;
    bool open_circuit;
} hil_signal_t;

typedef struct hil_signal_registry {
    hil_signal_t *items;
    size_t count;
    size_t capacity;
} hil_signal_registry_t;

void hil_signal_init(hil_signal_t *signal, const char *name, const char *unit,
                     double min_value, double max_value, double initial_value);
hil_status_t hil_signal_clamp_value(const hil_signal_t *signal, double value,
                                    double *clamped);

void hil_signal_registry_init(hil_signal_registry_t *registry);
void hil_signal_registry_deinit(hil_signal_registry_t *registry);
hil_status_t hil_signal_registry_add(hil_signal_registry_t *registry,
                                     const hil_signal_t *signal,
                                     const hil_logger_t *logger);
hil_signal_t *hil_signal_registry_find(hil_signal_registry_t *registry,
                                       const char *name);
const hil_signal_t *hil_signal_registry_find_const(
    const hil_signal_registry_t *registry, const char *name);
hil_status_t hil_signal_registry_set(hil_signal_registry_t *registry,
                                     const char *name, double value,
                                     const hil_logger_t *logger);
hil_status_t hil_signal_registry_set_valid(hil_signal_registry_t *registry,
                                           const char *name, bool valid);
size_t hil_signal_registry_count(const hil_signal_registry_t *registry);
hil_status_t hil_signal_registry_load_csv(hil_signal_registry_t *registry,
                                          const char *path,
                                          const hil_logger_t *logger);
hil_status_t hil_signal_registry_reset(hil_signal_registry_t *registry);

#endif
