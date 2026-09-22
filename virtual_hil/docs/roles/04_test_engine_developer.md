# 角色 4：测试引擎

## 职责

负责测试脚本解析、步骤模型、执行流程、信号级故障注入和断言。

## 实现文件

- `include/hil_script.h`、`src/hil_script.c`
- `include/hil_fault.h`、`src/hil_fault.c`
- `include/hil_assert.h`、`src/hil_assert.c`
- `include/hil_executor.h`、`src/hil_executor.c`

## 脚本语法

支持 `TEST`、`SET`、`WAIT`、`FAULT`、`ASSERT`、`RESET`、`END`。解析错误包含行号和原因，未知命令、缺少 `END`、重复用例名均会被拒绝。

## 故障注入

故障类型：`OFFSET`、`STUCK`、`NOISE`、`DROPOUT`、`OPEN_CIRCUIT`。

`hil_fault_manager_apply()` 每个 tick 先恢复原始值，再叠加所有生效故障；`OPEN_CIRCUIT` 和 `DROPOUT` 同时设置信号无效。

## 断言

算子：`EQ`、`NE`、`LT`、`LE`、`GT`、`GE`、`BETWEEN`。

`hil_assert_check_double()` 和 `hil_assert_check_double_range()` 返回 `HIL_OK` 或 `HIL_ERR_ASSERT`，并填充期望值、实际值和消息。

## 执行流程

`hil_executor_run()` 遍历用例；每个用例开始前复位 ECU、信号和故障。`WAIT` 按 tick 推进时间并执行 ECU、故障注入、总线收发和数据记录；`RESET` 清除故障并复位 ECU。

## 错误码与所有权

- `hil_script_t` 拥有用例和步骤链表，`hil_script_deinit()` 递归释放。
- `hil_fault_manager_t` 拥有故障规格数组，`hil_fault_manager_deinit()` 释放。
- 解析失败时，脚本对象保持可释放状态。

## 验证

- `script_valid`、`script_unknown_command`、`script_missing_end`、`script_duplicate_case`。
- `fault_stuck`、`fault_open_circuit`、`fault_remove`。
- `assert_pass_and_fail`。
- `executor_end_to_end` 覆盖 15 个端到端用例；`script_ecu_commands`、`script_ecu_command_invalid`、`executor_ecu_fault_scenarios` 覆盖 `POWER` 与 `FAULT ECU LATCH|COMM|CLEAR`。
