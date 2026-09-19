# 需求追踪矩阵

| 案例 12 必做功能 | 实现模块 | 自动化测试 |
|---|---|---|
| 虚拟 ECU 状态机 | `hil_ecu.c` | `executor_end_to_end`、ECU 状态转换经 e2e 用例覆盖 |
| 周期任务 | `hil_ecu.c` | `wait_and_periodic_task`、`long_run_stability` |
| 至少 8 路信号 | `hil_signal.c`、`examples/signals.csv` | `signal_load_csv` |
| 安全输出 | `hil_ecu.c` | `fault_stuck`、`fault_open_circuit` 等 |
| 总线帧队列 | `hil_bus.c` | `bus_publish_poll`、`bus_queue_full` |
| 帧丢失 | `hil_bus.c` | `bus_loss` |
| 帧延迟 | `hil_bus.c` | `bus_delay` |
| 帧篡改 | `hil_bus.c` | `bus_tamper` |
| 脚本 SET/WAIT/FAULT/ASSERT/RESET | `hil_script.c`、`hil_executor.c` | `script_valid`、`executor_end_to_end` |
| CSV 记录 | `hil_csv.c`、`hil_data_logger.c` | `csv_roundtrip`、`executor_end_to_end` |
| 按时间/信号查询 | `hil_data_logger.c` | `executor_end_to_end` 查询步骤 |
| 用例级/步骤级报告 | `hil_report.c` | `report_builder` |
| 统一配置 | `hil_config.c` | `config_load_valid`、`config_load_invalid` |
| 错误码 | `hil_common.c` | `status_names` |
| 日志 | `hil_logger.c` | 全部测试通过日志模块输出 |
| 一键构建/测试 | `Makefile`、`CMakeLists.txt`、`build.ps1` | `build.ps1 -Target test` |

## 六人协作追踪

- 1 项目经理/架构：common、logger、time、executor、main，文档 `01`。
- 2 ECU 开发：signal、ecu，文档 `02`。
- 3 总线开发：frame、bus，文档 `03`。
- 4 测试引擎：script、fault、assert、executor，文档 `04`。
- 5 数据质量：csv、data_logger、report、config，文档 `05`。
- 6 测试经理：tests、构建、Sanitizer、交付文档，文档 `06`。
