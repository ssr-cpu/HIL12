#ifndef HIL_CONFIG_H
#define HIL_CONFIG_H

#include <stdint.h>

#include "hil_common.h"
#include "hil_logger.h"

typedef struct hil_config {
    uint64_t tick_ms;
    uint64_t duration_ms;
    uint64_t ecu_power_on_delay_ms;
    uint64_t ecu_comm_timeout_ms;
    uint64_t ecu_recovery_delay_ms;
    size_t bus_queue_capacity;
    unsigned int bus_loss_probability_percent;
    uint64_t bus_delay_ms;
    bool bus_tamper_enabled;
    char output_dir[HIL_PATH_MAX];
    char log_file[HIL_PATH_MAX];
    char report_file[HIL_PATH_MAX];
    unsigned int random_seed;
    bool stop_on_failure;
    bool verbose;
} hil_config_t;

void hil_config_init(hil_config_t *config);
hil_status_t hil_config_load(hil_config_t *config, const char *path,
                             const hil_logger_t *logger);
hil_status_t hil_config_set_default_paths(hil_config_t *config,
                                         const char *output_dir);

#endif
