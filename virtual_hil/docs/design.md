# 架构设计

## 1. 目标

实现一个无硬件 C17 综合虚拟 HIL 测试台。命令行加载配置文件、信号定义和测试脚本，驱动虚拟 ECU 与虚拟总线，执行测试步骤，记录 CSV，并生成 Markdown 报告。

## 2. 模块边界

| 模块 | 文件 | 职责 | 归属角色 |
|---|---|---|---|
| 公共基础 | `hil_common` | 状态码、版本、数值解析 | 1 |
| 日志 | `hil_logger` | 分级日志、错误上下文 | 1 |
| 虚拟时间 | `hil_time` | 虚拟毫秒时钟、溢出检查 | 1 |
| 配置 | `hil_config` | 加载 key=value 配置 | 1/5 |
| 信号 | `hil_signal` | 信号定义、注册表、CSV 加载 | 2 |
| ECU | `hil_ecu` | 状态机、周期任务、安全输出 | 2 |
| 帧 | `hil_frame` | 帧结构、CRC、编解码 | 3 |
| 总线 | `hil_bus` | 帧队列、丢失/延迟/篡改 | 3 |
| 故障注入 | `hil_fault` | 信号级故障叠加 | 4 |
| 脚本 | `hil_script` | 脚本词法解析、对象模型 | 4 |
| 断言 | `hil_assert` | 数值比较与范围判断 | 4 |
| CSV | `hil_csv` | CSV 读写、转义、错误检查 | 5 |
| 数据记录 | `hil_data_logger` | CSV 样本记录与查询 | 5 |
| 报告 | `hil_report` | 步骤结果、Markdown 报告 | 5 |
| 执行器 | `hil_executor` | 集成 ECU、总线、故障、脚本 | 1/4 |
| CLI/入口 | `hil_cli`、`main` | 命令解析、输出目录、退出码 | 1 |

## 3. 主执行流程

1. `main` 解析 `run` 或 `query` 命令。
2. `run` 加载配置和信号表，打开 CSV 记录器。
3. `hil_executor_init` 加载测试脚本，初始化虚拟时间、ECU、总线和故障管理器。
4. 每个 `TEST...END` 用例执行前复位 ECU、信号和故障。
5. `SET` 修改信号；`WAIT` 按 tick 推进时间并执行 ECU 周期任务、总线收发和故障注入；`FAULT` 注入信号故障；`ASSERT` 记录通过/失败；`RESET` 清除故障并复位 ECU。
6. 所有用例完成后关闭 CSV，写 Markdown 报告。
7. 有失败或错误时进程退出码非 0。

## 4. 关键数据结构

- `hil_signal_registry_t` 拥有动态信号数组，由调用方负责 `deinit`。
- `hil_bus_t` 拥有环形帧队列，`hil_bus_deinit` 释放队列。
- `hil_fault_manager_t` 拥有动态故障规格数组。
- `hil_script_t` 拥有测试用例和步骤链表，`hil_script_deinit` 递归释放。
- `hil_report_builder_t` 拥有步骤结果链表，`hil_report_deinit` 释放。

## 5. 指针所有权规则

- 创建方释放原则：`registry`、`bus`、`fault_manager`、`script`、`report` 的拥有者负责调用对应 `deinit`。
- 跨模块传入的 `const` 对象只读，不转移所有权。
- `hil_executor` 借用 `signals`、`data_logger`、`report`，不负责释放它们。
- `hil_data_logger` 借用 `signals`，不负责释放信号注册表。

## 6. 错误码

所有公开 API 返回 `hil_status_t`：`HIL_OK=0`，负值表示错误。日志中会包含模块、错误码和上下文。主要错误码见 `include/hil_common.h`。

## 7. 构建与线程假设

本项目为单线程命令行程程，未使用线程；虚拟时间由 `hil_virtual_time_t` 显式推进。模块不依赖大面积全局可变状态。
