# Virtual HIL Web Console 使用文档

## 1. 功能简介

本网页是“综合虚拟 HIL 测试台”C17 项目的本地 Web 控制台。它不替代原命令行程序，而是在浏览器中调用 `../build/hil.exe`，提供可视化输入、输出和报告查看。

网页后端使用 C17 实现，负责：

- 提供静态前端页面。
- 调用 `hil.exe run` 执行测试。
- 调用 `hil.exe query` 查询 CSV。
- 读取本地报告或 CSV 文件。

## 2. 启动方法

### 2.1 Windows

```powershell
cd F:\c\12\webapp
powershell -NoProfile -ExecutionPolicy Bypass -File .\run_web.ps1
```

如果 PowerShell 提示脚本执行策略，使用上面的 `-ExecutionPolicy Bypass` 即可，不会永久修改系统策略。

### 2.2 Linux / macOS

```bash
cd /path/to/webapp
./run_web.sh
```

启动后打开：

```text
http://127.0.0.1:8080
```

## 3. 页面模块

### 3.1 运行测试

输入项：

| 字段 | 默认值 | 说明 |
|---|---|---|
| 配置文件 | `../virtual_hil/examples/default_config.ini` | HIL 配置 |
| 信号文件 | `../virtual_hil/examples/signals.csv` | 信号定义 |
| 测试脚本 | `../virtual_hil/examples/demo.hil` | HIL 测试脚本 |
| 输出目录 | `data/out` | CSV 和报告输出目录 |
| 语言 | `中文` | `zh` 或 `en` |

点击“运行测试”后，后端执行：

```text
../virtual_hil/build/hil.exe run --config ... --signals ... --script ... --out ... --lang ...
```

输出：

- 运行日志和测试汇总显示在“运行输出”区域。
- 实际文件生成在 `webapp/data/out/run.csv` 和 `webapp/data/out/report.md`。

### 3.2 按时间 / 信号查询

输入项：

| 字段 | 默认值 | 说明 |
|---|---|---|
| CSV 文件 | `data/out/run.csv` | 查询数据文件 |
| 信号名 | `engine_speed` | 信号名称 |
| 开始时间 | `0` | 毫秒 |
| 结束时间 | `4294967295` | 毫秒 |

后端执行：

```text
../virtual_hil/build/hil.exe query --csv ... --signal ... --from ... --to ...
```

输出为两列 CSV 文本：`timestamp_ms,signal_name`。

### 3.3 查看报告 / 文件

输入一个本地相对路径，例如：

```text
data/out/report.md
```

点击“加载文件”后，页面显示文件内容。

## 4. HTTP API

后端只监听 `127.0.0.1:8080`，不对外网开放。

| 接口 | 方法 | 参数 | 功能 |
|---|---|---|---|
| `/api/status` | GET | 无 | 返回后端状态 |
| `/api/run` | GET | `config`、`signals`、`script`、`out`、`lang` | 运行 HIL 测试 |
| `/api/query` | GET | `csv`、`signal`、`from`、`to` | 查询信号 |
| `/api/file` | GET | `path` | 读取本地文件内容 |

示例：

```text
http://127.0.0.1:8080/api/status
http://127.0.0.1:8080/api/run?script=../virtual_hil/examples/demo.hil
http://127.0.0.1:8080/api/query?csv=data/out/run.csv&signal=engine_speed&from=200&to=400
http://127.0.0.1:8080/api/file?path=data/out/report.md
```

## 5. 目录结构

```text
webapp/
  backend/
    include/web_server.h
    src/main.c        服务器入口
    src/http.c        HTTP 解析与静态文件
    src/api.c         运行、查询、文件 API
    src/utils.c       URL 解码、参数校验、JSON 转义
  frontend/
    index.html
    css/style.css
    js/api.js
    js/app.js
  data/out/           运行输出
  docs/usage_web.md   本文档
```

## 6. 安全边界

- 服务只绑定本机回环地址 `127.0.0.1`。
- API 参数禁止 `"`、`&`、`|`、`;`、`<`、`>`、换行等字符。
- 文件读取只接受安全的相对路径。
- 后端不提供文件上传和任意目录浏览。

## 7. 常见问题

**网页能打开，但运行失败？**

请先确认主程序已经编译：

```powershell
cd F:\c\12\virtual_hil
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Target all
```

**端口 8080 被占用？**

可以修改 `backend/include/web_server.h` 中的 `WEB_DEFAULT_PORT`，然后重新运行 `run_web.ps1`。

**报告或 CSV 不存在？**

先在页面点击“运行测试”，生成 `data/out/run.csv` 和 `data/out/report.md`，再点击“加载文件”或执行查询。
