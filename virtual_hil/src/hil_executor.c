#include "hil_executor.h"

#include <stdio.h>
#include <string.h>

static hil_status_t run_case(hil_executor_t *executor,
                             hil_test_case_t *test,
                             const hil_logger_t *logger);
static hil_status_t run_step(hil_executor_t *executor, const hil_step_t *step,
                             const hil_logger_t *logger);
static hil_status_t run_wait(hil_executor_t *executor, uint64_t duration_ms,
                             const hil_logger_t *logger);
static void publish_periodic_frames(hil_executor_t *executor,
                                    const hil_logger_t *logger);

hil_status_t hil_executor_init(hil_executor_t *executor,
                               const hil_config_t *config,
                               hil_signal_registry_t *signals,
                               const char *script_path,
                               hil_data_logger_t *data_logger,
                               hil_report_builder_t *report,
                               const hil_logger_t *logger)
{
    hil_status_t status;
    if (executor == NULL || config == NULL || signals == NULL ||
        script_path == NULL) {
        return HIL_ERR_NULL;
    }
    (void)memset(executor, 0, sizeof(*executor));
    executor->config = *config;
    executor->signals = signals;
    executor->data_logger = data_logger;
    executor->report = report;
    hil_time_init(&executor->time);
    hil_script_init(&executor->script);
    status = hil_script_load_file(&executor->script, script_path, logger);
    if (status != HIL_OK) {
        return status;
    }
    hil_ecu_init(&executor->ecu, &executor->time,
                 config->ecu_power_on_delay_ms,
                 config->ecu_comm_timeout_ms,
                 config->ecu_recovery_delay_ms, config->tick_ms);
    hil_bus_init(&executor->bus, config->bus_queue_capacity,
                 config->bus_loss_probability_percent, config->bus_delay_ms,
                 config->bus_tamper_enabled, config->random_seed);
    if (executor->bus.queue == NULL) {
        return HIL_ERR_NOMEM;
    }
    hil_fault_manager_init(&executor->faults, config->random_seed);
    return HIL_OK;
}

void hil_executor_deinit(hil_executor_t *executor)
{
    if (executor == NULL) {
        return;
    }
    hil_fault_manager_deinit(&executor->faults);
    hil_bus_deinit(&executor->bus);
    hil_script_deinit(&executor->script);
}

hil_status_t hil_executor_run(hil_executor_t *executor,
                              const hil_logger_t *logger)
{
    hil_test_case_t *test;
    hil_status_t status = HIL_OK;
    if (executor == NULL || executor->signals == NULL) {
        return HIL_ERR_NULL;
    }
    for (test = executor->script.cases; test != NULL; test = test->next) {
        if (executor->report != NULL) {
            (void)hil_report_begin_case(executor->report, test->name);
        }
        status = run_case(executor, test, logger);
        if (status != HIL_OK) {
            if (executor->report != NULL) {
                (void)hil_report_add_step(
                    executor->report, 0, HIL_STEP_NONE, "", "", "",
                    HIL_REPORT_ERROR, "test case aborted by error");
            }
            break;
        }
    }
    return status;
}

bool hil_executor_aborted(const hil_executor_t *executor)
{
    return executor != NULL && executor->aborted;
}

static hil_status_t run_case(hil_executor_t *executor,
                             hil_test_case_t *test,
                             const hil_logger_t *logger)
{
    const hil_step_t *step;
    hil_status_t status;
    if (executor == NULL || test == NULL || executor->signals == NULL) {
        return HIL_ERR_NULL;
    }
    executor->current_case = test;
    executor->aborted = false;
    (void)hil_fault_manager_clear(&executor->faults);
    (void)hil_signal_registry_reset(executor->signals);
    (void)hil_ecu_reset(&executor->ecu, &executor->time, logger);
    hil_bus_deinit(&executor->bus);
    hil_bus_init(&executor->bus, executor->config.bus_queue_capacity,
                 executor->config.bus_loss_probability_percent,
                 executor->config.bus_delay_ms,
                 executor->config.bus_tamper_enabled,
                 executor->config.random_seed);
    if (executor->bus.queue == NULL) {
        return HIL_ERR_NOMEM;
    }
    executor->bus.next_sequence = 1U;
    executor->ecu.last_tx_ms = hil_time_now(&executor->time);
    hil_log_message(logger, HIL_LOG_INFO, "executor",
                    "start test case '%s'", test->name);
    for (step = test->steps; step != NULL; step = step->next) {
        executor->current_step = (hil_step_t *)step;
        if (step->type == HIL_STEP_END) {
            if (executor->report != NULL) {
                (void)hil_report_add_step(
                    executor->report, step->line, step->type, "", "", "",
                    HIL_REPORT_PASS, "test case ended");
            }
            break;
        }
        status = run_step(executor, step, logger);
        if (status != HIL_OK && executor->config.stop_on_failure) {
            executor->aborted = true;
            break;
        }
    }
    executor->current_case = NULL;
    executor->current_step = NULL;
    return executor->aborted ? HIL_ERR_ASSERT : HIL_OK;
}

static hil_status_t run_step(hil_executor_t *executor,
                             const hil_step_t *step,
                             const hil_logger_t *logger)
{
    hil_status_t status = HIL_OK;
    hil_report_status_t report_status = HIL_REPORT_PASS;
    char expected[HIL_MESSAGE_MAX] = "";
    char actual[HIL_MESSAGE_MAX] = "";
    char message[HIL_MESSAGE_MAX] = "";

    if (executor == NULL || step == NULL || executor->signals == NULL) {
        return HIL_ERR_NULL;
    }
    switch (step->type) {
    case HIL_STEP_SET: {
        hil_signal_t *signal =
            hil_signal_registry_find(executor->signals, step->target);
        if (signal == NULL) {
            status = HIL_ERR_NOT_FOUND;
            (void)snprintf(message, sizeof(message), "unknown signal %s",
                           step->target);
            report_status = HIL_REPORT_ERROR;
            break;
        }
        status = hil_signal_registry_set(executor->signals, step->target,
                                         step->value, logger);
        if (status != HIL_OK) {
            report_status = HIL_REPORT_ERROR;
            (void)snprintf(message, sizeof(message),
                           "failed to set %s to %.6g", step->target,
                           step->value);
            break;
        }
        (void)snprintf(expected, sizeof(expected), "%.6g", step->value);
        (void)snprintf(actual, sizeof(actual), "%.6g", signal->value);
        (void)snprintf(message, sizeof(message), "signal set");
        break;
    }
    case HIL_STEP_WAIT: {
        status = run_wait(executor, (uint64_t)step->value, logger);
        (void)snprintf(expected, sizeof(expected), "%llu ms",
                       (unsigned long long)(uint64_t)step->value);
        (void)snprintf(actual, sizeof(actual), "%llu ms",
                       (unsigned long long)hil_time_now(&executor->time));
        if (status != HIL_OK) {
            report_status = HIL_REPORT_ERROR;
            (void)snprintf(message, sizeof(message), "wait failed: %s",
                           hil_status_name(status));
        } else {
            (void)snprintf(message, sizeof(message), "wait completed");
        }
        break;
    }
    case HIL_STEP_FAULT: {
        hil_fault_spec_t spec;
        (void)memset(&spec, 0, sizeof(spec));
        if (step->ecu_fault != HIL_ECU_FAULT_NONE) {
            if (step->ecu_fault == HIL_ECU_FAULT_LATCH) {
                status = hil_ecu_latch_fault(&executor->ecu,
                                             &executor->time, logger);
            } else if (step->ecu_fault == HIL_ECU_FAULT_COMM) {
                status = hil_ecu_set_comm_fault(&executor->ecu, true,
                                                &executor->time, logger);
            } else {
                status = hil_ecu_clear_fault(&executor->ecu, &executor->time,
                                             logger);
            }
            if (status == HIL_OK) {
                status = hil_ecu_tick(&executor->ecu, &executor->time,
                                      executor->signals, logger);
            }
            (void)snprintf(expected, sizeof(expected), "ECU %s",
                           hil_ecu_fault_action_name(step->ecu_fault));
            (void)snprintf(actual, sizeof(actual), "%s latched=%d",
                           hil_ecu_state_name(executor->ecu.state),
                           executor->ecu.fault_latched ? 1 : 0);
            if (status != HIL_OK) {
                report_status = HIL_REPORT_ERROR;
                (void)snprintf(message, sizeof(message),
                               "failed to inject ECU fault");
            } else {
                (void)snprintf(message, sizeof(message), "ECU fault injected");
            }
            break;
        }
        (void)strncpy(spec.signal_name, step->target,
                      sizeof(spec.signal_name) - 1U);
        spec.type = step->fault_type;
        spec.value = step->value;
        spec.start_ms = hil_time_now(&executor->time);
        if (step->second_value > 0.0) {
            uint64_t duration = (uint64_t)step->second_value;
            if (UINT64_MAX - spec.start_ms < duration) {
                status = HIL_ERR_OVERFLOW;
                report_status = HIL_REPORT_ERROR;
                (void)snprintf(message, sizeof(message),
                               "fault duration overflows");
                break;
            }
            spec.end_ms = spec.start_ms + duration;
        }
        status = hil_fault_manager_add(&executor->faults, &spec, logger);
        (void)snprintf(expected, sizeof(expected), "%s %s %.6g",
                       step->target, hil_fault_type_name(spec.type),
                       spec.value);
        (void)snprintf(actual, sizeof(actual), "active from %llu ms",
                       (unsigned long long)spec.start_ms);
        if (status != HIL_OK) {
            report_status = HIL_REPORT_ERROR;
            (void)snprintf(message, sizeof(message), "failed to inject fault");
        } else {
            (void)snprintf(message, sizeof(message), "fault injected");
        }
        break;
    }
    case HIL_STEP_ASSERT: {
        const hil_signal_t *signal =
            hil_signal_registry_find_const(executor->signals, step->target);
        hil_assert_result_t result;
        if (signal == NULL) {
            status = HIL_ERR_NOT_FOUND;
            report_status = HIL_REPORT_ERROR;
            (void)snprintf(message, sizeof(message), "unknown signal %s",
                           step->target);
            (void)snprintf(expected, sizeof(expected), "%s %s", step->target,
                           hil_assert_op_name(step->assert_op));
            break;
        }
        if (step->assert_op == HIL_ASSERT_BETWEEN) {
            status = hil_assert_check_double_range(
                signal->value, step->value, step->second_value,
                step->tolerance, &result);
        } else {
            status = hil_assert_check_double(
                signal->value, step->assert_op, step->value,
                step->tolerance, &result);
        }
        (void)strncpy(expected, result.expected, sizeof(expected) - 1U);
        (void)strncpy(actual, result.actual, sizeof(actual) - 1U);
        (void)strncpy(message, result.message, sizeof(message) - 1U);
        report_status = result.passed ? HIL_REPORT_PASS : HIL_REPORT_FAIL;
        break;
    }
    case HIL_STEP_RESET: {
        (void)hil_fault_manager_clear(&executor->faults);
        (void)hil_signal_registry_reset(executor->signals);
        status = hil_ecu_reset(&executor->ecu, &executor->time, logger);
        (void)snprintf(expected, sizeof(expected), "reset ECU and faults");
        (void)snprintf(actual, sizeof(actual), "%s",
                       hil_ecu_state_name(executor->ecu.state));
        if (status != HIL_OK) {
            report_status = HIL_REPORT_ERROR;
            (void)snprintf(message, sizeof(message), "reset failed");
        } else {
            (void)snprintf(message, sizeof(message), "system reset");
        }
        break;
    }
    case HIL_STEP_POWER: {
        status = step->power_on
                     ? hil_ecu_power_on(&executor->ecu, &executor->time, logger)
                     : hil_ecu_power_off(&executor->ecu, &executor->time,
                                         logger);
        if (status != HIL_OK) {
            report_status = HIL_REPORT_ERROR;
        } else {
            status = hil_ecu_tick(&executor->ecu, &executor->time,
                                  executor->signals, logger);
            if (status != HIL_OK) {
                report_status = HIL_REPORT_ERROR;
            }
        }
        (void)snprintf(expected, sizeof(expected), "power %s",
                       step->power_on ? "ON" : "OFF");
        (void)snprintf(actual, sizeof(actual), "%s",
                       hil_ecu_state_name(executor->ecu.state));
        if (status != HIL_OK) {
            (void)snprintf(message, sizeof(message), "power step failed: %s",
                           hil_status_name(status));
        } else {
            (void)snprintf(message, sizeof(message), "power switched");
        }
        break;
    }
    case HIL_STEP_TEST:
    case HIL_STEP_END:
    case HIL_STEP_NONE:
        status = HIL_OK;
        (void)snprintf(message, sizeof(message), "step handled");
        break;
    }

    if (executor->report != NULL) {
        (void)hil_report_add_step(executor->report, step->line, step->type,
                                  step->target, expected, actual,
                                  report_status, message);
    }
    if (status != HIL_OK) {
        hil_log_message(logger, HIL_LOG_ERROR, "executor",
                        "step at line %d failed: %s", step->line,
                        hil_status_name(status));
    }
    return status;
}

static hil_status_t run_wait(hil_executor_t *executor, uint64_t duration_ms,
                             const hil_logger_t *logger)
{
    uint64_t remaining = duration_ms;
    uint64_t tick = executor->config.tick_ms;
    if (tick == 0U) {
        tick = 1U;
    }
    while (remaining > 0U) {
        uint64_t delta = remaining < tick ? remaining : tick;
        hil_status_t status = hil_time_advance(&executor->time, delta);
        if (status != HIL_OK) {
            return status;
        }
        hil_bus_set_time(&executor->bus, hil_time_now(&executor->time));
        status = hil_ecu_tick(&executor->ecu, &executor->time,
                              executor->signals, logger);
        if (status != HIL_OK) {
            return status;
        }
        status = hil_fault_manager_apply(&executor->faults,
                                         executor->signals,
                                         hil_time_now(&executor->time),
                                         logger);
        if (status != HIL_OK) {
            return status;
        }
        publish_periodic_frames(executor, logger);
        if (executor->data_logger != NULL) {
            status = hil_data_logger_log_sample(
                executor->data_logger, hil_time_now(&executor->time), logger);
            if (status != HIL_OK) {
                return status;
            }
        }
        remaining -= delta;
    }
    return HIL_OK;
}

static void publish_periodic_frames(hil_executor_t *executor,
                                    const hil_logger_t *logger)
{
    hil_frame_t frame;
    hil_status_t status;
    uint64_t now = hil_time_now(&executor->time);
    const hil_signal_t *engine;
    const hil_signal_t *vehicle;
    if (now - executor->ecu.last_tx_ms < 10U) {
        return;
    }
    executor->ecu.last_tx_ms = now;
    engine = hil_signal_registry_find_const(executor->signals,
                                            "engine_speed");
    vehicle = hil_signal_registry_find_const(executor->signals,
                                             "vehicle_speed");
    if (engine == NULL || vehicle == NULL) {
        return;
    }
    hil_frame_init(&frame, 0x100U, 4U, now, executor->bus.next_sequence,
                   executor->bus.next_alive_counter);
    status = hil_frame_encode_double(&frame, 0U, engine->value, 0.1, 0.0);
    if (status != HIL_OK) {
        return;
    }
    status = hil_frame_encode_double(&frame, 2U, vehicle->value, 0.01, 0.0);
    if (status != HIL_OK) {
        return;
    }
    (void)hil_bus_publish(&executor->bus, frame.id, frame.dlc, frame.data,
                          now, logger);
    while ((status = hil_bus_poll(&executor->bus, &frame, logger)) == HIL_OK) {
        hil_log_message(logger, HIL_LOG_DEBUG, "bus",
                        "delivered frame id=%u seq=%u crc=%u", frame.id,
                        frame.sequence, frame.crc);
    }
    if (status != HIL_ERR_NOT_FOUND && status != HIL_ERR_TIMEOUT &&
        status != HIL_ERR_FAULT) {
        hil_log_message(logger, HIL_LOG_WARN, "bus",
                        "frame delivery ended with %s",
                        hil_status_name(status));
    }
}
