#ifndef HIL_LOGGER_H
#define HIL_LOGGER_H

#include <stdarg.h>
#include <stdio.h>

#include "hil_common.h"

typedef enum hil_log_level {
    HIL_LOG_TRACE = 0,
    HIL_LOG_DEBUG = 1,
    HIL_LOG_INFO = 2,
    HIL_LOG_WARN = 3,
    HIL_LOG_ERROR = 4,
    HIL_LOG_OFF = 5
} hil_log_level_t;

typedef struct hil_logger {
    FILE *stream;
    hil_log_level_t min_level;
    bool enabled;
} hil_logger_t;

void hil_logger_init(hil_logger_t *logger, FILE *stream, hil_log_level_t min_level);
void hil_logger_set_level(hil_logger_t *logger, hil_log_level_t level);
const char *hil_log_level_name(hil_log_level_t level);

void hil_log_message(const hil_logger_t *logger, hil_log_level_t level,
                     const char *module, const char *format, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 4, 5)))
#endif
    ;

void hil_log_error_status(const hil_logger_t *logger, const char *module,
                          hil_status_t status, const char *context);

#endif
