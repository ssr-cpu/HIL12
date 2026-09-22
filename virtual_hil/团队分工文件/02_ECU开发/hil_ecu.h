#ifndef HIL_ECU_H
#define HIL_ECU_H

#include <stdbool.h>
#include <stdint.h>

#include "hil_common.h"
#include "hil_logger.h"
#include "hil_signal.h"
#include "hil_time.h"

typedef enum hil_ecu_state {
    HIL_ECU_POWER_OFF = 0,
    HIL_ECU_INIT,
    HIL_ECU_SELF_TEST,
    HIL_ECU_STANDBY,
    HIL_ECU_RUN,
    HIL_ECU_FAULT,
    HIL_ECU_RECOVERY
} hil_ecu_state_t;

typedef struct hil_ecu {
    hil_ecu_state_t state;
    uint64_t state_since_ms;
    uint64_t last_periodic_ms;
    uint64_t last_tx_ms;
    uint64_t power_on_delay_ms;
    uint64_t comm_timeout_ms;
    uint64_t recovery_delay_ms;
    uint64_t periodic_period_ms;
    bool power_requested;
    bool comm_fault;
    bool fault_latched;
    unsigned int recovery_attempts;
} hil_ecu_t;

const char *hil_ecu_state_name(hil_ecu_state_t state);
bool hil_ecu_state_from_name(const char *name, hil_ecu_state_t *state);

void hil_ecu_init(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                  uint64_t power_on_delay_ms, uint64_t comm_timeout_ms,
                  uint64_t recovery_delay_ms, uint64_t periodic_period_ms);
hil_status_t hil_ecu_power_on(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                              const hil_logger_t *logger);
hil_status_t hil_ecu_power_off(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                               const hil_logger_t *logger);
hil_status_t hil_ecu_tick(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                          hil_signal_registry_t *signals,
                          const hil_logger_t *logger);
hil_status_t hil_ecu_set_comm_fault(hil_ecu_t *ecu, bool present,
                                    const hil_virtual_time_t *time,
                                    const hil_logger_t *logger);
hil_status_t hil_ecu_latch_fault(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                                 const hil_logger_t *logger);
hil_status_t hil_ecu_clear_fault(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                                 const hil_logger_t *logger);
hil_status_t hil_ecu_reset(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                           const hil_logger_t *logger);
bool hil_ecu_is_running(const hil_ecu_t *ecu);
bool hil_ecu_has_safe_outputs(const hil_ecu_t *ecu);
hil_ecu_state_t hil_ecu_get_state(const hil_ecu_t *ecu);
hil_status_t hil_ecu_run_periodic_tasks(hil_ecu_t *ecu,
                                        hil_signal_registry_t *signals,
                                        const hil_logger_t *logger);
void hil_ecu_apply_safe_outputs(hil_ecu_t *ecu,
                                hil_signal_registry_t *signals,
                                const hil_logger_t *logger);

#endif
