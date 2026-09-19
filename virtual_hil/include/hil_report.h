#ifndef HIL_REPORT_H
#define HIL_REPORT_H

#include <stdbool.h>
#include <stddef.h>

#include "hil_common.h"
#include "hil_logger.h"
#include "hil_script.h"

typedef enum hil_report_status {
    HIL_REPORT_PASS = 0,
    HIL_REPORT_FAIL,
    HIL_REPORT_SKIP,
    HIL_REPORT_ERROR
} hil_report_status_t;

typedef struct hil_report_step {
    char case_name[HIL_NAME_MAX];
    int line;
    hil_step_type_t step_type;
    char target[HIL_NAME_MAX];
    char expected[HIL_MESSAGE_MAX];
    char actual[HIL_MESSAGE_MAX];
    hil_report_status_t status;
    char message[HIL_MESSAGE_MAX];
    struct hil_report_step *next;
} hil_report_step_t;

typedef struct hil_report_builder {
    hil_report_step_t *steps;
    hil_report_step_t *steps_tail;
    size_t step_count;
    size_t passed;
    size_t failed;
    size_t skipped;
    size_t errors;
    char current_case[HIL_NAME_MAX];
} hil_report_builder_t;

const char *hil_report_status_name(hil_report_status_t status);

void hil_report_init(hil_report_builder_t *report);
void hil_report_deinit(hil_report_builder_t *report);
hil_status_t hil_report_begin_case(hil_report_builder_t *report,
                                   const char *case_name);
hil_status_t hil_report_add_step(hil_report_builder_t *report, int line,
                                 hil_step_type_t type, const char *target,
                                 const char *expected, const char *actual,
                                 hil_report_status_t status,
                                 const char *message);
hil_status_t hil_report_write_markdown(const hil_report_builder_t *report,
                                       const char *path,
                                       const hil_logger_t *logger);
void hil_report_summary(const hil_report_builder_t *report, size_t *passed,
                        size_t *failed, size_t *skipped, size_t *errors);
bool hil_report_all_passed(const hil_report_builder_t *report);

#endif
