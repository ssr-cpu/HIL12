# 角色 1：项目经理 / 架构

## 职责

负责项目计划、模块边界、公开接口、集成和风险。本实现对应 `hil_common`、`hil_logger`、`hil_time`、`hil_executor`、`hil_cli` 和 `main`。

## 实现文件

- `include/hil_common.h`、`src/hil_common.c`
- `include/hil_logger.h`、`src/hil_logger.c`
- `include/hil_time.h`、`src/hil_time.c`
- `include/hil_executor.h`、`src/hil_executor.c`
- `include/hil_cli.h`、`src/hil_cli.c`
- `src/main.c`

## 关键接口

- `hil_status_t`：统一返回码，0 表示成功，负值表示错误。
- `hil_status_name()`：错误码转稳定名称。
- `hil_parse_bool()` / `hil_parse_u64()` / `hil_parse_double()`：所有不可信输入的数值解析入口。
- `hil_logger_init()` / `hil_log_message()` / `hil_log_error_status()`：分级日志和错误上下文。
- `hil_time_advance()`：虚拟时间推进，带溢出检查。
- `hil_executor_init()` / `hil_executor_run()` / `hil_executor_deinit()`：集成全部角色模块。
- `hil_cli_parse()` / `hil_cli_run()` / `hil_cli_query()`：命令行入口。

## 错误码与所有权

- 架构统一约定：创建方负责释放，借用方不释放。
- `hil_executor` 借用信号注册表、数据记录器和报告，不拥有它们。
- 所有公开函数不依赖全局可变状态。

## 验证

- `status_names`：错误码名称正确。
- `parse_bool` / `parse_u64` / `parse_double`：合法与非法输入均被正确识别。
- `time_advance` / `time_overflow`：虚拟时间和溢出路径。
- `executor_end_to_end`：六模块集成的 15 个端到端用例全部通过。
