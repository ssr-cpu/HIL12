# 测试报告

## 执行结果

测试入口：`tests/test_main.c`。

当前结果：41 项测试全部通过，0 失败。

测试分类：

- 正常：配置加载、信号注册、帧编解码、CSV 往返、报告生成。
- 边界：信号边界、队列满、帧偏移溢出、时间溢出。
- 异常：重复信号、未知脚本命令、缺少 END、损坏 CSV、日志文件不可写。
- 故障注入：STUCK、OPEN_CIRCUIT、DROPOUT、总线丢失/延迟/篡改、ECU 锁存与通信中断（脚本层）。
- 状态与超时：通信超时、断电、锁存故障、长时间运行。
- 脚本健壮性：`POWER`、`FAULT ECU` 的合法与非法写法。
- 端到端：`examples/e2e_suite.hil` 中 15 个用例（107 个步骤）通过。

回归记录：为修复"锁存故障与断电只能由单元测试触发、脚本不可达"这一缺陷，新增 `script_ecu_commands`、`script_ecu_command_invalid`、`executor_ecu_fault_scenarios` 三项测试，以及 `power_off_and_restart`、`comm_timeout_recoverable`、`ecu_latched_fault` 三个端到端用例。

## 复现命令

```powershell
cd F:\c\12\virtual_hil
.\build.ps1 -Target test
```

Sanitizer：

```powershell
.\build.ps1 -Target sanitize
```

或：

```bash
make sanitize
```

> 本机当前为 Windows + MinGW UCRT，环境中未安装 `libasan`/`libubsan`，因此 Sanitizer 目标已提供但无法在本地链接。请在 WSL 或 Linux 环境执行 `make sanitize`，或安装带 Sanitizer 运行时的 MinGW/Clang 工具链。

## 结果判定

测试程序返回 0 表示全部通过，返回 1 表示至少一项失败。每个失败会输出文件名、行号和断言。
