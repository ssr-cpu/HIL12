#include "hil_web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "web_http.h"

void hil_web_default_signals(hil_signal_registry_t *registry,
                             const hil_logger_t *logger)
{
    static const struct {
        const char *name;
        const char *unit;
        double min_value;
        double max_value;
        double initial;
    } defaults[] = {
        {"battery_voltage", "V", 9.0, 16.0, 12.5},
        {"engine_speed", "rpm", 0.0, 8000.0, 0.0},
        {"vehicle_speed", "km/h", 0.0, 260.0, 0.0},
        {"coolant_temp", "degC", -40.0, 140.0, 25.0},
        {"throttle_position", "%", 0.0, 100.0, 0.0},
        {"brake_pressure", "bar", 0.0, 250.0, 0.0},
        {"fuel_level", "%", 0.0, 100.0, 60.0},
        {"oil_pressure", "bar", 0.0, 8.0, 1.2},
        {"gear_position", "", 0.0, 8.0, 0.0},
        {"ambient_temp", "degC", -40.0, 80.0, 25.0},
        {"ecu_state", "", 0.0, 6.0, 0.0},
        {"ecu_fault_latched", "", 0.0, 1.0, 0.0}
    };
    size_t index;
    for (index = 0U; index < sizeof(defaults) / sizeof(defaults[0]);
         index++) {
        hil_signal_t signal;
        hil_signal_init(&signal, defaults[index].name, defaults[index].unit,
                        defaults[index].min_value, defaults[index].max_value,
                        defaults[index].initial);
        (void)hil_signal_registry_add(registry, &signal, logger);
    }
}

void hil_web_session_init(hil_web_session_t *session)
{
    if (session == NULL) {
        return;
    }
    (void)memset(session, 0, sizeof(*session));
    hil_config_init(&session->config);
    session->config.tick_ms = 10U;
    hil_logger_init(&session->app_logger, stderr, HIL_LOG_WARN);
    hil_time_init(&session->time);
    hil_signal_registry_init(&session->signals);
    hil_fault_manager_init(&session->faults, session->config.random_seed);
    hil_report_init(&session->report);
    session->sample_time =
        (uint64_t *)calloc(HIL_WEB_MAX_SAMPLES, sizeof(*session->sample_time));
    session->sample_values = (double *)calloc(
        HIL_WEB_MAX_SAMPLES * HIL_WEB_MAX_SIGNALS,
        sizeof(*session->sample_values));
}

void hil_web_session_deinit(hil_web_session_t *session)
{
    if (session == NULL) {
        return;
    }
    if (session->ready) {
        (void)hil_data_logger_close(&session->data_logger,
                                    &session->app_logger);
    }
    hil_report_deinit(&session->report);
    hil_fault_manager_deinit(&session->faults);
    hil_bus_deinit(&session->bus);
    hil_signal_registry_deinit(&session->signals);
    free(session->sample_time);
    free(session->sample_values);
    session->sample_time = NULL;
    session->sample_values = NULL;
    session->ready = false;
}

hil_status_t hil_web_session_start(hil_web_session_t *session,
                                   const char *output_dir)
{
    hil_status_t status;
    char log_path[HIL_PATH_MAX];

    if (session == NULL) {
        return HIL_ERR_NULL;
    }
    if (output_dir != NULL && output_dir[0] != '\0') {
        status = hil_config_set_default_paths(&session->config, output_dir);
        if (status != HIL_OK) {
            return status;
        }
    }
    if (!web_ensure_dir(session->config.output_dir)) {
        return HIL_ERR_IO;
    }
    hil_web_default_signals(&session->signals, &session->app_logger);
    hil_ecu_init(&session->ecu, &session->time,
                 session->config.ecu_power_on_delay_ms,
                 session->config.ecu_comm_timeout_ms,
                 session->config.ecu_recovery_delay_ms,
                 session->config.tick_ms);
    hil_bus_init(&session->bus, session->config.bus_queue_capacity,
                 session->config.bus_loss_probability_percent,
                 session->config.bus_delay_ms,
                 session->config.bus_tamper_enabled,
                 session->config.random_seed);
    if (session->bus.queue == NULL) {
        return HIL_ERR_NOMEM;
    }
    if (snprintf(log_path, sizeof(log_path), "%s/%s",
                 session->config.output_dir,
                 session->config.log_file) < 0) {
        return HIL_ERR_OVERFLOW;
    }
    status = hil_data_logger_open(&session->data_logger, log_path,
                                  &session->signals, &session->app_logger);
    if (status != HIL_OK) {
        hil_bus_deinit(&session->bus);
        return status;
    }
    session->sample_signal_count = hil_signal_registry_count(&session->signals);
    if (session->sample_signal_count > HIL_WEB_MAX_SIGNALS) {
        session->sample_signal_count = HIL_WEB_MAX_SIGNALS;
    }
    session->ready = true;
    (void)hil_ecu_power_on(&session->ecu, &session->time,
                           &session->app_logger);
    return HIL_OK;
}

static void publish_frames(hil_web_session_t *session)
{
    hil_frame_t frame;
    uint64_t now;
    const hil_signal_t *engine;
    const hil_signal_t *vehicle;

    if (session == NULL) {
        return;
    }
    now = hil_time_now(&session->time);
    if (now - session->ecu.last_tx_ms < 10U) {
        return;
    }
    session->ecu.last_tx_ms = now;
    engine = hil_signal_registry_find_const(&session->signals, "engine_speed");
    vehicle =
        hil_signal_registry_find_const(&session->signals, "vehicle_speed");
    if (engine == NULL || vehicle == NULL) {
        return;
    }
    hil_frame_init(&frame, 0x100U, 4U, now, session->bus.next_sequence,
                   session->bus.next_alive_counter);
    if (hil_frame_encode_double(&frame, 0U, engine->value, 0.1, 0.0) !=
        HIL_OK) {
        return;
    }
    if (hil_frame_encode_double(&frame, 2U, vehicle->value, 0.01, 0.0) !=
        HIL_OK) {
        return;
    }
    (void)hil_bus_publish(&session->bus, frame.id, frame.dlc, frame.data, now,
                          &session->app_logger);
    while (hil_bus_poll(&session->bus, &frame, &session->app_logger) ==
           HIL_OK) {
        /* 交付即丢弃：网页只统计总线计数器。 */
    }
}

hil_status_t hil_web_session_advance(hil_web_session_t *session,
                                     uint64_t duration_ms)
{
    uint64_t remaining = duration_ms;
    uint64_t tick;

    if (session == NULL || !session->ready) {
        return HIL_ERR_STATE;
    }
    tick = session->config.tick_ms == 0U ? 1U : session->config.tick_ms;
    while (remaining > 0U) {
        uint64_t delta = remaining < tick ? remaining : tick;
        hil_status_t status = hil_time_advance(&session->time, delta);
        uint64_t now;
        if (status != HIL_OK) {
            return status;
        }
        now = hil_time_now(&session->time);
        hil_bus_set_time(&session->bus, now);
        status = hil_ecu_tick(&session->ecu, &session->time, &session->signals,
                              &session->app_logger);
        if (status != HIL_OK) {
            return status;
        }
        status = hil_fault_manager_apply(&session->faults, &session->signals,
                                         now, &session->app_logger);
        if (status != HIL_OK) {
            return status;
        }
        publish_frames(session);
        hil_web_session_record(session);
        remaining -= delta;
    }
    return HIL_OK;
}

void hil_web_session_record(hil_web_session_t *session)
{
    uint64_t now;
    size_t index;
    size_t slot;
    size_t base;

    if (session == NULL || !session->ready) {
        return;
    }
    now = hil_time_now(&session->time);
    if (session->data_logger.open) {
        (void)hil_data_logger_log_sample(&session->data_logger, now,
                                         &session->app_logger);
    }
    if (session->sample_time == NULL || session->sample_values == NULL) {
        return;
    }
    if (session->sample_count < HIL_WEB_MAX_SAMPLES) {
        index = session->sample_count;
        session->sample_count++;
    } else {
        (void)memmove(session->sample_time, session->sample_time + 1U,
                      (HIL_WEB_MAX_SAMPLES - 1U) * sizeof(*session->sample_time));
        (void)memmove(session->sample_values,
                      session->sample_values + HIL_WEB_MAX_SIGNALS,
                      (HIL_WEB_MAX_SAMPLES - 1U) * HIL_WEB_MAX_SIGNALS *
                          sizeof(*session->sample_values));
        index = HIL_WEB_MAX_SAMPLES - 1U;
    }
    session->sample_time[index] = now;
    base = index * HIL_WEB_MAX_SIGNALS;
    for (slot = 0U; slot < session->sample_signal_count; slot++) {
        if (slot >= session->signals.count) {
            session->sample_values[base + slot] = 0.0;
            continue;
        }
        session->sample_values[base + slot] = session->signals.items[slot].value;
    }
}

hil_status_t hil_web_session_reset(hil_web_session_t *session)
{
    if (session == NULL || !session->ready) {
        return HIL_ERR_STATE;
    }
    (void)hil_fault_manager_clear(&session->faults);
    (void)hil_signal_registry_reset(&session->signals);
    (void)hil_ecu_reset(&session->ecu, &session->time, &session->app_logger);
    hil_bus_deinit(&session->bus);
    hil_bus_init(&session->bus, session->config.bus_queue_capacity,
                 session->config.bus_loss_probability_percent,
                 session->config.bus_delay_ms,
                 session->config.bus_tamper_enabled,
                 session->config.random_seed);
    if (session->bus.queue == NULL) {
        return HIL_ERR_NOMEM;
    }
    session->sample_count = 0U;
    return HIL_OK;
}

hil_status_t hil_web_session_reconfigure_bus(hil_web_session_t *session,
                                             unsigned int loss_percent,
                                             uint64_t delay_ms, bool tamper,
                                             size_t capacity)
{
    if (session == NULL || !session->ready) {
        return HIL_ERR_STATE;
    }
    if (loss_percent > 100U) {
        return HIL_ERR_RANGE;
    }
    session->config.bus_loss_probability_percent = loss_percent;
    session->config.bus_delay_ms = delay_ms;
    session->config.bus_tamper_enabled = tamper;
    if (capacity > 0U) {
        session->config.bus_queue_capacity = capacity;
    }
    hil_bus_deinit(&session->bus);
    hil_bus_init(&session->bus, session->config.bus_queue_capacity,
                 session->config.bus_loss_probability_percent,
                 session->config.bus_delay_ms,
                 session->config.bus_tamper_enabled,
                 session->config.random_seed);
    if (session->bus.queue == NULL) {
        return HIL_ERR_NOMEM;
    }
    return HIL_OK;
}
