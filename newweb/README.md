# 综合虚拟 HIL 测试台 · Web 控制台

案例 12《综合虚拟HIL测试台》的 Web 形态实现。**核心模块仍然是 C17**，与 `virtual_hil` 共用同一套 `.c` 源码；本目录下的 C17 后端直接链接核心模块，前端只是操作界面。因此不存在"用 Python / C++ 替代核心 C 模块"的问题。

## 目录

```text
newweb/
  backend/
    include/    hil_web.h（会话）、web_http.h（HTTP 与工具）
    src/        main.c 服务器主循环、http.c HTTP 层、api.c REST 分发、
                session.c HIL 会话、utils.c 编解码与文件
    Makefile    一键构建
  frontend/
    index.html  单页控制台
    css/style.css
    js/api.js   与后端 REST 接口通信
    js/app.js   页面逻辑与 Canvas 绘图
  data/out/     run.csv、report.md、last_script.hil
  start.ps1     Windows 一键构建并启动
```

## 构建与启动

**最简单：双击 `start.bat`**（会自动构建并打开浏览器 http://127.0.0.1:8090）。

或用 PowerShell：

```powershell
cd F:\c\12\newweb
powershell -NoProfile -ExecutionPolicy Bypass -File .\start.ps1
```

或手动：

```bash
cd newweb/backend
make            # 需要 gcc + make
make run
```

启动后打开 <http://127.0.0.1:8090>（默认 8090，避免与既有 `webapp` 的 8080 冲突；可用 `--port` 改）。

后端要求工作目录为 `newweb/`，静态资源按 `frontend/...` 相对路径读取。

## 需求覆盖

| 需求 | 实现位置 | 页面入口 |
|---|---|---|
| 虚拟 ECU 状态机 | `hil_ecu.c`，7 态 | 试验台 · ECU 状态链 |
| 周期任务、安全输出 | `hil_ecu.c` run_periodic_tasks / apply_safe_outputs | WAIT 后信号自动刷新 |
| 12 路信号（含 ecu_state、ecu_fault_latched 镜像） | `session.c` hil_web_default_signals | 试验台 · 信号表 |
| 虚拟总线：帧队列/丢失/延迟/篡改 | `hil_bus.c`、`hil_frame.c` | 试验台 · 虚拟总线 |
| 脚本 SET/WAIT/FAULT/ASSERT/RESET/POWER/END | `hil_script.c`、`hil_executor.c` | 脚本执行 |
| CSV 记录与按时间/信号查询 | `hil_data_logger.c`、`hil_csv.c` | 数据记录 + `data/out/run.csv` |
| 用例级 / 步骤级报告 | `hil_report.c` | 报告页 + `data/out/report.md` |
| 统一配置、错误码、日志 | `hil_config.c`、`hil_common.c`、`hil_logger.c` | 后端统一处理 |
| 信号级故障 5 种 | `hil_fault.c` | 试验台 · 信号级故障注入 |
| ECU 级锁存 / 通信故障 / 断电 | `hil_ecu.c` latch / comm / power | 试验台 · ECU 级故障与电源 |

## REST 接口

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/api/state` | 时间、ECU 状态、信号、故障、总线统计 |
| GET | `/api/set?signal=&value=` | 设置信号 |
| GET | `/api/wait?ms=` | 推进虚拟时间（同时跑周期任务、故障、总线、采样） |
| GET | `/api/fault?signal=&type=&value=&duration=` | 注入信号级故障 |
| GET | `/api/fault/clear` | 清除信号级故障 |
| GET | `/api/ecufault?action=LATCH｜COMM｜CLEAR` | ECU 级故障 |
| GET | `/api/power?state=ON｜OFF` | 上电 / 断电 |
| GET | `/api/reset` | 复位 ECU、信号、故障、总线 |
| GET | `/api/assert?signal=&op=&value=&tol=` | 立即执行断言，写进会话报告 |
| GET | `/api/history?signal=&from=&to=` | 按时间窗取采样点 |
| POST | `/api/script`（body `script=`） | 运行整份脚本，返回步骤级结果 |
| GET | `/api/report` | 最近一次脚本运行的报告 |
| GET | `/api/config?loss=&delay=&tamper=&capacity=` | 调整总线故障参数 |

## 演示建议（10～15 分钟）

1. 试验台页：`SET throttle_position 20` → `WAIT 300`，看状态链走到 RUN、转速升到 2040。
2. 注入 `engine_speed STUCK 1500 100` → `WAIT 20`，转速被卡死；`WAIT 200` 后自动恢复。
3. 点 `FAULT ECU LATCH` → 状态变 FAULT；连点 `WAIT 800`，仍然停在 FAULT（锁存不自愈）；再点 `FAULT ECU CLEAR` → RECOVERY。
4. `POWER OFF` → POWER_OFF 且转速归零（安全输出）；`POWER ON` → 重新走 INIT→SELF_TEST→STANDBY→RUN。
5. 虚拟总线设 `loss=25 tamper=开` → `WAIT 1000`，看已发布/已丢失/已篡改计数。
6. 脚本执行页载入"完整套件"并运行，报告页看 18 个步骤全 PASS。
