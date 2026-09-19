#include "hil_assert.h"
#include "hil_bus.h"
#include "hil_common.h"
#include "hil_config.h"
#include "hil_csv.h"
#include "hil_data_logger.h"
#include "hil_ecu.h"
#include "hil_executor.h"
#include "hil_fault.h"
#include "hil_frame.h"
#include "hil_logger.h"
#include "hil_report.h"
#include "hil_script.h"
#include "hil_signal.h"
#include "hil_time.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;
static hil_logger_t silent_logger;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            (void)fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,      \
                          #cond);                                               \
            tests_failed++;                                                     \
            return;                                                            \
        }                                                                      \
    } while (0)

#define CHECK_STATUS(actual, expected)                                         \
    do {                                                                       \
        hil_status_t check_status_ = (actual);                                 \
        if (check_status_ != (expected)) {                                     \
            (void)fprintf(stderr,                                              \
                          "FAIL %s:%d: expected %s, got %s\n", __FILE__,       \
                          __LINE__, hil_status_name(expected),                  \
                          hil_status_name(check_status_));                      \
            tests_failed++;                                                     \
            return;                                                            \
        }                                                                      \
    } while (0)

static void run_test(const char *name, void (*test_fn)(void))
{
    int before = tests_failed;
    tests_run++;
    test_fn();
    if (tests_failed == before) {
        (void)printf("PASS %s\n", name);
    } else {
        (void)printf("FAIL %s\n", name);
    }
}

static void test_status_names(void)
{
    CHECK(strcmp(hil_status_name(HIL_OK), "OK") == 0);
    CHECK(strcmp(hil_status_name(HIL_ERR_NULL), "NULL_POINTER") == 0);
    CHECK(strcmp(hil_status_name(HIL_EOF), "EOF") == 0);
}

static void test_parse_bool(void)
{
    bool value = false;
    CHECK(hil_parse_bool("true", &value) && value);
    CHECK(hil_parse_bool("no", &value) && !value);
    CHECK(!hil_parse_bool("maybe", &value));
}

static void test_parse_u64(void)
{
    uint64_t value = 0U;
    CHECK(hil_parse_u64("12345", &value) && value == 12345U);
    CHECK(!hil_parse_u64("12x", &value));
    CHECK(!hil_parse_u64("-1", &value));
}

static void test_parse_double(void)
{
    double value = 0.0;
    CHECK(hil_parse_double("12.5", &value) && fabs(value - 12.5) < 1e-9);
    CHECK(!hil_parse_double("1e999", &value));
    CHECK(!hil_parse_double("", &value));
}

static void test_config_defaults(void)
{
    hil_config_t config;
    hil_config_init(&config);
    CHECK(config.tick_ms == 10U);
    CHECK(config.bus_queue_capacity == 64U);
    CHECK(strcmp(config.output_dir, "out") == 0);
}

static void test_config_load_valid(void)
{
    hil_config_t config;
    hil_config_init(&config);
    CHECK_STATUS(hil_config_load(&config, "examples/default_config.ini",
                                 &silent_logger),
                 HIL_OK);
    CHECK(config.tick_ms == 10U);
    CHECK(config.bus_queue_capacity == 64U);
}

static void test_config_load_invalid(void)
{
    hil_config_t config;
    FILE *fp = fopen("build/test_invalid_config.ini", "w");
    CHECK(fp != NULL);
    (void)fputs("runtime.tick_ms=0\n", fp);
    (void)fclose(fp);
    hil_config_init(&config);
    CHECK(hil_config_load(&config, "build/test_invalid_config.ini",
                          &silent_logger) != HIL_OK);
    (void)remove("build/test_invalid_config.ini");
}

static void test_time_advance(void)
{
    hil_virtual_time_t time;
    hil_time_init(&time);
    CHECK_STATUS(hil_time_advance(&time, 25U), HIL_OK);
    CHECK(hil_time_now(&time) == 25U);
    CHECK(hil_time_elapsed(&time, 10U) == 15U);
}

static void test_time_overflow(void)
{
    hil_virtual_time_t time;
    hil_time_init(&time);
    time.now_ms = UINT64_MAX;
    CHECK_STATUS(hil_time_advance(&time, 1U), HIL_ERR_OVERFLOW);
}

static void test_signal_add_find(void)
{
    hil_signal_registry_t registry;
    hil_signal_t signal;
    hil_signal_registry_init(&registry);
    hil_signal_init(&signal, "speed", "rpm", 0.0, 8000.0, 100.0);
    CHECK_STATUS(hil_signal_registry_add(&registry, &signal, &silent_logger),
                 HIL_OK);
    CHECK(hil_signal_registry_find(&registry, "speed") != NULL);
    CHECK(hil_signal_registry_find(&registry, "missing") == NULL);
    hil_signal_registry_deinit(&registry);
}

static void test_signal_duplicate(void)
{
    hil_signal_registry_t registry;
    hil_signal_t signal;
    hil_signal_registry_init(&registry);
    hil_signal_init(&signal, "speed", "rpm", 0.0, 8000.0, 100.0);
    CHECK_STATUS(hil_signal_registry_add(&registry, &signal, &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_signal_registry_add(&registry, &signal, &silent_logger),
                 HIL_ERR_DUPLICATE);
    hil_signal_registry_deinit(&registry);
}

static void test_signal_set_reset(void)
{
    hil_signal_registry_t registry;
    hil_signal_t signal;
    hil_signal_registry_init(&registry);
    hil_signal_init(&signal, "speed", "rpm", 0.0, 8000.0, 100.0);
    (void)hil_signal_registry_add(&registry, &signal, &silent_logger);
    CHECK_STATUS(hil_signal_registry_set(&registry, "speed", 250.0,
                                         &silent_logger),
                 HIL_OK);
    CHECK(fabs(hil_signal_registry_find(&registry, "speed")->value - 250.0) <
          1e-9);
    CHECK_STATUS(hil_signal_registry_reset(&registry), HIL_OK);
    CHECK(fabs(hil_signal_registry_find(&registry, "speed")->value - 100.0) <
          1e-9);
    hil_signal_registry_deinit(&registry);
}

static void test_signal_load_csv(void)
{
    hil_signal_registry_t registry;
    hil_signal_registry_init(&registry);
    CHECK_STATUS(hil_signal_registry_load_csv(&registry, "examples/signals.csv",
                                              &silent_logger),
                 HIL_OK);
    CHECK(hil_signal_registry_count(&registry) >= 8U);
    CHECK(hil_signal_registry_find(&registry, "engine_speed") != NULL);
    hil_signal_registry_deinit(&registry);
}

static void test_frame_crc_and_payload(void)
{
    hil_frame_t frame;
    uint8_t out[2] = {0U, 0U};
    hil_frame_init(&frame, 0x123U, 8U, 10U, 1U, 0U);
    CHECK(hil_frame_is_crc_valid(&frame));
    CHECK_STATUS(hil_frame_set_payload(&frame, 2U, out, 2U), HIL_OK);
    CHECK(frame.data[2] == 0U);
    CHECK_STATUS(hil_frame_get_payload(&frame, 2U, out, 2U), HIL_OK);
}

static void test_frame_encode_decode(void)
{
    hil_frame_t frame;
    double decoded = 0.0;
    hil_frame_init(&frame, 0x100U, 4U, 0U, 1U, 0U);
    CHECK_STATUS(hil_frame_encode_double(&frame, 0U, 1234.5, 0.1, 0.0),
                 HIL_OK);
    CHECK_STATUS(hil_frame_decode_double(&frame, 0U, 0.1, 0.0, &decoded),
                 HIL_OK);
    CHECK(fabs(decoded - 1234.5) < 1e-6);
}

static void test_frame_encode_overflow(void)
{
    hil_frame_t frame;
    hil_frame_init(&frame, 0x100U, 2U, 0U, 1U, 0U);
    CHECK_STATUS(hil_frame_encode_double(&frame, 0U, 1.0, 0.1, 0.0), HIL_OK);
    CHECK(hil_frame_encode_double(&frame, 7U, 1.0, 0.1, 0.0) != HIL_OK);
}

static void test_frame_string_roundtrip(void)
{
    hil_frame_t frame;
    hil_frame_t decoded;
    char text[HIL_FRAME_TEXT_SIZE];
    hil_frame_init(&frame, 0x321U, 3U, 999U, 77U, 5U);
    frame.data[0] = 1U;
    frame.data[1] = 2U;
    frame.data[2] = 3U;
    hil_frame_refresh_crc(&frame);
    CHECK_STATUS(hil_frame_to_string(&frame, text, sizeof(text)), HIL_OK);
    CHECK_STATUS(hil_frame_from_string(&decoded, text), HIL_OK);
    CHECK(decoded.id == frame.id);
    CHECK(decoded.dlc == frame.dlc);
    CHECK(memcmp(decoded.data, frame.data, frame.dlc) == 0);
}

static void test_bus_publish_poll(void)
{
    hil_bus_t bus;
    hil_frame_t frame;
    uint8_t data[2] = {1U, 2U};
    hil_bus_init(&bus, 4U, 0U, 0U, false, 1U);
    hil_bus_set_time(&bus, 5U);
    CHECK_STATUS(hil_bus_publish(&bus, 0x10U, 2U, data, 5U, &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_bus_poll(&bus, &frame, &silent_logger), HIL_OK);
    CHECK(frame.id == 0x10U);
    CHECK(frame.sequence == 1U);
    hil_bus_deinit(&bus);
}

static void test_bus_queue_full(void)
{
    hil_bus_t bus;
    uint8_t data[1] = {0U};
    unsigned int i;
    hil_bus_init(&bus, 2U, 0U, 0U, false, 1U);
    for (i = 0U; i < 3U; i++) {
        hil_status_t status =
            hil_bus_publish(&bus, (uint32_t)i, 1U, data, 0U, &silent_logger);
        if (i < 2U) {
            CHECK_STATUS(status, HIL_OK);
        } else {
            CHECK_STATUS(status, HIL_ERR_BUSY);
        }
    }
    hil_bus_deinit(&bus);
}

static void test_bus_delay(void)
{
    hil_bus_t bus;
    hil_frame_t frame;
    uint8_t data[1] = {0U};
    hil_bus_init(&bus, 2U, 0U, 20U, false, 1U);
    (void)hil_bus_publish(&bus, 0x10U, 1U, data, 0U, &silent_logger);
    hil_bus_set_time(&bus, 10U);
    CHECK_STATUS(hil_bus_poll(&bus, &frame, &silent_logger),
                 HIL_ERR_TIMEOUT);
    hil_bus_set_time(&bus, 25U);
    CHECK_STATUS(hil_bus_poll(&bus, &frame, &silent_logger), HIL_OK);
    hil_bus_deinit(&bus);
}

static void test_bus_loss(void)
{
    hil_bus_t bus;
    hil_frame_t frame;
    uint8_t data[1] = {0U};
    hil_bus_init(&bus, 2U, 100U, 0U, false, 1U);
    (void)hil_bus_publish(&bus, 0x10U, 1U, data, 0U, &silent_logger);
    CHECK_STATUS(hil_bus_poll(&bus, &frame, &silent_logger),
                 HIL_ERR_FAULT);
    CHECK(bus.total_lost == 1U);
    hil_bus_deinit(&bus);
}

static void test_bus_tamper(void)
{
    hil_bus_t bus;
    hil_frame_t frame;
    uint8_t data[1] = {0U};
    unsigned int i;
    hil_bus_init(&bus, 32U, 0U, 0U, true, 7U);
    for (i = 0U; i < 32U; i++) {
        (void)hil_bus_publish(&bus, (uint32_t)i, 1U, data, (uint64_t)i,
                              &silent_logger);
        hil_bus_set_time(&bus, (uint64_t)i);
        while (hil_bus_poll(&bus, &frame, &silent_logger) == HIL_OK) {
        }
    }
    CHECK(bus.total_tampered > 0U);
    hil_bus_deinit(&bus);
}

static void test_fault_stuck(void)
{
    hil_fault_manager_t manager;
    hil_signal_registry_t registry;
    hil_signal_t signal;
    hil_fault_spec_t spec;
    hil_fault_manager_init(&manager, 1U);
    hil_signal_registry_init(&registry);
    hil_signal_init(&signal, "speed", "rpm", 0.0, 8000.0, 1000.0);
    (void)hil_signal_registry_add(&registry, &signal, &silent_logger);
    (void)memset(&spec, 0, sizeof(spec));
    (void)strncpy(spec.signal_name, "speed", sizeof(spec.signal_name) - 1U);
    spec.type = HIL_FAULT_STUCK;
    spec.value = 2000.0;
    spec.start_ms = 10U;
    CHECK_STATUS(hil_fault_manager_add(&manager, &spec, &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_fault_manager_apply(&manager, &registry, 20U,
                                         &silent_logger),
                 HIL_OK);
    CHECK(fabs(hil_signal_registry_find(&registry, "speed")->value - 2000.0) <
          1e-9);
    hil_fault_manager_deinit(&manager);
    hil_signal_registry_deinit(&registry);
}

static void test_fault_open_circuit(void)
{
    hil_fault_manager_t manager;
    hil_signal_registry_t registry;
    hil_signal_t signal;
    hil_fault_spec_t spec;
    hil_fault_manager_init(&manager, 1U);
    hil_signal_registry_init(&registry);
    hil_signal_init(&signal, "temp", "degC", -40.0, 140.0, 25.0);
    (void)hil_signal_registry_add(&registry, &signal, &silent_logger);
    (void)memset(&spec, 0, sizeof(spec));
    (void)strncpy(spec.signal_name, "temp", sizeof(spec.signal_name) - 1U);
    spec.type = HIL_FAULT_OPEN_CIRCUIT;
    spec.start_ms = 0U;
    CHECK_STATUS(hil_fault_manager_add(&manager, &spec, &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_fault_manager_apply(&manager, &registry, 5U,
                                         &silent_logger),
                 HIL_OK);
    CHECK(!hil_signal_registry_find(&registry, "temp")->valid);
    CHECK(hil_signal_registry_find(&registry, "temp")->open_circuit);
    hil_fault_manager_deinit(&manager);
    hil_signal_registry_deinit(&registry);
}

static void test_fault_remove(void)
{
    hil_fault_manager_t manager;
    hil_fault_spec_t spec;
    hil_fault_manager_init(&manager, 1U);
    (void)memset(&spec, 0, sizeof(spec));
    (void)strncpy(spec.signal_name, "speed", sizeof(spec.signal_name) - 1U);
    spec.type = HIL_FAULT_STUCK;
    spec.value = 2000.0;
    CHECK_STATUS(hil_fault_manager_add(&manager, &spec, &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_fault_manager_remove(&manager, "speed",
                                          HIL_FAULT_STUCK),
                 HIL_OK);
    CHECK(!hil_fault_manager_is_active(&manager, "speed", 0U));
    hil_fault_manager_deinit(&manager);
}

static void test_assert_pass_and_fail(void)
{
    hil_assert_result_t result;
    CHECK_STATUS(hil_assert_check_double(12.5, HIL_ASSERT_EQ, 12.5, 0.001,
                                         &result),
                 HIL_OK);
    CHECK(result.passed);
    CHECK(hil_assert_check_double(12.4, HIL_ASSERT_EQ, 12.5, 0.001,
                                  &result) == HIL_ERR_ASSERT);
    CHECK(!result.passed);
    CHECK_STATUS(hil_assert_check_double_range(12.4, 12.0, 12.5, 0.0,
                                               &result),
                 HIL_OK);
    CHECK(result.passed);
}

static void test_csv_roundtrip(void)
{
    hil_csv_writer_t writer;
    hil_csv_reader_t reader;
    const char *fields[3] = {"a,b", "say \"hi\"", "plain"};
    CHECK_STATUS(hil_csv_writer_open(&writer, "build/test_quoted.csv", 3U,
                                     &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_csv_writer_write_row(&writer, fields, 3U, &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_csv_writer_close(&writer, &silent_logger), HIL_OK);
    CHECK_STATUS(hil_csv_reader_open(&reader, "build/test_quoted.csv",
                                     &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_csv_reader_read_row(&reader, &silent_logger), HIL_OK);
    CHECK(strcmp(hil_csv_reader_field(&reader, 0U), "a,b") == 0);
    CHECK(strcmp(hil_csv_reader_field(&reader, 1U), "say \"hi\"") == 0);
    hil_csv_reader_close(&reader);
    (void)remove("build/test_quoted.csv");
}

static void test_csv_malformed(void)
{
    hil_csv_reader_t reader;
    FILE *fp = fopen("build/test_bad.csv", "w");
    CHECK(fp != NULL);
    (void)fputs("a,\"bad\n", fp);
    (void)fclose(fp);
    CHECK_STATUS(hil_csv_reader_open(&reader, "build/test_bad.csv",
                                     &silent_logger),
                 HIL_OK);
    CHECK(hil_csv_reader_read_row(&reader, &silent_logger) == HIL_ERR_FORMAT);
    hil_csv_reader_close(&reader);
    (void)remove("build/test_bad.csv");
}

static void test_script_valid(void)
{
    hil_script_t script;
    hil_script_init(&script);
    CHECK_STATUS(
        hil_script_load_text(
            &script,
            "TEST demo\nSET throttle_position 20\nWAIT 100\nASSERT "
            "throttle_position EQ 20 0.1\nRESET\nEND\n",
            &silent_logger),
        HIL_OK);
    CHECK(script.case_count == 1U);
    CHECK(script.cases->step_count == 5U);
    hil_script_deinit(&script);
}

static void test_script_unknown_command(void)
{
    hil_script_t script;
    hil_script_init(&script);
    CHECK(hil_script_load_text(&script, "TEST demo\nBOGUS 1\nEND\n",
                               &silent_logger) != HIL_OK);
    hil_script_deinit(&script);
}

static void test_script_missing_end(void)
{
    hil_script_t script;
    hil_script_init(&script);
    CHECK(hil_script_load_text(&script, "TEST demo\nWAIT 10\n",
                               &silent_logger) != HIL_OK);
    hil_script_deinit(&script);
}

static void test_script_duplicate_case(void)
{
    hil_script_t script;
    hil_script_init(&script);
    CHECK(hil_script_load_text(&script,
                               "TEST demo\nEND\nTEST demo\nEND\n",
                               &silent_logger) != HIL_OK);
    hil_script_deinit(&script);
}

static void test_report_builder(void)
{
    hil_report_builder_t report;
    hil_report_init(&report);
    CHECK_STATUS(hil_report_begin_case(&report, "demo"), HIL_OK);
    CHECK_STATUS(hil_report_add_step(&report, 1, HIL_STEP_ASSERT, "speed",
                                     "EQ 10", "10", HIL_REPORT_PASS, "ok"),
                 HIL_OK);
    CHECK(hil_report_all_passed(&report));
    CHECK_STATUS(hil_report_write_markdown(&report,
                                           "build/test_report_unit.md",
                                           &silent_logger),
                 HIL_OK);
    hil_report_deinit(&report);
    (void)remove("build/test_report_unit.md");
}

static void test_executor_end_to_end(void)
{
    hil_config_t config;
    hil_signal_registry_t signals;
    hil_data_logger_t logger;
    hil_report_builder_t report;
    hil_executor_t executor;
    hil_config_init(&config);
    config.tick_ms = 10U;
    config.bus_loss_probability_percent = 0U;
    config.bus_delay_ms = 0U;
    config.bus_tamper_enabled = false;
    hil_signal_registry_init(&signals);
    CHECK_STATUS(hil_signal_registry_load_csv(&signals, "examples/signals.csv",
                                              &silent_logger),
                 HIL_OK);
    hil_report_init(&report);
    CHECK_STATUS(hil_data_logger_open(&logger, "build/test_run.csv", &signals,
                                      &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_executor_init(&executor, &config, &signals,
                                   "examples/e2e_suite.hil", &logger, &report,
                                   &silent_logger),
                 HIL_OK);
    CHECK(executor.script.case_count == 12U);
    CHECK_STATUS(hil_executor_run(&executor, &silent_logger), HIL_OK);
    hil_executor_deinit(&executor);
    CHECK_STATUS(hil_data_logger_close(&logger, &silent_logger), HIL_OK);
    CHECK(hil_report_all_passed(&report));
    CHECK_STATUS(hil_report_write_markdown(&report, "build/test_report.md",
                                           &silent_logger),
                 HIL_OK);
    CHECK_STATUS(hil_data_logger_query("build/test_run.csv", "engine_speed",
                                       200U, 400U, &silent_logger),
                 HIL_OK);
    hil_report_deinit(&report);
    hil_signal_registry_deinit(&signals);
    (void)remove("build/test_run.csv");
    (void)remove("build/test_report.md");
}

int main(void)
{
    hil_logger_init(&silent_logger, stderr, HIL_LOG_OFF);
    run_test("status_names", test_status_names);
    run_test("parse_bool", test_parse_bool);
    run_test("parse_u64", test_parse_u64);
    run_test("parse_double", test_parse_double);
    run_test("config_defaults", test_config_defaults);
    run_test("config_load_valid", test_config_load_valid);
    run_test("config_load_invalid", test_config_load_invalid);
    run_test("time_advance", test_time_advance);
    run_test("time_overflow", test_time_overflow);
    run_test("signal_add_find", test_signal_add_find);
    run_test("signal_duplicate", test_signal_duplicate);
    run_test("signal_set_reset", test_signal_set_reset);
    run_test("signal_load_csv", test_signal_load_csv);
    run_test("frame_crc_and_payload", test_frame_crc_and_payload);
    run_test("frame_encode_decode", test_frame_encode_decode);
    run_test("frame_encode_overflow", test_frame_encode_overflow);
    run_test("frame_string_roundtrip", test_frame_string_roundtrip);
    run_test("bus_publish_poll", test_bus_publish_poll);
    run_test("bus_queue_full", test_bus_queue_full);
    run_test("bus_delay", test_bus_delay);
    run_test("bus_loss", test_bus_loss);
    run_test("bus_tamper", test_bus_tamper);
    run_test("fault_stuck", test_fault_stuck);
    run_test("fault_open_circuit", test_fault_open_circuit);
    run_test("fault_remove", test_fault_remove);
    run_test("assert_pass_and_fail", test_assert_pass_and_fail);
    run_test("csv_roundtrip", test_csv_roundtrip);
    run_test("csv_malformed", test_csv_malformed);
    run_test("script_valid", test_script_valid);
    run_test("script_unknown_command", test_script_unknown_command);
    run_test("script_missing_end", test_script_missing_end);
    run_test("script_duplicate_case", test_script_duplicate_case);
    run_test("report_builder", test_report_builder);
    run_test("executor_end_to_end", test_executor_end_to_end);

    (void)printf("Total: %d, Failed: %d\n", tests_run, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
