#ifndef HIL_ASSERT_H
#define HIL_ASSERT_H

#include <stdbool.h>

#include "hil_common.h"

typedef enum hil_assert_op {
    HIL_ASSERT_EQ = 0,
    HIL_ASSERT_NE,
    HIL_ASSERT_LT,
    HIL_ASSERT_LE,
    HIL_ASSERT_GT,
    HIL_ASSERT_GE,
    HIL_ASSERT_BETWEEN
} hil_assert_op_t;

typedef struct hil_assert_result {
    bool passed;
    char expected[HIL_MESSAGE_MAX];
    char actual[HIL_MESSAGE_MAX];
    char message[HIL_MESSAGE_MAX];
} hil_assert_result_t;

const char *hil_assert_op_name(hil_assert_op_t op);
bool hil_assert_op_from_name(const char *name, hil_assert_op_t *op);
hil_status_t hil_assert_check_double(double actual, hil_assert_op_t op,
                                     double expected, double tolerance,
                                     hil_assert_result_t *result);
hil_status_t hil_assert_check_double_range(double actual, double lower,
                                           double upper, double tolerance,
                                           hil_assert_result_t *result);

#endif
