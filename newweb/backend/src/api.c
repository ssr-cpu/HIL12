#include "hil_web.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "web_http.h"

static hil_web_session_t *g_session = NULL;

void hil_web_api_set_session(hil_web_session_t *session)
{
    g_session = session;
}

static void json_add(char *output, size_t size, size_t *used, const char *fmt,
                     ...)
{
    va_list args;
    int written;
    if (output == NULL || used == NULL || *used >= size) {
        return;
    }
    va_start(args, fmt);
    written = vsnprintf(output + *used, size - *used, fmt, args);
    va_end(args);
    if (written < 0) {
        return;
    }
    if (*used + (size_t)written >= size) {
        *used = size - 1U;
        return;
    }
    *used += (size_t)written;
}

static void json_ok(char *output, size_t size, const char *payload)
{
    size_t used = 0U;
    json_add(output, size, &used, "{\"ok\":true,%s}", payload != NULL ? payload : "");
}

static void json_error(char *output, size_t size, const char *message,
                       const char *detail)
{
    char escaped[HIL_MESSAGE_MAX];
    web_json_escape(message != NULL ? message : "error", escaped,
                    sizeof(escaped));
    if (detail != NULL && detail[0] != '\0') {
        char escaped_detail[HIL_MESSAGE_MAX];
        web_json_escape(detail, escaped_detail, sizeof(escaped_detail));
        (void)snprintf(output, size,
                       "{\"ok\":false,\"error\":\"%s\",\"detail\":\"%s\"}",
                       escaped, escaped_detail);
        return;
    }
    (void)snprintf(output, size, "{\"ok\":false,\"error\":\"%s\"}", escaped);
}

static void add_double(char *buffer, size_t buffer_size, double value)
{
    (void)snprintf(buffer, buffer_size, "%.6g", value);
}

static void write_state_json(const hil_web_session_t *session, char *output,
                             size_t size)
{
    size_t used = 0U;
    size_t index;
    char number[64];
    char escaped[HIL_NAME_MAX * 2];

    json_add(output, size, &used, "\"time_ms\":%llu",
             (unsigned long long)hil_time_now(&session->time));
    json_add(output, size, &used, ",\"ecu_state\":\"%s\"",
             hil_ecu_state_name(session->ecu.state));
    json_add(output, size, &used, ",\"ecu_state_id\":%d",
             (int)session->ecu.state);
    json_add(output, size, &used, ",\"latched\":%s",
             session->ecu.fault_latched ? "true" : "false");
    json_add(output, size, &used, ",\"power_requested\":%s",
             session->ecu.power_requested ? "true" : "false");
    json_add(output, size, &used, ",\"comm_fault\":%s",
             session->ecu.comm_fault ? "true" : "false");
    json_add(output, size, &used, ",\"recovery_attempts\":%u",
             session->ecu.recovery_attempts);

    json_add(output, size, &used, ",\"signals\":[");
    for (index = 0U; index < session->signals.count; index++) {
        const hil_signal_t *signal = &session->signals.items[index];
        add_double(number, sizeof(number), signal->value);
        web_json_escape(signal->name, escaped, sizeof(escaped));
        json_add(output, size, &used,
                 "%s{\"name\":\"%s\",\"unit\":\"%s\",\"value\":%s,"
                 "\"min\":%.6g,\"max\":%.6g,\"valid\":%s,\"open\":%s}",
                 index == 0U ? "" : ",", escaped, signal->unit, number,
                 signal->min_value, signal->max_value,
                 signal->valid ? "true" : "false",
                 signal->open_circuit ? "true" : "false");
    }
    json_add(output, size, &used, "]");

    json_add(output, size, &used, ",\"faults\":[");
    for (index = 0U; index < session->faults.count; index++) {
        const hil_fault_spec_t *spec = &session->faults.items[index];
        web_json_escape(spec->signal_name, escaped, sizeof(escaped));
        json_add(output, size, &used,
                 "%s{\"signal\":\"%s\",\"type\":\"%s\",\"value\":%.6g,"
                 "\"start_ms\":%llu,\"end_ms\":%llu,\"active\":%s}",
                 index == 0U ? "" : ",", escaped,
                 hil_fault_type_name(spec->type), spec->value,
                 (unsigned long long)spec->start_ms,
                 (unsigned long long)spec->end_ms,
                 spec->active ? "true" : "false");
    }
    json_add(output, size, &used, "]");

    json_add(output, size, &used,
             ",\"bus\":{\"capacity\":%llu,\"queued\":%llu,\"published\":%llu,"
             "\"delivered\":%llu,\"lost\":%llu,\"tampered\":%llu,"
             "\"loss_percent\":%u,\"delay_ms\":%llu,\"tamper\":%s}",
             (unsigned long long)session->bus.capacity,
             (unsigned long long)session->bus.count,
             (unsigned long long)session->bus.total_published,
             (unsigned long long)session->bus.total_delivered,
             (unsigned long long)session->bus.total_lost,
             (unsigned long long)session->bus.total_tampered,
             session->bus.loss_probability_percent,
             (unsigned long long)session->bus.delay_ms,
             session->bus.tamper_enabled ? "true" : "false");

    json_add(output, size, &used,
             ",\"recording\":{\"samples\":%llu,\"signal_count\":%llu,"
             "\"log_file\":\"%s\"}",
             (unsigned long long)session->sample_count,
             (unsigned long long)session->sample_signal_count,
             session->config.log_file);
    output[used] = '\0';
}

static int api_state(const web_request_t *request, char *output, size_t size)
{
    char *payload;
    (void)request;
    if (g_session == NULL || !g_session->ready) {
        json_error(output, size, "session not ready", NULL);
        return 500;
    }
    payload = (char *)malloc(WEB_MAX_RESPONSE);
    if (payload == NULL) {
        json_error(output, size, "out of memory", NULL);
        return 500;
    }
    write_state_json(g_session, payload, WEB_MAX_RESPONSE);
    json_ok(output, size, payload);
    free(payload);
    return 200;
}

static int api_set(const web_request_t *request, char *output, size_t size)
{
    char name[HIL_NAME_MAX];
    char value_text[64];
    double value;
    hil_status_t status;

    if (!web_query_get(request->query, "signal", name, sizeof(name)) ||
        !web_query_get(request->query, "value", value_text,
                       sizeof(value_text))) {
        json_error(output, size, "missing signal or value", NULL);
        return 400;
    }
    if (!web_parse_double(value_text, &value)) {
        json_error(output, size, "invalid value", value_text);
        return 400;
    }
    status = hil_signal_registry_set(&g_session->signals, name, value,
                                     &g_session->app_logger);
    if (status != HIL_OK) {
        json_error(output, size, "cannot set signal", hil_status_name(status));
        return 400;
    }
    (void)hil_fault_manager_apply(&g_session->faults, &g_session->signals,
                                  hil_time_now(&g_session->time),
                                  &g_session->app_logger);
    hil_web_session_record(g_session);
    return api_state(request, output, size);
}

static int api_wait(const web_request_t *request, char *output, size_t size)
{
    char value_text[64];
    uint64_t duration = 0U;
    hil_status_t status;

    if (!web_query_get(request->query, "ms", value_text, sizeof(value_text)) ||
        !web_parse_u64(value_text, &duration) || duration == 0U) {
        json_error(output, size, "missing or invalid ms", value_text);
        return 400;
    }
    if (duration > 600000U) {
        json_error(output, size, "duration too large", "max 600000 ms");
        return 400;
    }
    status = hil_web_session_advance(g_session, duration);
    if (status != HIL_OK) {
        json_error(output, size, "advance failed", hil_status_name(status));
        return 400;
    }
    return api_state(request, output, size);
}

static int api_fault(const web_request_t *request, char *output, size_t size)
{
    char name[HIL_NAME_MAX];
    char type_text[HIL_NAME_MAX];
    char value_text[64];
    char duration_text[64];
    hil_fault_spec_t spec;
    hil_fault_type_t type;
    hil_status_t status;
    uint64_t duration = 0U;

    (void)memset(&spec, 0, sizeof(spec));
    if (!web_query_get(request->query, "signal", name, sizeof(name)) ||
        !web_query_get(request->query, "type", type_text,
                       sizeof(type_text))) {
        json_error(output, size, "missing signal or type", NULL);
        return 400;
    }
    if (!hil_fault_type_from_name(type_text, &type) ||
        type == HIL_FAULT_NONE) {
        json_error(output, size, "unknown fault type", type_text);
        return 400;
    }
    if (web_query_get(request->query, "value", value_text,
                      sizeof(value_text))) {
        if (!web_parse_double(value_text, &spec.value)) {
            json_error(output, size, "invalid fault value", value_text);
            return 400;
        }
    } else {
        if (type != HIL_FAULT_OPEN_CIRCUIT && type != HIL_FAULT_DROPOUT) {
            json_error(output, size, "fault type requires a value", type_text);
            return 400;
        }
        spec.value = 0.0;
    }
    if (web_query_get(request->query, "duration", duration_text,
                      sizeof(duration_text))) {
        if (!web_parse_u64(duration_text, &duration)) {
            json_error(output, size, "invalid duration", duration_text);
            return 400;
        }
    }
    spec.type = type;
    spec.start_ms = hil_time_now(&g_session->time);
    if (duration > 0U) {
        if (UINT64_MAX - spec.start_ms < duration) {
            json_error(output, size, "duration overflows", NULL);
            return 400;
        }
        spec.end_ms = spec.start_ms + duration;
    }
    (void)strncpy(spec.signal_name, name, sizeof(spec.signal_name) - 1U);
    status = hil_fault_manager_add(&g_session->faults, &spec,
                                   &g_session->app_logger);
    if (status != HIL_OK) {
        json_error(output, size, "cannot inject fault",
                   hil_status_name(status));
        return 400;
    }
    (void)hil_fault_manager_apply(&g_session->faults, &g_session->signals,
                                  hil_time_now(&g_session->time),
                                  &g_session->app_logger);
    hil_web_session_record(g_session);
    return api_state(request, output, size);
}

static int api_fault_clear(const web_request_t *request, char *output,
                           size_t size)
{
    (void)request;
    (void)hil_fault_manager_clear(&g_session->faults);
    (void)hil_fault_manager_apply(&g_session->faults, &g_session->signals,
                                  hil_time_now(&g_session->time),
                                  &g_session->app_logger);
    hil_web_session_record(g_session);
    return api_state(request, output, size);
}

static int api_ecu_fault(const web_request_t *request, char *output,
                         size_t size)
{
    char action[HIL_NAME_MAX];
    hil_status_t status;

    if (!web_query_get(request->query, "action", action, sizeof(action))) {
        json_error(output, size, "missing action", NULL);
        return 400;
    }
    if (strcmp(action, "LATCH") == 0) {
        status = hil_ecu_latch_fault(&g_session->ecu, &g_session->time,
                                     &g_session->app_logger);
    } else if (strcmp(action, "COMM") == 0) {
        status = hil_ecu_set_comm_fault(&g_session->ecu, true,
                                        &g_session->time,
                                        &g_session->app_logger);
    } else if (strcmp(action, "CLEAR") == 0) {
        status = hil_ecu_clear_fault(&g_session->ecu, &g_session->time,
                                     &g_session->app_logger);
    } else {
        json_error(output, size, "unknown ECU fault action", action);
        return 400;
    }
    if (status != HIL_OK) {
        json_error(output, size, "ECU fault failed", hil_status_name(status));
        return 400;
    }
    (void)hil_ecu_tick(&g_session->ecu, &g_session->time, &g_session->signals,
                       &g_session->app_logger);
    hil_web_session_record(g_session);
    return api_state(request, output, size);
}

static int api_power(const web_request_t *request, char *output, size_t size)
{
    char state[HIL_NAME_MAX];
    hil_status_t status;

    if (!web_query_get(request->query, "state", state, sizeof(state))) {
        json_error(output, size, "missing state", NULL);
        return 400;
    }
    if (strcmp(state, "ON") == 0) {
        status = hil_ecu_power_on(&g_session->ecu, &g_session->time,
                                  &g_session->app_logger);
    } else if (strcmp(state, "OFF") == 0) {
        status = hil_ecu_power_off(&g_session->ecu, &g_session->time,
                                   &g_session->app_logger);
    } else {
        json_error(output, size, "state must be ON or OFF", state);
        return 400;
    }
    if (status != HIL_OK) {
        json_error(output, size, "power step failed", hil_status_name(status));
        return 400;
    }
    (void)hil_ecu_tick(&g_session->ecu, &g_session->time, &g_session->signals,
                       &g_session->app_logger);
    hil_web_session_record(g_session);
    return api_state(request, output, size);
}

static int api_reset(const web_request_t *request, char *output, size_t size)
{
    hil_status_t status = hil_web_session_reset(g_session);
    if (status != HIL_OK) {
        json_error(output, size, "reset failed", hil_status_name(status));
        return 400;
    }
    return api_state(request, output, size);
}

static int api_assert(const web_request_t *request, char *output, size_t size)
{
    char name[HIL_NAME_MAX];
    char op_text[HIL_NAME_MAX];
    char value_text[64];
    char tolerance_text[64];
    hil_assert_op_t op;
    hil_assert_result_t result;
    hil_signal_t *signal;
    hil_status_t status;
    double expected = 0.0;
    double tolerance = 0.001;
    char escaped[HIL_MESSAGE_MAX * 2];

    if (!web_query_get(request->query, "signal", name, sizeof(name)) ||
        !web_query_get(request->query, "op", op_text, sizeof(op_text))) {
        json_error(output, size, "missing signal or op", NULL);
        return 400;
    }
    if (!hil_assert_op_from_name(op_text, &op)) {
        json_error(output, size, "unknown operator", op_text);
        return 400;
    }
    if (!web_query_get(request->query, "value", value_text,
                       sizeof(value_text))) {
        json_error(output, size, "missing expected value", NULL);
        return 400;
    }
    {
        hil_ecu_state_t state;
        if (hil_ecu_state_from_name(value_text, &state)) {
            expected = (double)state;
        } else if (!web_parse_double(value_text, &expected)) {
            json_error(output, size, "invalid expected value", value_text);
            return 400;
        }
    }
    if (web_query_get(request->query, "tol", tolerance_text,
                      sizeof(tolerance_text))) {
        if (!web_parse_double(tolerance_text, &tolerance)) {
            json_error(output, size, "invalid tolerance", tolerance_text);
            return 400;
        }
    }
    signal = hil_signal_registry_find(&g_session->signals, name);
    if (signal == NULL) {
        json_error(output, size, "unknown signal", name);
        return 400;
    }
    status = hil_assert_check_double(signal->value, op, expected, tolerance,
                                     &result);
    if (status != HIL_OK) {
        json_error(output, size, "assertion error", hil_status_name(status));
        return 400;
    }
    (void)hil_report_add_step(&g_session->report, 0, HIL_STEP_ASSERT, name,
                              result.expected, result.actual,
                              result.passed ? HIL_REPORT_PASS : HIL_REPORT_FAIL,
                              result.message);
    web_json_escape(result.message, escaped, sizeof(escaped));
    {
        char *body = (char *)malloc(WEB_MAX_RESPONSE);
        size_t used = 0U;
        if (body == NULL) {
            json_error(output, size, "out of memory", NULL);
            return 500;
        }
        json_add(body, WEB_MAX_RESPONSE, &used,
                 "\"assert\":{\"signal\":\"%s\",\"op\":\"%s\",\"passed\":%s,"
                 "\"expected\":\"%s\",\"actual\":\"%s\",\"message\":\"%s\"},",
                 name, hil_assert_op_name(op),
                 result.passed ? "true" : "false", result.expected,
                 result.actual, escaped);
        write_state_json(g_session, body + used, WEB_MAX_RESPONSE - used);
        json_ok(output, size, body);
        free(body);
    }
    return 200;
}

static int api_history(const web_request_t *request, char *output,
                       size_t size)
{
    char name[HIL_NAME_MAX];
    char from_text[64];
    char to_text[64];
    uint64_t from_ms = 0U;
    uint64_t to_ms = UINT64_MAX;
    size_t index;
    size_t slot = SIZE_MAX;
    size_t used = 0U;
    size_t emitted = 0U;

    if (!web_query_get(request->query, "signal", name, sizeof(name))) {
        json_error(output, size, "missing signal", NULL);
        return 400;
    }
    if (web_query_get(request->query, "from", from_text, sizeof(from_text)) &&
        !web_parse_u64(from_text, &from_ms)) {
        json_error(output, size, "invalid from", from_text);
        return 400;
    }
    if (web_query_get(request->query, "to", to_text, sizeof(to_text)) &&
        !web_parse_u64(to_text, &to_ms)) {
        json_error(output, size, "invalid to", to_text);
        return 400;
    }
    for (index = 0U; index < g_session->signals.count; index++) {
        if (strcmp(g_session->signals.items[index].name, name) == 0) {
            slot = index;
            break;
        }
    }
    if (slot == SIZE_MAX) {
        json_error(output, size, "unknown signal", name);
        return 400;
    }
    {
        char *payload = (char *)malloc(WEB_MAX_RESPONSE);
        size_t stride = 1U;
        if (payload == NULL) {
            json_error(output, size, "out of memory", NULL);
            return 500;
        }
        if (g_session->sample_count > 1500U) {
            stride = g_session->sample_count / 1500U + 1U;
        }
        json_add(payload, WEB_MAX_RESPONSE, &used,
                 "\"history\":{\"signal\":\"%s\",\"from_ms\":%llu,"
                 "\"to_ms\":%llu,\"stride\":%llu,\"points\":[",
                 name, (unsigned long long)from_ms, (unsigned long long)to_ms,
                 (unsigned long long)stride);
        for (index = 0U; index < g_session->sample_count; index += stride) {
            uint64_t stamp = g_session->sample_time[index];
            double value =
                g_session->sample_values[index * HIL_WEB_MAX_SIGNALS + slot];
            if (stamp < from_ms || stamp > to_ms) {
                continue;
            }
            json_add(payload, WEB_MAX_RESPONSE, &used,
                     "%s{\"t\":%llu,\"v\":%.6g}", emitted == 0U ? "" : ",",
                     (unsigned long long)stamp, value);
            emitted++;
        }
        json_add(payload, WEB_MAX_RESPONSE, &used, "],\"count\":%llu}",
                 (unsigned long long)emitted);
        json_ok(output, size, payload);
        free(payload);
    }
    return 200;
}

static void serialize_report(const hil_report_builder_t *report, char *output,
                             size_t size)
{
    const hil_report_step_t *step;
    size_t used = 0U;
    size_t count = 0U;
    char esc_case[HIL_NAME_MAX * 2];
    char esc_target[HIL_NAME_MAX * 2];
    char esc_expected[HIL_MESSAGE_MAX * 2];
    char esc_actual[HIL_MESSAGE_MAX * 2];
    char esc_message[HIL_MESSAGE_MAX * 2];

    json_add(output, size, &used, "\"summary\":{\"steps\":%llu,\"passed\":%llu,"
             "\"failed\":%llu,\"skipped\":%llu,\"errors\":%llu},",
             (unsigned long long)report->step_count,
             (unsigned long long)report->passed,
             (unsigned long long)report->failed,
             (unsigned long long)report->skipped,
             (unsigned long long)report->errors);
    json_add(output, size, &used, "\"steps\":[");
    for (step = report->steps; step != NULL && count < 500U;
         step = step->next) {
        web_json_escape(step->case_name, esc_case, sizeof(esc_case));
        web_json_escape(step->target, esc_target, sizeof(esc_target));
        web_json_escape(step->expected, esc_expected, sizeof(esc_expected));
        web_json_escape(step->actual, esc_actual, sizeof(esc_actual));
        web_json_escape(step->message, esc_message, sizeof(esc_message));
        json_add(output, size, &used,
                 "%s{\"case\":\"%s\",\"line\":%d,\"type\":\"%s\","
                 "\"target\":\"%s\",\"expected\":\"%s\",\"actual\":\"%s\","
                 "\"status\":\"%s\",\"message\":\"%s\"}",
                 count == 0U ? "" : ",", esc_case, step->line,
                 hil_step_type_name(step->step_type), esc_target, esc_expected,
                 esc_actual, hil_report_status_name(step->status),
                 esc_message);
        count++;
    }
    json_add(output, size, &used, "]");
}

static int api_script(const web_request_t *request, char *output, size_t size)
{
    char script_text[WEB_MAX_BODY];
    char script_path[HIL_PATH_MAX];
    char report_path[HIL_PATH_MAX];
    hil_signal_registry_t registry;
    hil_report_builder_t report;
    hil_executor_t executor;
    hil_status_t status;
    size_t passed;
    size_t failed;
    size_t skipped;
    size_t errors;

    if (!web_body_get(request->body, "script", script_text,
                      sizeof(script_text)) &&
        !web_query_get(request->query, "script", script_text,
                       sizeof(script_text))) {
        json_error(output, size, "missing script body", NULL);
        return 400;
    }
    if (script_text[0] == '\0') {
        json_error(output, size, "script is empty", NULL);
        return 400;
    }
    if (snprintf(script_path, sizeof(script_path), "%s/last_script.hil",
                 g_session->config.output_dir) < 0) {
        json_error(output, size, "path overflow", NULL);
        return 500;
    }
    if (!web_write_file(script_path, script_text, strlen(script_text))) {
        json_error(output, size, "cannot write script file", script_path);
        return 500;
    }

    hil_signal_registry_init(&registry);
    hil_web_default_signals(&registry, &g_session->app_logger);
    hil_report_init(&report);
    status = hil_executor_init(&executor, &g_session->config, &registry,
                               script_path, NULL, &report,
                               &g_session->app_logger);
    if (status != HIL_OK) {
        hil_signal_registry_deinit(&registry);
        hil_report_deinit(&report);
        json_error(output, size, "cannot load script",
                   hil_status_name(status));
        return 400;
    }
    (void)hil_executor_run(&executor, &g_session->app_logger);
    serialize_report(&report, g_session->last_report_json,
                     sizeof(g_session->last_report_json));
    hil_report_summary(&report, &passed, &failed, &skipped, &errors);
    if (snprintf(report_path, sizeof(report_path), "%s/%s",
                 g_session->config.output_dir,
                 g_session->config.report_file) < 0) {
        json_error(output, size, "path overflow", NULL);
        return 500;
    }
    (void)hil_report_write_markdown(&report, report_path,
                                    &g_session->app_logger);
    hil_executor_deinit(&executor);
    hil_report_deinit(&report);
    hil_signal_registry_deinit(&registry);

    {
        char body[HIL_WEB_JSON_MAX + 256U];
        (void)snprintf(body, sizeof(body), "%s,\"all_passed\":%s",
                       g_session->last_report_json,
                       failed == 0U && errors == 0U ? "true" : "false");
        json_ok(output, size, body);
    }
    return 200;
}

static int api_report(const web_request_t *request, char *output, size_t size)
{
    (void)request;
    if (g_session->last_report_json[0] == '\0') {
        json_error(output, size, "no report yet", NULL);
        return 404;
    }
    json_ok(output, size, g_session->last_report_json);
    return 200;
}

static int api_config(const web_request_t *request, char *output, size_t size)
{
    char loss_text[64];
    char delay_text[64];
    char tamper_text[64];
    char capacity_text[64];
    unsigned int loss = g_session->config.bus_loss_probability_percent;
    uint64_t delay = g_session->config.bus_delay_ms;
    bool tamper = g_session->config.bus_tamper_enabled;
    size_t capacity = g_session->config.bus_queue_capacity;
    hil_status_t status;

    if (web_query_get(request->query, "loss", loss_text, sizeof(loss_text))) {
        uint64_t parsed = 0U;
        if (!web_parse_u64(loss_text, &parsed) || parsed > 100U) {
            json_error(output, size, "invalid loss percent", loss_text);
            return 400;
        }
        loss = (unsigned int)parsed;
    }
    if (web_query_get(request->query, "delay", delay_text,
                      sizeof(delay_text))) {
        if (!web_parse_u64(delay_text, &delay)) {
            json_error(output, size, "invalid delay", delay_text);
            return 400;
        }
    }
    if (web_query_get(request->query, "tamper", tamper_text,
                      sizeof(tamper_text))) {
        if (!web_parse_bool(tamper_text, &tamper)) {
            json_error(output, size, "invalid tamper flag", tamper_text);
            return 400;
        }
    }
    if (web_query_get(request->query, "capacity", capacity_text,
                      sizeof(capacity_text))) {
        uint64_t parsed = 0U;
        if (!web_parse_u64(capacity_text, &parsed) || parsed == 0U ||
            parsed > 4096U) {
            json_error(output, size, "invalid capacity", capacity_text);
            return 400;
        }
        capacity = (size_t)parsed;
    }
    status = hil_web_session_reconfigure_bus(g_session, loss, delay, tamper,
                                             capacity);
    if (status != HIL_OK) {
        json_error(output, size, "reconfigure failed",
                   hil_status_name(status));
        return 400;
    }
    return api_state(request, output, size);
}

int web_api_dispatch(const web_request_t *request, char *body,
                     size_t body_size)
{
    if (request == NULL || body == NULL || body_size == 0U) {
        return -1;
    }
    if (g_session == NULL || !g_session->ready) {
        json_error(body, body_size, "session not ready", NULL);
        return 500;
    }
    if (strcmp(request->path, "/api/state") == 0) {
        return api_state(request, body, body_size);
    }
    if (strcmp(request->path, "/api/set") == 0) {
        return api_set(request, body, body_size);
    }
    if (strcmp(request->path, "/api/wait") == 0) {
        return api_wait(request, body, body_size);
    }
    if (strcmp(request->path, "/api/fault") == 0) {
        return api_fault(request, body, body_size);
    }
    if (strcmp(request->path, "/api/fault/clear") == 0) {
        return api_fault_clear(request, body, body_size);
    }
    if (strcmp(request->path, "/api/ecufault") == 0) {
        return api_ecu_fault(request, body, body_size);
    }
    if (strcmp(request->path, "/api/power") == 0) {
        return api_power(request, body, body_size);
    }
    if (strcmp(request->path, "/api/reset") == 0) {
        return api_reset(request, body, body_size);
    }
    if (strcmp(request->path, "/api/assert") == 0) {
        return api_assert(request, body, body_size);
    }
    if (strcmp(request->path, "/api/history") == 0) {
        return api_history(request, body, body_size);
    }
    if (strcmp(request->path, "/api/script") == 0) {
        return api_script(request, body, body_size);
    }
    if (strcmp(request->path, "/api/report") == 0) {
        return api_report(request, body, body_size);
    }
    if (strcmp(request->path, "/api/config") == 0) {
        return api_config(request, body, body_size);
    }
    json_error(body, body_size, "unknown endpoint", request->path);
    return 404;
}
