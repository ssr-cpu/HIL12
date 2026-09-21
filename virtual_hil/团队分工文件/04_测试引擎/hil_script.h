#ifndef HIL_SCRIPT_H
#define HIL_SCRIPT_H

#include <stddef.h> 

#include "hil_assert.h"
#include "hil_common.h"
#include "hil_fault.h"
#include "hil_logger.h"

typedef enum hil_step_type {
    HIL_STEP_NONE = 0,
    HIL_STEP_TEST,
    HIL_STEP_SET,
    HIL_STEP_WAIT,
    HIL_STEP_FAULT,
    HIL_STEP_ASSERT,
    HIL_STEP_RESET,
    HIL_STEP_END
} hil_step_type_t;

typedef struct hil_step {
    hil_step_type_t type;
    int line;
    char target[HIL_NAME_MAX];
    char argument[HIL_NAME_MAX];
    double value;
    double second_value;
    double tolerance;
    hil_assert_op_t assert_op;
    hil_fault_type_t fault_type;
    struct hil_step *next;
} hil_step_t;

typedef struct hil_test_case {
    char name[HIL_NAME_MAX];
    hil_step_t *steps;
    hil_step_t *steps_tail;
    size_t step_count;
    struct hil_test_case *next;
} hil_test_case_t;

typedef struct hil_script {
    hil_test_case_t *cases;
    hil_test_case_t *cases_tail;
    size_t case_count;
} hil_script_t;

const char *hil_step_type_name(hil_step_type_t type);

void hil_script_init(hil_script_t *script);
void hil_script_deinit(hil_script_t *script);
hil_status_t hil_script_load_file(hil_script_t *script, const char *path,
                                  const hil_logger_t *logger);
hil_status_t hil_script_load_text(hil_script_t *script, const char *text,
                                  const hil_logger_t *logger);
hil_status_t hil_script_append_test(hil_script_t *script,
                                    const char *name);
hil_status_t hil_script_append_step(hil_script_t *script,
                                    const hil_step_t *step);

#endif
