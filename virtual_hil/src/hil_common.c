#include "hil_common.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const status_names[] = {
    "OK",
    "NULL_POINTER",
    "OUT_OF_MEMORY",
    "OVERFLOW",
    "RANGE",
    "FORMAT",
    "NOT_FOUND",
    "DUPLICATE",
    "BUSY",
    "TIMEOUT",
    "ASSERT",
    "IO",
    "STATE",
    "FAULT",
    "CONFIG",
    "UNSUPPORTED",
    "RESOURCE",
    "SCRIPT",
    "VALUE",
    "PARSE",
    "EOF"
};

const char *hil_status_name(hil_status_t status)
{
    if (status > 0 || status < HIL_EOF) {
        return "UNKNOWN";
    }
    return status_names[-status];
}

hil_version_t hil_version(void)
{
    hil_version_t version = {HIL_VERSION_MAJOR, HIL_VERSION_MINOR,
                             HIL_VERSION_PATCH};
    return version;
}

const char *hil_version_string(void)
{
    static char buffer[32];
    (void)snprintf(buffer, sizeof(buffer), "%u.%u.%u", HIL_VERSION_MAJOR,
                   HIL_VERSION_MINOR, HIL_VERSION_PATCH);
    return buffer;
}

bool hil_double_is_finite(double value)
{
    return isfinite(value);
}

static char *trim(char *text)
{
    char *end;
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        text++;
    }
    if (*text == '\0') {
        return text;
    }
    end = text + strlen(text) - 1U;
    while (end > text &&
           (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return text;
}

bool hil_parse_bool(const char *text, bool *value)
{
    char buffer[16];
    char *p;
    if (text == NULL || value == NULL) {
        return false;
    }
    if (strlen(text) >= sizeof(buffer)) {
        return false;
    }
    (void)strncpy(buffer, text, sizeof(buffer) - 1U);
    buffer[sizeof(buffer) - 1U] = '\0';
    p = trim(buffer);
    if (strcmp(p, "1") == 0 || strcmp(p, "true") == 0 ||
        strcmp(p, "yes") == 0 || strcmp(p, "on") == 0) {
        *value = true;
        return true;
    }
    if (strcmp(p, "0") == 0 || strcmp(p, "false") == 0 ||
        strcmp(p, "no") == 0 || strcmp(p, "off") == 0) {
        *value = false;
        return true;
    }
    return false;
}

bool hil_parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;
    if (text == NULL || value == NULL || *text == '\0') {
        return false;
    }
    if (strchr(text, '-') != NULL) {
        return false;
    }
    errno = 0;
    parsed = strtoull(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0') {
        return false;
    }
    *value = (uint64_t)parsed;
    return true;
}

bool hil_parse_double(const char *text, double *value)
{
    char *end = NULL;
    double parsed;
    if (text == NULL || value == NULL || *text == '\0') {
        return false;
    }
    errno = 0;
    parsed = strtod(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' ||
        !hil_double_is_finite(parsed)) {
        return false;
    }
    *value = parsed;
    return true;
}
