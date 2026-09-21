#include "hil_ecu.h"

#include <string.h>

static const char *const state_names[] = {
    [HIL_ECU_POWER_OFF] = "POWER_OFF",
    [HIL_ECU_INIT] = "INIT",
    [HIL_ECU_SELF_TEST] = "SELF_TEST",
    [HIL_ECU_STANDBY] = "STANDBY",
    [HIL_ECU_RUN] = "RUN",
    [HIL_ECU_FAULT] = "FAULT",
    [HIL_ECU_RECOVERY] = "RECOVERY"
};

const char *hil_ecu_state_name(hil_ecu_state_t state)
{
    if (state < HIL_ECU_POWER_OFF || state > HIL_ECU_RECOVERY) {
        return "UNKNOWN";
    }
    return state_names[state];
}

void hil_ecu_init(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                  uint64_t power_on_delay_ms, uint64_t comm_timeout_ms,
                  uint64_t recovery_delay_ms, uint64_t periodic_period_ms)
{
    uint64_t now;
    if (ecu == NULL) {
        return;
    }
    now = hil_time_now(time);
    (void)memset(ecu, 0, sizeof(*ecu));
    ecu->state = HIL_ECU_POWER_OFF;
    ecu->state_since_ms = now;
    ecu->last_periodic_ms = now;
    ecu->last_tx_ms = now;
    ecu->power_on_delay_ms = power_on_delay_ms;
    ecu->comm_timeout_ms = comm_timeout_ms;
    ecu->recovery_delay_ms = recovery_delay_ms;
    ecu->periodic_period_ms = periodic_period_ms;
    ecu->power_requested = false;
}

static hil_status_t change_state(hil_ecu_t *ecu, hil_ecu_state_t new_state,
                                 const hil_virtual_time_t *time,
                                 const hil_logger_t *logger)
{
    if (ecu == NULL || time == NULL) {
        return HIL_ERR_NULL;
    }
    hil_log_message(logger, HIL_LOG_INFO, "ecu", "%s -> %s at %llu ms",
                    hil_ecu_state_name(ecu->state),
                    hil_ecu_state_name(new_state),
                    (unsigned long long)hil_time_now(time));
    ecu->state = new_state;
    ecu->state_since_ms = hil_time_now(time);
    return HIL_OK;
}

hil_status_t hil_ecu_power_on(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                              const hil_logger_t *logger)
{
    if (ecu == NULL || time == NULL) {
        return HIL_ERR_NULL;
    }
    if (ecu->power_requested && ecu->state != HIL_ECU_POWER_OFF) {
        return HIL_OK;
    }
    ecu->power_requested = true;
    ecu->comm_fault = false;
    ecu->fault_latched = false;
    ecu->recovery_attempts = 0U;
    return change_state(ecu, HIL_ECU_INIT, time, logger);
}

hil_status_t hil_ecu_power_off(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                               const hil_logger_t *logger)
{
    if (ecu == NULL || time == NULL) {
        return HIL_ERR_NULL;
    }
    ecu->power_requested = false;
    ecu->comm_fault = false;
    ecu->fault_latched = false;
    ecu->recovery_attempts = 0U;
    return change_state(ecu, HIL_ECU_POWER_OFF, time, logger);
}

static hil_status_t transition_if_elapsed(hil_ecu_t *ecu,
                                          const hil_virtual_time_t *time,
                                          uint64_t delay_ms,
                                          hil_ecu_state_t new_state,
                                          const hil_logger_t *logger)
{
    if (hil_time_elapsed(time, ecu->state_since_ms) >= delay_ms) {
        return change_state(ecu, new_state, time, logger);
    }
    return HIL_OK;
}

hil_status_t hil_ecu_tick(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                          hil_signal_registry_t *signals,
                          const hil_logger_t *logger)
{
    hil_status_t status;
    if (ecu == NULL || time == NULL || signals == NULL) {
        return HIL_ERR_NULL;
    }
    if (!ecu->power_requested) {
        hil_ecu_apply_safe_outputs(ecu, signals, logger);
        return HIL_OK;
    }

    switch (ecu->state) {
    case HIL_ECU_INIT:
        status = transition_if_elapsed(ecu, time, ecu->power_on_delay_ms,
                                       HIL_ECU_SELF_TEST, logger);
        if (status != HIL_OK) {
            return status;
        }
        break;
    case HIL_ECU_SELF_TEST:
        status = transition_if_elapsed(ecu, time, 60U, HIL_ECU_STANDBY,
                                       logger);
        if (status != HIL_OK) {
            return status;
        }
        break;
    case HIL_ECU_STANDBY:
        if (ecu->comm_fault &&
            hil_time_elapsed(time, ecu->state_since_ms) >=
                ecu->comm_timeout_ms) {
            status = change_state(ecu, HIL_ECU_FAULT, time, logger);
            if (status != HIL_OK) {
                return status;
            }
        } else if (!ecu->comm_fault) {
            status = transition_if_elapsed(ecu, time, 100U, HIL_ECU_RUN,
                                           logger);
            if (status != HIL_OK) {
                return status;
            }
        }
        break;
    case HIL_ECU_RUN:
        if (ecu->comm_fault) {
            status = change_state(ecu, HIL_ECU_FAULT, time, logger);
            if (status != HIL_OK) {
                return status;
            }
        }
        break;
    case HIL_ECU_FAULT:
        if (!ecu->fault_latched &&
            hil_time_elapsed(time, ecu->state_since_ms) >=
                ecu->recovery_delay_ms) {
            ecu->recovery_attempts++;
            status = change_state(ecu, HIL_ECU_RECOVERY, time, logger);
            if (status != HIL_OK) {
                return status;
            }
        }
        break;
    case HIL_ECU_RECOVERY:
        status = transition_if_elapsed(ecu, time, 100U, HIL_ECU_STANDBY,
                                       logger);
        if (status != HIL_OK) {
            return status;
        }
        break;
    case HIL_ECU_POWER_OFF:
        break;
    }

    if (hil_ecu_is_running(ecu) &&
        hil_time_elapsed(time, ecu->last_periodic_ms) >=
            ecu->periodic_period_ms) {
        status = hil_ecu_run_periodic_tasks(ecu, signals, logger);
        if (status != HIL_OK) {
            return status;
        }
        ecu->last_periodic_ms = hil_time_now(time);
    }

    if (hil_ecu_has_safe_outputs(ecu)) {
        hil_ecu_apply_safe_outputs(ecu, signals, logger);
    }
    return HIL_OK;
}

hil_status_t hil_ecu_set_comm_fault(hil_ecu_t *ecu, bool present,
                                    const hil_virtual_time_t *time,
                                    const hil_logger_t *logger)
{
    if (ecu == NULL || time == NULL) {
        return HIL_ERR_NULL;
    }
    if (ecu->comm_fault == present) {
        return HIL_OK;
    }
    ecu->comm_fault = present;
    hil_log_message(logger, HIL_LOG_WARN, "ecu",
                    "communication fault %s",
                    present ? "active" : "cleared");
    if (present && (ecu->state == HIL_ECU_STANDBY ||
                    ecu->state == HIL_ECU_RUN)) {
        return change_state(ecu, HIL_ECU_FAULT, time, logger);
    }
    if (!present && ecu->state == HIL_ECU_FAULT && !ecu->fault_latched) {
        return change_state(ecu, HIL_ECU_RECOVERY, time, logger);
    }
    return HIL_OK;
}
hil_status_t hil_ecu_latch_fault(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                                 const hil_logger_t *logger)
{
    if (ecu == NULL || time == NULL) {
        return HIL_ERR_NULL;
    }
    ecu->fault_latched = true;
    return change_state(ecu, HIL_ECU_FAULT, time, logger);
}

hil_status_t hil_ecu_clear_fault(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                                 const hil_logger_t *logger)
{
    if (ecu == NULL || time == NULL) {
        return HIL_ERR_NULL;
    }
    ecu->comm_fault = false;
    ecu->fault_latched = false;
    if (ecu->state == HIL_ECU_FAULT) {
        return change_state(ecu, HIL_ECU_RECOVERY, time, logger);
    }
    return HIL_OK;
}

hil_status_t hil_ecu_reset(hil_ecu_t *ecu, const hil_virtual_time_t *time,
                           const hil_logger_t *logger)
{
    if (ecu == NULL || time == NULL) {
        return HIL_ERR_NULL;
    }
    ecu->comm_fault = false;
    ecu->fault_latched = false;
    ecu->recovery_attempts = 0U;
    ecu->power_requested = true;
    return change_state(ecu, HIL_ECU_INIT, time, logger);
}

bool hil_ecu_is_running(const hil_ecu_t *ecu)
{
    return ecu != NULL && ecu->state == HIL_ECU_RUN;
}

bool hil_ecu_has_safe_outputs(const hil_ecu_t *ecu)
{
    return ecu != NULL &&
           (ecu->state == HIL_ECU_POWER_OFF || ecu->state == HIL_ECU_FAULT);
}

hil_ecu_state_t hil_ecu_get_state(const hil_ecu_t *ecu)
{
    return ecu != NULL ? ecu->state : HIL_ECU_POWER_OFF;
}

static void set_base_value(hil_signal_registry_t *registry, const char *name,
                           double value)
{
    hil_signal_t *signal = hil_signal_registry_find(registry, name);
    if (signal != NULL) {
        signal->raw_value = value;
        signal->value = value;
        signal->valid = true;
        signal->open_circuit = false;
    }
}

hil_status_t hil_ecu_run_periodic_tasks(hil_ecu_t *ecu,
                                        hil_signal_registry_t *signals,
                                        const hil_logger_t *logger)
{
    hil_signal_t *throttle;
    double throttle_percent = 0.0;
    double engine_speed;
    double vehicle_speed;
    double oil_pressure;
    if (ecu == NULL || signals == NULL) {
        return HIL_ERR_NULL;
    }
    throttle = hil_signal_registry_find(signals, "throttle_position");
    if (throttle != NULL && throttle->valid) {
        throttle_percent = throttle->value;
    }
    engine_speed = 800.0 + throttle_percent / 100.0 * 6200.0;
    vehicle_speed = engine_speed / 60.0;
    oil_pressure = 1.2 + engine_speed / 2200.0;

    set_base_value(signals, "engine_speed", engine_speed);
    set_base_value(signals, "vehicle_speed", vehicle_speed);
    set_base_value(signals, "oil_pressure", oil_pressure);
    set_base_value(signals, "battery_voltage", 12.5);
    set_base_value(signals, "coolant_temp", 90.0);
    set_base_value(signals, "fuel_level", 55.0);
    set_base_value(signals, "brake_pressure", 0.0);
    set_base_value(signals, "gear_position", 3.0);
    set_base_value(signals, "ambient_temp", 25.0);
    hil_log_message(logger, HIL_LOG_DEBUG, "ecu",
                    "periodic tasks executed at %llu ms, engine=%.2f rpm",
                    (unsigned long long)ecu->last_periodic_ms, engine_speed);
    return HIL_OK;
}

void hil_ecu_apply_safe_outputs(hil_ecu_t *ecu,
                                hil_signal_registry_t *signals,
                                const hil_logger_t *logger)
{
    if (ecu == NULL || signals == NULL) {
        return;
    }
    set_base_value(signals, "engine_speed", 0.0);
    set_base_value(signals, "vehicle_speed", 0.0);
    set_base_value(signals, "throttle_position", 0.0);
    set_base_value(signals, "brake_pressure", 0.0);
    set_base_value(signals, "oil_pressure", 0.0);
    hil_log_message(logger, HIL_LOG_WARN, "ecu",
                    "safe outputs applied in %s state",
                    hil_ecu_state_name(ecu->state));
}
