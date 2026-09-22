#ifndef HIL_WEB_H
#define HIL_WEB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hil_assert.h"
#include "hil_bus.h"
#include "hil_common.h"
#include "hil_config.h"
#include "hil_data_logger.h"
#include "hil_ecu.h"
#include "hil_executor.h"
#include "hil_fault.h"
#include "hil_frame.h"
#include "hil_logger.h"
#include "hil_report.h"
#include "hil_script.h"
#include "hil_signal.h"
#include "hil_time.h"

#define HIL_WEB_MAX_SAMPLES 4000U
#define HIL_WEB_MAX_SIGNALS 32U
#define HIL_WEB_JSON_MAX 32768U

/* 交互式会话：直接持有 C 核心模块的对象，网页上的每一次操作都作用在它上面。
   所有权：session 拥有 signals / bus / faults / data_logger / report /
   sample_time / sample_values，由 hil_web_session_deinit() 统一释放。 */
typedef struct hil_web_session {
    hil_config_t config;
    hil_virtual_time_t time;
    hil_signal_registry_t signals;
    hil_ecu_t ecu;
    hil_bus_t bus;
    hil_fault_manager_t faults;
    hil_data_logger_t data_logger;
    hil_report_builder_t report;
    hil_logger_t app_logger;
    bool ready;
    uint64_t *sample_time;
    double *sample_values;
    size_t sample_count;
    size_t sample_signal_count;
    char last_report_json[HIL_WEB_JSON_MAX];
    char last_error[HIL_MESSAGE_MAX];
} hil_web_session_t;

void hil_web_default_signals(hil_signal_registry_t *registry,
                             const hil_logger_t *logger);
void hil_web_api_set_session(hil_web_session_t *session);
void hil_web_session_init(hil_web_session_t *session);
void hil_web_session_deinit(hil_web_session_t *session);
hil_status_t hil_web_session_start(hil_web_session_t *session,
                                   const char *output_dir);
hil_status_t hil_web_session_advance(hil_web_session_t *session,
                                     uint64_t duration_ms);
void hil_web_session_record(hil_web_session_t *session);
hil_status_t hil_web_session_reset(hil_web_session_t *session);
hil_status_t hil_web_session_reconfigure_bus(hil_web_session_t *session,
                                             unsigned int loss_percent,
                                             uint64_t delay_ms, bool tamper,
                                             size_t capacity);

#endif
