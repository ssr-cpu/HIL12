#include "hil_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const status_names[] = {
    [HIL_REPORT_PASS] = "PASS",
    [HIL_REPORT_FAIL] = "FAIL",
    [HIL_REPORT_SKIP] = "SKIP",
    [HIL_REPORT_ERROR] = "ERROR"
};

const char *hil_report_status_name(hil_report_status_t status)
{
    if (status < HIL_REPORT_PASS || status > HIL_REPORT_ERROR) {
        return "UNKNOWN";
    }
    return status_names[status];
}

void hil_report_init(hil_report_builder_t *report)
{
    if (report == NULL) {
        return;
    }
    (void)memset(report, 0, sizeof(*report));
    report->current_case[0] = '\0';
}

void hil_report_deinit(hil_report_builder_t *report)
{
    hil_report_step_t *step;
    if (report == NULL) {
        return;
    }
    step = report->steps;
    while (step != NULL) {
        hil_report_step_t *next = step->next;
        free(step);
        step = next;
    }
    report->steps = NULL;
    report->steps_tail = NULL;
    report->step_count = 0U;
    report->passed = 0U;
    report->failed = 0U;
    report->skipped = 0U;
    report->errors = 0U;
    report->current_case[0] = '\0';
}

hil_status_t hil_report_begin_case(hil_report_builder_t *report,
                                   const char *case_name)
{
    if (report == NULL || case_name == NULL || case_name[0] == '\0') {
        return HIL_ERR_NULL;
    }
    if (strlen(case_name) >= sizeof(report->current_case)) {
        return HIL_ERR_OVERFLOW;
    }
    (void)strncpy(report->current_case, case_name,
                  sizeof(report->current_case) - 1U);
    report->current_case[sizeof(report->current_case) - 1U] = '\0';
    return HIL_OK;
}

hil_status_t hil_report_add_step(hil_report_builder_t *report, int line,
                                 hil_step_type_t type, const char *target,
                                 const char *expected, const char *actual,
                                 hil_report_status_t status,
                                 const char *message)
{
    hil_report_step_t *step;
    if (report == NULL) {
        return HIL_ERR_NULL;
    }
    step = (hil_report_step_t *)calloc(1U, sizeof(*step));
    if (step == NULL) {
        return HIL_ERR_NOMEM;
    }
    (void)strncpy(step->case_name, report->current_case,
                  sizeof(step->case_name) - 1U);
    step->line = line;
    step->step_type = type;
    if (target != NULL) {
        (void)strncpy(step->target, target, sizeof(step->target) - 1U);
    }
    if (expected != NULL) {
        (void)strncpy(step->expected, expected, sizeof(step->expected) - 1U);
    }
    if (actual != NULL) {
        (void)strncpy(step->actual, actual, sizeof(step->actual) - 1U);
    }
    if (message != NULL) {
        (void)strncpy(step->message, message, sizeof(step->message) - 1U);
    }
    step->status = status;
    if (report->steps_tail == NULL) {
        report->steps = step;
    } else {
        report->steps_tail->next = step;
    }
    report->steps_tail = step;
    report->step_count++;
    switch (status) {
    case HIL_REPORT_PASS:
        report->passed++;
        break;
    case HIL_REPORT_FAIL:
        report->failed++;
        break;
    case HIL_REPORT_SKIP:
        report->skipped++;
        break;
    case HIL_REPORT_ERROR:
        report->errors++;
        break;
    }
    return HIL_OK;
}

static void fprintf_cell(FILE *fp, const char *text)
{
    const char *cursor;
    (void)fputc('|', fp);
    (void)fputc(' ', fp);
    if (text != NULL) {
        for (cursor = text; *cursor != '\0'; cursor++) {
            if (*cursor == '|') {
                (void)fputc('\\', fp);
            }
            (void)fputc((unsigned char)*cursor, fp);
        }
    }
    (void)fputc(' ', fp);
}

hil_status_t hil_report_write_markdown(const hil_report_builder_t *report,
                                       const char *path,
                                       const hil_logger_t *logger)
{
    FILE *fp;
    const hil_report_step_t *step;
    if (report == NULL || path == NULL) {
        return HIL_ERR_NULL;
    }
    fp = fopen(path, "w");
    if (fp == NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "report",
                        "cannot create report file: %s", path);
        return HIL_ERR_IO;
    }
    (void)fprintf(fp, "# HIL Test Report\n\n");
    (void)fprintf(fp, "| Metric | Value |\n|---|---:|\n");
    (void)fprintf(fp, "| Steps | %llu |\n",
                  (unsigned long long)report->step_count);
    (void)fprintf(fp, "| Passed | %llu |\n",
                  (unsigned long long)report->passed);
    (void)fprintf(fp, "| Failed | %llu |\n",
                  (unsigned long long)report->failed);
    (void)fprintf(fp, "| Skipped | %llu |\n",
                  (unsigned long long)report->skipped);
    (void)fprintf(fp, "| Errors | %llu |\n\n",
                  (unsigned long long)report->errors);

    (void)fprintf(fp,
                  "| Case | Line | Step | Target | Expected | Actual | Status | "
                  "Message |\n");
    (void)fprintf(fp,
                  "|---|---:|---|---|---|---|---|---|\n");
    for (step = report->steps; step != NULL; step = step->next) {
        fprintf_cell(fp, step->case_name);
        (void)fprintf(fp, "| %d ", step->line);
        fprintf_cell(fp, hil_step_type_name(step->step_type));
        fprintf_cell(fp, step->target);
        fprintf_cell(fp, step->expected);
        fprintf_cell(fp, step->actual);
        fprintf_cell(fp, hil_report_status_name(step->status));
        fprintf_cell(fp, step->message);
        (void)fprintf(fp, "|\n");
    }
    if (fclose(fp) != 0) {
        return HIL_ERR_IO;
    }
    return HIL_OK;
}

void hil_report_summary(const hil_report_builder_t *report, size_t *passed,
                        size_t *failed, size_t *skipped, size_t *errors)
{
    if (passed != NULL) {
        *passed = report != NULL ? report->passed : 0U;
    }
    if (failed != NULL) {
        *failed = report != NULL ? report->failed : 0U;
    }
    if (skipped != NULL) {
        *skipped = report != NULL ? report->skipped : 0U;
    }
    if (errors != NULL) {
        *errors = report != NULL ? report->errors : 0U;
    }
}

bool hil_report_all_passed(const hil_report_builder_t *report)
{
    return report != NULL && report->failed == 0U && report->errors == 0U;
}
