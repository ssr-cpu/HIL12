# 角色 5：数据质量

## 职责

负责统一配置、CSV 读写、数据记录、按时间/信号查询、断言记录和测试报告。

## 实现文件

- `include/hil_config.h`、`src/hil_config.c`
- `include/hil_csv.h`、`src/hil_csv.c`
- `include/hil_data_logger.h`、`src/hil_data_logger.c`
- `include/hil_report.h`、`src/hil_report.c`

## 配置

`hil_config_load()` 解析 INI 风格配置；未知键、非法值、溢出均返回错误，并指出文件行号。

## CSV

- `hil_csv_writer_open/write_row/close` 输出带引号转义的 CSV。
- `hil_csv_reader_open/read_row/close` 安全读取 CSV，处理引号、逗号和超长字段。
- 读取错误时返回 `HIL_ERR_FORMAT` 或 `HIL_ERR_OVERFLOW`。

## 数据记录

`hil_data_logger_open()` 写表头；`hil_data_logger_log_sample()` 每个 tick 写入时间戳和所有信号；无效值写 `NA`。

`hil_data_logger_query()` 支持按信号名和 `[from_ms, to_ms]` 过滤 CSV 记录。

## 报告

`hil_report_add_step()` 记录用例、行号、步骤、目标、期望、实际、状态和消息；`hil_report_write_markdown()` 输出用例级和步骤级 Markdown 报告。

## 错误码与所有权

- CSV writer/reader 拥有各自 `FILE*`，`close` 后不再有效。
- 数据记录器借用信号注册表，不负责释放。
- 报告构建器拥有步骤链表，`hil_report_deinit()` 释放。

## 验证

- `config_load_valid` / `config_load_invalid`。
- `csv_roundtrip` / `csv_malformed`。
- `report_builder`。
- `executor_end_to_end` 验证 CSV 输出、查询和报告生成。
