#ifndef HIL_COMMON_H
#define HIL_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HIL_VERSION_MAJOR 1
#define HIL_VERSION_MINOR 0
#define HIL_VERSION_PATCH 0

#define HIL_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#define HIL_NAME_MAX 64U
#define HIL_PATH_MAX 512U
#define HIL_LINE_MAX 1024U
#define HIL_MESSAGE_MAX 256U

/*
 * All public APIs return hil_status_t. Zero means success, negative values are
 * stable error codes. Error context is emitted through hil_logger when an
 * explicit logger is available; otherwise callers can map codes with
 * hil_status_name().
 */
typedef enum hil_status {
    HIL_OK = 0,
    HIL_ERR_NULL = -1,
    HIL_ERR_NOMEM = -2,
    HIL_ERR_OVERFLOW = -3,
    HIL_ERR_RANGE = -4,
    HIL_ERR_FORMAT = -5,
    HIL_ERR_NOT_FOUND = -6,
    HIL_ERR_DUPLICATE = -7,
    HIL_ERR_BUSY = -8,
    HIL_ERR_TIMEOUT = -9,
    HIL_ERR_ASSERT = -10,
    HIL_ERR_IO = -11,
    HIL_ERR_STATE = -12,
    HIL_ERR_FAULT = -13,
    HIL_ERR_CONFIG = -14,
    HIL_ERR_UNSUPPORTED = -15,
    HIL_ERR_RESOURCE = -16,
    HIL_ERR_SCRIPT = -17,
    HIL_ERR_VALUE = -18,
    HIL_ERR_PARSE = -19,
    HIL_EOF = -20
} hil_status_t;

typedef struct hil_version {
    unsigned int major;
    unsigned int minor;
    unsigned int patch;
} hil_version_t;

const char *hil_status_name(hil_status_t status);
hil_version_t hil_version(void);
const char *hil_version_string(void);

/* Small checked helpers shared by modules. */
bool hil_double_is_finite(double value);
bool hil_parse_bool(const char *text, bool *value);
bool hil_parse_u64(const char *text, uint64_t *value);
bool hil_parse_double(const char *text, double *value);

#endif
