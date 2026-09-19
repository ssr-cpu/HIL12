#ifndef HIL_EXECUTOR_H
#define HIL_EXECUTOR_H

#include "hil_common.h"
#include "hil_config.h"
#include "hil_data_logger.h"
#include "hil_ecu.h"
#include "hil_fault.h"
#include "hil_bus.h"
#include "hil_logger.h"
#include "hil_report.h"
#include "hil_script.h"
#include "hil_signal.h"
#include "hil_time.h"

typedef struct hil_executor {
    hil_config_t config;
    hil_virtual_time_t time;
    hil_signal_registry_t *signals;
    hil_ecu_t ecu;
    hil_bus_t bus;
    hil_fault_manager_t faults;
    hil_script_t script;
    hil_data_logger_t *data_logger;
    hil_report_builder_t *report;
    hil_test_case_t *current_case;
    hil_step_t *current_step;
    bool aborted;
    uint64_t run_start_ms;
} hil_executor_t;

hil_status_t hil_executor_init(hil_executor_t *executor,
                               const hil_config_t *config,
                               hil_signal_registry_t *signals,
                               const char *script_path,
                               hil_data_logger_t *data_logger,
                               hil_report_builder_t *report,
                               const hil_logger_t *logger);
void hil_executor_deinit(hil_executor_t *executor);
hil_status_t hil_executor_run(hil_executor_t *executor,
                              const hil_logger_t *logger);
bool hil_executor_aborted(const hil_executor_t *executor);

#endif
