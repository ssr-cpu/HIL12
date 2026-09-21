#ifndef HIL_FAULT_H
#define HIL_FAULT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hil_common.h"
#include "hil_logger.h"
#include "hil_signal.h"

typedef enum hil_fault_type {
    HIL_FAULT_NONE = 0,
    HIL_FAULT_OFFSET,
    HIL_FAULT_STUCK,
    HIL_FAULT_NOISE,
    HIL_FAULT_DROPOUT,
    HIL_FAULT_OPEN_CIRCUIT
} hil_fault_type_t;

typedef struct hil_fault_spec {
    char signal_name[HIL_NAME_MAX];
    hil_fault_type_t type;
    double value;
    uint64_t start_ms;
    uint64_t end_ms;
    bool active;
} hil_fault_spec_t;

typedef struct hil_fault_manager {
    hil_fault_spec_t *items;
    size_t count;
    size_t capacity;
    uint64_t random_state;
} hil_fault_manager_t;

const char *hil_fault_type_name(hil_fault_type_t type);
bool hil_fault_type_from_name(const char *name, hil_fault_type_t *type);

void hil_fault_manager_init(hil_fault_manager_t *manager, uint64_t seed);
void hil_fault_manager_deinit(hil_fault_manager_t *manager);
hil_status_t hil_fault_manager_add(hil_fault_manager_t *manager,
                                   const hil_fault_spec_t *spec,
                                   const hil_logger_t *logger);
hil_status_t hil_fault_manager_remove(hil_fault_manager_t *manager,
                                      const char *signal_name,
                                      hil_fault_type_t type);
hil_status_t hil_fault_manager_clear(hil_fault_manager_t *manager);
hil_status_t hil_fault_manager_apply(hil_fault_manager_t *manager,
                                     hil_signal_registry_t *registry,
                                     uint64_t now_ms,
                                     const hil_logger_t *logger);
bool hil_fault_manager_is_active(const hil_fault_manager_t *manager,
                                 const char *signal_name, uint64_t now_ms);

#endif
