# 综合虚拟 HIL 测试台（C17）

本项目严格按照《HIL 工程师 C 语言项目需求文档索引》和《案例 12：综合虚拟HIL测试台》实现，是一个无真实硬件的 C17 命令行应用。它把虚拟 ECU、虚拟总线、故障注入、脚本执行、数据记录和测试报告串成一个可复现的测试台。

## 已实现功能

- 虚拟 ECU：`POWER_OFF / INIT / SELF_TEST / STANDBY / RUN / FAULT / RECOVERY` 状态机，周期任务，故障安全输出。
- 10 路信号：电池电压、转速、车速、水温、油门、制动压力、燃油、机油压力、挡位、环境温度。
- 虚拟总线：帧队列、帧编码/解码、CRC、丢失、延迟、篡改。
- 测试脚本：`TEST / SET / WAIT / FAULT / ASSERT / RESET / POWER / END`。
- 故障注入两层：信号级 `OFFSET/STUCK/NOISE/DROPOUT/OPEN_CIRCUIT`，ECU 级 `FAULT ECU LATCH|COMM|CLEAR` 与 `POWER ON|OFF`。
- 数据记录：CSV 写入、按时间和信号查询。
- 报告：用例级和步骤级 Markdown 报告。
- 统一配置、错误码、日志，以及 Makefile、CMake、PowerShell 构建脚本。

## 目录结构

```text
virtual_hil/
  include/         公开接口
  src/             C17 实现（main.c 除外均为可测试模块）
  tests/           自动化测试入口
  examples/        示例配置、信号表、测试脚本
  docs/            设计、文件格式、追踪、角色文档
  build.ps1        Windows PowerShell 一键构建/测试
  Makefile         一键构建/测试
  CMakeLists.txt   CMake 构建
```

## 构建与测试

Windows PowerShell：

```powershell
cd F:\c\12\virtual_hil
.\build.ps1 -Target all
.\build.ps1 -Target test
```

使用 GNU Make：

```bash
make
make test
```

使用 CMake：

```bash
cmake -S . -B build/cmake
cmake --build build/cmake
ctest --test-dir build/cmake
```

## 运行示例

```powershell
.\build\hil.exe run `
  --config examples\default_config.ini `
  --signals examples\signals.csv `
  --script examples\demo.hil `
  --out build\out `
  --lang zh

.\build\hil.exe query `
  --csv build\out\run.csv `
  --signal engine_speed `
  --from 200 `
  --to 400
```

运行后会在 `build\out` 生成 `run.csv` 和 `report.md`。

## 六人分工

| 角色 | 主要模块 | 文档 |
|---|---|---|
| 1 项目经理/架构 | common、config、logger、time、executor、main | [01](docs/roles/01_project_manager_architecture.md) |
| 2 ECU 开发 | signal、ecu | [02](docs/roles/02_ecu_developer.md) |
| 3 总线开发 | frame、bus | [03](docs/roles/03_bus_developer.md) |
| 4 测试引擎 | script、fault、assert、executor | [04](docs/roles/04_test_engine_developer.md) |
| 5 数据质量 | csv、data_logger、report、config | [05](docs/roles/05_data_quality_developer.md) |
| 6 测试经理 | tests、构建、Sanitizer、交付文档 | [06](docs/roles/06_test_manager.md) |

按人查看文件：见 [团队分工文件](团队分工文件/README.md)。

## 测试概况

当前自动化测试共 41 项，全部通过，覆盖正常、边界、异常、故障注入、通信超时、断电、锁存故障、日志失败、长时间运行和端到端流程。测试入口是 `tests/test_main.c`，`examples/e2e_suite.hil` 提供 15 个端到端用例（107 个步骤）。

脚本层注入 ECU 故障与断电的写法：

```
TEST ecu_latched_fault
WAIT 300
FAULT ECU LATCH          # 锁存故障，停在 FAULT 不再自动恢复
ASSERT ecu_fault_latched EQ 1 0.01
WAIT 1000
ASSERT ecu_state EQ FAULT
FAULT ECU CLEAR          # 手动清除后进入 RECOVERY
ASSERT ecu_state EQ RECOVERY
END

TEST power_off_and_restart
WAIT 300
POWER OFF
ASSERT ecu_state EQ POWER_OFF
POWER ON
WAIT 300
ASSERT ecu_state EQ RUN
END
```

`ecu_state` 与 `ecu_fault_latched` 是 ECU 每 tick 镜像出来的只读信号，取值见 `docs/file_format.md`。

详细测试结果见 [docs/test_report.md](docs/test_report.md)。

答辩前快速上手请看 [docs/答辩使用文档.md](答辩使用文档.md)。
