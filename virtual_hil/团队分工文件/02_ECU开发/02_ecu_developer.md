# 角色 2：ECU 开发

## 职责

负责虚拟 ECU 的状态机、周期任务、安全输出，以及信号模型和信号注册表。

## 实现文件

- `include/hil_signal.h`、`src/hil_signal.c`
- `include/hil_ecu.h`、`src/hil_ecu.c`

## 信号模型

信号注册表支持动态添加、查找、设置、复位和 CSV 加载。本项目内置 10 路信号：

`battery_voltage`、`engine_speed`、`vehicle_speed`、`coolant_temp`、`throttle_position`、`brake_pressure`、`fuel_level`、`oil_pressure`、`gear_position`、`ambient_temp`。

## ECU 状态机

状态：`POWER_OFF`、`INIT`、`SELF_TEST`、`STANDBY`、`RUN`、`FAULT`、`RECOVERY`。

- `hil_ecu_power_on()` 从 `POWER_OFF` 进入 `INIT`。
- `hil_ecu_tick()` 根据虚拟时间和超时推进状态。
- `hil_ecu_set_comm_fault()` 注入通信故障。
- `hil_ecu_latch_fault()` 锁存故障。
- `hil_ecu_clear_fault()` / `hil_ecu_reset()` 清除故障或复位。
- `hil_ecu_apply_safe_outputs()` 在断电/故障时输出安全值。

## 周期任务

`hil_ecu_run_periodic_tasks()` 在 `RUN` 状态下按 `periodic_period_ms` 更新转速、车速、机油压力等信号，使系统具备连续仿真行为。

## 错误码与所有权

- 信号注册表拥有 `items` 数组，调用方必须调用 `hil_signal_registry_deinit()`。
- ECU 只借用信号注册表，不负责释放。
- 重复信号返回 `HIL_ERR_DUPLICATE`，未知信号返回 `HIL_ERR_NOT_FOUND`。

## 验证

- `signal_add_find` / `signal_duplicate` / `signal_set_reset` / `signal_load_csv`。
- `fault_stuck` / `fault_open_circuit` / `executor_end_to_end` 覆盖状态机、周期任务和安全输出。
