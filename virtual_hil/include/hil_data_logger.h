#ifndef HIL_DATA_LOGGER_H
#define HIL_DATA_LOGGER_H

#include <stdbool.h>
#include <stdint.h>

#include "hil_common.h"
#include "hil_csv.h"
#include "hil_logger.h"
#include "hil_signal.h"

typedef struct hil_data_logger {
    hil_csv_writer_t writer;
    hil_signal_registry_t *signals;
    char path[HIL_PATH_MAX];
    bool open;
} hil_data_logger_t;

hil_status_t hil_data_logger_open(hil_data_logger_t *logger,
                                  const char *path,
                                  hil_signal_registry_t *signals,
                                  const hil_logger_t *app_logger);
hil_status_t hil_data_logger_log_sample(hil_data_logger_t *logger,
                                        uint64_t timestamp_ms,
                                        const hil_logger_t *app_logger);
hil_status_t hil_data_logger_close(hil_data_logger_t *logger,
                                   const hil_logger_t *app_logger);

hil_status_t hil_data_logger_query(const char *path, const char *signal_name,
                                   uint64_t from_ms, uint64_t to_ms,
                                   const hil_logger_t *logger);

#endif
