#include "hil_logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *const level_names[] = {
    [HIL_LOG_TRACE] = "TRACE",
    [HIL_LOG_DEBUG] = "DEBUG",
    [HIL_LOG_INFO] = "INFO",
    [HIL_LOG_WARN] = "WARN",
    [HIL_LOG_ERROR] = "ERROR",
    [HIL_LOG_OFF] = "OFF"
};

void hil_logger_init(hil_logger_t *logger, FILE *stream,
                     hil_log_level_t min_level)
{
    if (logger == NULL) {
        return;
    }
    logger->stream = stream != NULL ? stream : stderr;
    logger->min_level = min_level;
    logger->enabled = true;
}

void hil_logger_set_level(hil_logger_t *logger, hil_log_level_t level)
{
    if (logger != NULL) {
        logger->min_level = level;
    }
}

const char *hil_log_level_name(hil_log_level_t level)
{
    if (level < HIL_LOG_TRACE || level > HIL_LOG_OFF) {
        return "UNKNOWN";
    }
    return level_names[level];
}

void hil_log_message(const hil_logger_t *logger, hil_log_level_t level,
                     const char *module, const char *format, ...)
{
    char buffer[HIL_MESSAGE_MAX];
    va_list args;
    int written;
    if (logger == NULL || !logger->enabled || logger->min_level > level ||
        format == NULL) {
        return;
    }
    va_start(args, format);
    written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (written < 0) {
        return;
    }
    (void)fprintf(logger->stream, "[%s] [%s] %s\n", level_names[level],
                  module != NULL ? module : "-", buffer);
    (void)fflush(logger->stream);
}

void hil_log_error_status(const hil_logger_t *logger, const char *module,
                          hil_status_t status, const char *context)
{
    hil_log_message(logger, HIL_LOG_ERROR, module, "%s: %s",
                    context != NULL ? context : "operation failed",
                    hil_status_name(status));
}
