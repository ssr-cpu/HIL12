# 文件格式

## 1. 配置文件（INI 风格）

每行一个 `key=value`，空行及 `#`、`;` 开头视为注释。示例见 `examples/default_config.ini`。

| 键 | 说明 |
|---|---|
| `runtime.tick_ms` | 虚拟 tick 周期，必须大于 0 |
| `runtime.duration_ms` | 名义运行时长 |
| `runtime.random_seed` | 随机种子 |
| `runtime.stop_on_failure` | `true/false` |
| `ecu.power_on_delay_ms` | ECU INIT 阶段时长 |
| `ecu.comm_timeout_ms` | 通信超时 |
| `ecu.recovery_delay_ms` | 可恢复故障恢复等待 |
| `bus.queue_capacity` | 总线队列容量 |
| `bus.loss_probability_percent` | 0-100 |
| `bus.delay_ms` | 帧延迟 |
| `bus.tamper_enabled` | `true/false` |
| `data.log_file` | CSV 文件名 |
| `data.report_file` | 报告文件名 |

## 2. 信号定义 CSV

首行必须为：`name,unit,min,max,initial`。至少 8 个信号。示例见 `examples/signals.csv`。

## 3. 测试脚本

脚本由多个 `TEST name` 和 `END` 组成的用例构成。命令大小写不敏感，注释以 `#` 或 `;` 开头。

| 命令 | 语法 | 说明 |
|---|---|---|
| `TEST` | `TEST <name>` | 开始用例，名称不能重复 |
| `SET` | `SET <signal> <value>` | 设置信号 |
| `WAIT` | `WAIT <milliseconds>` | 推进虚拟时间 |
| `FAULT` | `FAULT <signal> <TYPE> <value> [duration_ms]` | 注入故障；`OPEN_CIRCUIT/DROPOUT` 可省略 value |
| `ASSERT` | `ASSERT <signal> <OP> <a> [b] [tol]` | 断言；`BETWEEN` 需要上下界 |
| `RESET` | `RESET` | 复位 ECU、信号和故障 |
| `END` | `END` | 结束用例 |

故障类型：`OFFSET`、`STUCK`、`NOISE`、`DROPOUT`、`OPEN_CIRCUIT`。

断言算子：`EQ`、`NE`、`LT`、`LE`、`GT`、`GE`、`BETWEEN`。

## 4. 数据 CSV

记录器输出首列 `timestamp`，随后为每个信号一列。无效值输出 `NA`。查询命令支持 `--from` 和 `--to` 毫秒过滤。

## 5. 报告

报告为 Markdown，包含汇总和步骤表：用例、行号、步骤、目标、期望、实际、状态、消息。
