#include "hil_assert.h"

#include <math.h>
#include <stdio.h>
#include <string.h> 

static const char *const op_names[] = {
    [HIL_ASSERT_EQ] = "EQ",
    [HIL_ASSERT_NE] = "NE",
    [HIL_ASSERT_LT] = "LT",
    [HIL_ASSERT_LE] = "LE",
    [HIL_ASSERT_GT] = "GT",
    [HIL_ASSERT_GE] = "GE",
    [HIL_ASSERT_BETWEEN] = "BETWEEN"
};

const char *hil_assert_op_name(hil_assert_op_t op)
{
    if (op < HIL_ASSERT_EQ || op > HIL_ASSERT_BETWEEN) {
        return "UNKNOWN";
    }
    return op_names[op];
}

bool hil_assert_op_from_name(const char *name, hil_assert_op_t *op)
{
    size_t index;
    if (name == NULL || op == NULL) {
        return false;
    }
    for (index = 0U; index < HIL_ARRAY_LEN(op_names); index++) {
        if (strcmp(name, op_names[index]) == 0) {
            *op = (hil_assert_op_t)index;
            return true;
        }
    }
    return false;
}

static bool compare_double(double actual, hil_assert_op_t op, double expected,
                           double tolerance)
{
    double delta = fabs(actual - expected);
    switch (op) {
    case HIL_ASSERT_EQ:
        return delta <= tolerance;
    case HIL_ASSERT_NE:
        return delta > tolerance;
    case HIL_ASSERT_LT:
        return actual < expected;
    case HIL_ASSERT_LE:
        return actual <= expected;
    case HIL_ASSERT_GT:
        return actual > expected;
    case HIL_ASSERT_GE:
        return actual >= expected;
    case HIL_ASSERT_BETWEEN:
        return false;
    }
    return false;
}

hil_status_t hil_assert_check_double(double actual, hil_assert_op_t op,
                                     double expected, double tolerance,
                                     hil_assert_result_t *result)
{
    if (result == NULL) {
        return HIL_ERR_NULL;
    }
    if (!hil_double_is_finite(actual) || !hil_double_is_finite(expected) ||
        tolerance < 0.0 || !hil_double_is_finite(tolerance) ||
        op < HIL_ASSERT_EQ || op > HIL_ASSERT_BETWEEN) {
        (void)snprintf(result->message, sizeof(result->message),
                       "invalid assertion arguments");
        result->passed = false;
        return HIL_ERR_VALUE;
    }
    result->passed = compare_double(actual, op, expected, tolerance);
    (void)snprintf(result->expected, sizeof(result->expected), "%s %.6f",
                   hil_assert_op_name(op), expected);
    (void)snprintf(result->actual, sizeof(result->actual), "%.6f", actual);
    (void)snprintf(result->message, sizeof(result->message), "%s",
                   result->passed ? "assertion passed" : "assertion failed");
    return result->passed ? HIL_OK : HIL_ERR_ASSERT;
}

hil_status_t hil_assert_check_double_range(double actual, double lower,
                                           double upper, double tolerance,
                                           hil_assert_result_t *result)
{
    bool passed;
    if (result == NULL) {
        return HIL_ERR_NULL;
    }
    if (!hil_double_is_finite(actual) || !hil_double_is_finite(lower) ||
        !hil_double_is_finite(upper) || lower > upper || tolerance < 0.0 ||
        !hil_double_is_finite(tolerance)) {
        (void)snprintf(result->message, sizeof(result->message),
                       "invalid range assertion arguments");
        result->passed = false;
        return HIL_ERR_VALUE;
    }
    passed = actual >= lower - tolerance && actual <= upper + tolerance;
    result->passed = passed;
    (void)snprintf(result->expected, sizeof(result->expected),
                   "BETWEEN %.6f %.6f", lower, upper);
    (void)snprintf(result->actual, sizeof(result->actual), "%.6f", actual);
    (void)snprintf(result->message, sizeof(result->message), "%s",
                   passed ? "assertion passed" : "assertion failed");
    return passed ? HIL_OK : HIL_ERR_ASSERT;
}
