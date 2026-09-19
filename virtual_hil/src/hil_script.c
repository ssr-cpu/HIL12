#include "hil_script.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const step_type_names[] = {
    [HIL_STEP_NONE] = "NONE",
    [HIL_STEP_TEST] = "TEST",
    [HIL_STEP_SET] = "SET",
    [HIL_STEP_WAIT] = "WAIT",
    [HIL_STEP_FAULT] = "FAULT",
    [HIL_STEP_ASSERT] = "ASSERT",
    [HIL_STEP_RESET] = "RESET",
    [HIL_STEP_END] = "END"
};

const char *hil_step_type_name(hil_step_type_t type)
{
    if (type < HIL_STEP_NONE || type > HIL_STEP_END) {
        return "UNKNOWN";
    }
    return step_type_names[type];
}

void hil_script_init(hil_script_t *script)
{
    if (script != NULL) {
        script->cases = NULL;
        script->cases_tail = NULL;
        script->case_count = 0U;
    }
}

void hil_script_deinit(hil_script_t *script)
{
    hil_test_case_t *test;
    if (script == NULL) {
        return;
    }
    test = script->cases;
    while (test != NULL) {
        hil_test_case_t *next_test = test->next;
        hil_step_t *step = test->steps;
        while (step != NULL) {
            hil_step_t *next_step = step->next;
            free(step);
            step = next_step;
        }
        free(test);
        test = next_test;
    }
    script->cases = NULL;
    script->cases_tail = NULL;
    script->case_count = 0U;
}

static char to_upper(char ch)
{
    if (ch >= 'a' && ch <= 'z') {
        return (char)(ch - 'a' + 'A');
    }
    return ch;
}

static bool token_equal(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (to_upper(*a) != to_upper(*b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static size_t tokenize(char *line, char **tokens, size_t max_tokens)
{
    size_t count = 0U;
    char *cursor = line;
    while (*cursor != '\0' && count < max_tokens) {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' ||
               *cursor == '\n') {
            cursor++;
        }
        if (*cursor == '\0' || *cursor == '#' || *cursor == ';') {
            break;
        }
        tokens[count++] = cursor;
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t' &&
               *cursor != '\r' && *cursor != '\n') {
            cursor++;
        }
        if (*cursor != '\0') {
            *cursor = '\0';
            cursor++;
        }
    }
    return count;
}

hil_status_t hil_script_append_test(hil_script_t *script, const char *name)
{
    hil_test_case_t *test;
    hil_test_case_t *existing;
    if (script == NULL || name == NULL || name[0] == '\0') {
        return HIL_ERR_NULL;
    }
    for (existing = script->cases; existing != NULL;
         existing = existing->next) {
        if (strcmp(existing->name, name) == 0) {
            return HIL_ERR_DUPLICATE;
        }
    }
    if (strlen(name) >= HIL_NAME_MAX) {
        return HIL_ERR_OVERFLOW;
    }
    test = (hil_test_case_t *)calloc(1U, sizeof(*test));
    if (test == NULL) {
        return HIL_ERR_NOMEM;
    }
    (void)strncpy(test->name, name, sizeof(test->name) - 1U);
    if (script->cases_tail == NULL) {
        script->cases = test;
    } else {
        script->cases_tail->next = test;
    }
    script->cases_tail = test;
    script->case_count++;
    return HIL_OK;
}

hil_status_t hil_script_append_step(hil_script_t *script,
                                    const hil_step_t *step)
{
    hil_step_t *copy;
    hil_test_case_t *test;
    if (script == NULL || step == NULL || script->cases_tail == NULL) {
        return HIL_ERR_STATE;
    }
    test = script->cases_tail;
    copy = (hil_step_t *)calloc(1U, sizeof(*copy));
    if (copy == NULL) {
        return HIL_ERR_NOMEM;
    }
    *copy = *step;
    copy->next = NULL;
    if (test->steps_tail == NULL) {
        test->steps = copy;
    } else {
        test->steps_tail->next = copy;
    }
    test->steps_tail = copy;
    test->step_count++;
    return HIL_OK;
}

static hil_status_t parse_test(hil_script_t *script, char **tokens,
                               size_t count, int line,
                               const hil_logger_t *logger)
{
    if (count != 2U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: TEST requires a name", line);
        return HIL_ERR_SCRIPT;
    }
    return hil_script_append_test(script, tokens[1]);
}

static hil_status_t parse_set(hil_script_t *script, char **tokens,
                              size_t count, int line,
                              const hil_logger_t *logger)
{
    hil_step_t step;
    if (count != 3U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: SET requires signal and value", line);
        return HIL_ERR_SCRIPT;
    }
    (void)memset(&step, 0, sizeof(step));
    step.type = HIL_STEP_SET;
    step.line = line;
    if (strlen(tokens[1]) >= sizeof(step.target)) {
        return HIL_ERR_OVERFLOW;
    }
    (void)strncpy(step.target, tokens[1], sizeof(step.target) - 1U);
    if (!hil_parse_double(tokens[2], &step.value)) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: invalid SET value", line);
        return HIL_ERR_SCRIPT;
    }
    return hil_script_append_step(script, &step);
}

static hil_status_t parse_wait(hil_script_t *script, char **tokens,
                               size_t count, int line,
                               const hil_logger_t *logger)
{
    hil_step_t step;
    uint64_t value;
    if (count != 2U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: WAIT requires milliseconds", line);
        return HIL_ERR_SCRIPT;
    }
    if (!hil_parse_u64(tokens[1], &value) || value == 0U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: invalid WAIT duration", line);
        return HIL_ERR_SCRIPT;
    }
    (void)memset(&step, 0, sizeof(step));
    step.type = HIL_STEP_WAIT;
    step.line = line;
    step.value = (double)value;
    return hil_script_append_step(script, &step);
}

static hil_status_t parse_fault(hil_script_t *script, char **tokens,
                                size_t count, int line,
                                const hil_logger_t *logger)
{
    hil_step_t step;
    uint64_t duration = 0U;
    if (count != 3U && count != 4U && count != 5U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: FAULT requires signal, type, value and "
                        "optional duration; OPEN_CIRCUIT/DROPOUT may omit "
                        "value", line);
        return HIL_ERR_SCRIPT;
    }
    (void)memset(&step, 0, sizeof(step));
    step.type = HIL_STEP_FAULT;
    step.line = line;
    if (strlen(tokens[1]) >= sizeof(step.target) ||
        strlen(tokens[2]) >= sizeof(step.argument)) {
        return HIL_ERR_OVERFLOW;
    }
    (void)strncpy(step.target, tokens[1], sizeof(step.target) - 1U);
    (void)strncpy(step.argument, tokens[2], sizeof(step.argument) - 1U);
    if (!hil_fault_type_from_name(step.argument, &step.fault_type)) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: unknown fault type '%s'", line,
                        step.argument);
        return HIL_ERR_SCRIPT;
    }
    if (count == 3U) {
        if (step.fault_type != HIL_FAULT_OPEN_CIRCUIT &&
            step.fault_type != HIL_FAULT_DROPOUT) {
            hil_log_message(logger, HIL_LOG_ERROR, "script",
                            "line %d: fault '%s' requires a value", line,
                            step.argument);
            return HIL_ERR_SCRIPT;
        }
        step.value = 0.0;
    } else {
        if (!hil_parse_double(tokens[3], &step.value)) {
            hil_log_message(logger, HIL_LOG_ERROR, "script",
                            "line %d: invalid fault value", line);
            return HIL_ERR_SCRIPT;
        }
    }
    if (count == 5U) {
        if (!hil_parse_u64(tokens[4], &duration)) {
            hil_log_message(logger, HIL_LOG_ERROR, "script",
                            "line %d: invalid fault duration", line);
            return HIL_ERR_SCRIPT;
        }
    }
    step.second_value = (double)duration;
    return hil_script_append_step(script, &step);
}

static hil_status_t parse_assert(hil_script_t *script, char **tokens,
                                 size_t count, int line,
                                 const hil_logger_t *logger)
{
    hil_step_t step;
    if (count < 4U || count > 6U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: ASSERT has invalid arguments", line);
        return HIL_ERR_SCRIPT;
    }
    (void)memset(&step, 0, sizeof(step));
    step.type = HIL_STEP_ASSERT;
    step.line = line;
    step.tolerance = 0.001;
    if (strlen(tokens[1]) >= sizeof(step.target) ||
        strlen(tokens[2]) >= sizeof(step.argument)) {
        return HIL_ERR_OVERFLOW;
    }
    (void)strncpy(step.target, tokens[1], sizeof(step.target) - 1U);
    (void)strncpy(step.argument, tokens[2], sizeof(step.argument) - 1U);
    if (!hil_assert_op_from_name(step.argument, &step.assert_op)) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: unknown assertion operator '%s'", line,
                        step.argument);
        return HIL_ERR_SCRIPT;
    }
    if (!hil_parse_double(tokens[3], &step.value)) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: invalid ASSERT expected value", line);
        return HIL_ERR_SCRIPT;
    }
    if (step.assert_op == HIL_ASSERT_BETWEEN) {
        if (count < 5U ||
            !hil_parse_double(tokens[4], &step.second_value)) {
            hil_log_message(logger, HIL_LOG_ERROR, "script",
                            "line %d: BETWEEN requires upper bound", line);
            return HIL_ERR_SCRIPT;
        }
        if (count == 6U && !hil_parse_double(tokens[5], &step.tolerance)) {
            hil_log_message(logger, HIL_LOG_ERROR, "script",
                            "line %d: invalid tolerance", line);
            return HIL_ERR_SCRIPT;
        }
    } else {
        if (count >= 5U && !hil_parse_double(tokens[4], &step.tolerance)) {
            hil_log_message(logger, HIL_LOG_ERROR, "script",
                            "line %d: invalid tolerance", line);
            return HIL_ERR_SCRIPT;
        }
    }
    return hil_script_append_step(script, &step);
}

static hil_status_t parse_reset(hil_script_t *script, char **tokens,
                                size_t count, int line,
                                const hil_logger_t *logger)
{
    hil_step_t step;
    (void)tokens;
    if (count != 1U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: RESET takes no arguments", line);
        return HIL_ERR_SCRIPT;
    }
    (void)memset(&step, 0, sizeof(step));
    step.type = HIL_STEP_RESET;
    step.line = line;
    return hil_script_append_step(script, &step);
}

static hil_status_t parse_end(hil_script_t *script, char **tokens,
                              size_t count, int line,
                              const hil_logger_t *logger)
{
    hil_step_t step;
    (void)tokens;
    if (count != 1U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: END takes no arguments", line);
        return HIL_ERR_SCRIPT;
    }
    (void)memset(&step, 0, sizeof(step));
    step.type = HIL_STEP_END;
    step.line = line;
    return hil_script_append_step(script, &step);
}

static hil_status_t parse_line(hil_script_t *script, char *line, int line_no,
                               hil_test_case_t **current,
                               const hil_logger_t *logger)
{
    char *tokens[16];
    size_t count = tokenize(line, tokens, HIL_ARRAY_LEN(tokens));
    if (count == 0U) {
        return HIL_OK;
    }
    if (token_equal(tokens[0], "TEST")) {
        if (*current != NULL) {
            hil_log_message(logger, HIL_LOG_ERROR, "script",
                            "line %d: TEST appears before previous END",
                            line_no);
            return HIL_ERR_SCRIPT;
        }
        if (parse_test(script, tokens, count, line_no, logger) != HIL_OK) {
            return HIL_ERR_SCRIPT;
        }
        *current = script->cases_tail;
        return HIL_OK;
    }
    if (token_equal(tokens[0], "END")) {
        if (*current == NULL) {
            hil_log_message(logger, HIL_LOG_ERROR, "script",
                            "line %d: END without TEST", line_no);
            return HIL_ERR_SCRIPT;
        }
        if (parse_end(script, tokens, count, line_no, logger) != HIL_OK) {
            return HIL_ERR_SCRIPT;
        }
        *current = NULL;
        return HIL_OK;
    }
    if (*current == NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "line %d: step appears outside TEST/END", line_no);
        return HIL_ERR_SCRIPT;
    }
    if (token_equal(tokens[0], "SET")) {
        return parse_set(script, tokens, count, line_no, logger);
    }
    if (token_equal(tokens[0], "WAIT")) {
        return parse_wait(script, tokens, count, line_no, logger);
    }
    if (token_equal(tokens[0], "FAULT")) {
        return parse_fault(script, tokens, count, line_no, logger);
    }
    if (token_equal(tokens[0], "ASSERT")) {
        return parse_assert(script, tokens, count, line_no, logger);
    }
    if (token_equal(tokens[0], "RESET")) {
        return parse_reset(script, tokens, count, line_no, logger);
    }
    hil_log_message(logger, HIL_LOG_ERROR, "script",
                    "line %d: unknown command '%s'", line_no, tokens[0]);
    return HIL_ERR_SCRIPT;
}

hil_status_t hil_script_load_text(hil_script_t *script, const char *text,
                                  const hil_logger_t *logger)
{
    const char *cursor;
    int line_no = 0;
    hil_test_case_t *current = NULL;
    if (script == NULL || text == NULL) {
        return HIL_ERR_NULL;
    }
    cursor = text;
    while (*cursor != '\0') {
        const char *line_start = cursor;
        char buffer[HIL_LINE_MAX];
        size_t length = 0U;
        while (*cursor != '\0' && *cursor != '\n' && *cursor != '\r') {
            if (length + 1U >= sizeof(buffer)) {
                hil_log_message(logger, HIL_LOG_ERROR, "script",
                                "line %d is too long", line_no + 1);
                return HIL_ERR_OVERFLOW;
            }
            buffer[length++] = *cursor++;
        }
        buffer[length] = '\0';
        line_no++;
        if (parse_line(script, buffer, line_no, &current, logger) != HIL_OK) {
            return HIL_ERR_SCRIPT;
        }
        if (*cursor == '\r') {
            cursor++;
        }
        if (*cursor == '\n') {
            cursor++;
        }
        (void)line_start;
    }
    if (current != NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "missing END for test '%s'", current->name);
        return HIL_ERR_SCRIPT;
    }
    if (script->case_count == 0U) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "script contains no test cases");
        return HIL_ERR_SCRIPT;
    }
    return HIL_OK;
}

hil_status_t hil_script_load_file(hil_script_t *script, const char *path,
                                  const hil_logger_t *logger)
{
    FILE *fp;
    long file_size;
    char *buffer;
    size_t read_count;
    hil_status_t status;
    if (script == NULL || path == NULL) {
        return HIL_ERR_NULL;
    }
    fp = fopen(path, "rb");
    if (fp == NULL) {
        hil_log_message(logger, HIL_LOG_ERROR, "script",
                        "cannot open script file: %s", path);
        return HIL_ERR_IO;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fclose(fp);
        return HIL_ERR_IO;
    }
    file_size = ftell(fp);
    if (file_size < 0L || fseek(fp, 0L, SEEK_SET) != 0) {
        (void)fclose(fp);
        return HIL_ERR_IO;
    }
    buffer = (char *)malloc((size_t)file_size + 1U);
    if (buffer == NULL) {
        (void)fclose(fp);
        return HIL_ERR_NOMEM;
    }
    read_count = fread(buffer, 1U, (size_t)file_size, fp);
    if (read_count != (size_t)file_size) {
        free(buffer);
        (void)fclose(fp);
        return HIL_ERR_IO;
    }
    buffer[read_count] = '\0';
    (void)fclose(fp);
    status = hil_script_load_text(script, buffer, logger);
    free(buffer);
    return status;
}
