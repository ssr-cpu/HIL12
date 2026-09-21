# Virtual HIL Web Console

这是 `virtual_hil` C17 命令行项目的本地 Web 控制台。后端使用 C17 编写，调用 `../virtual_hil/build/hil.exe`；前端使用原生 HTML/CSS/JavaScript。

## 目录


```text
webapp/
  backend/        C17 HTTP 后端
  frontend/       HTML/CSS/JS 前端
  data/           运行输出目录
  docs/           使用文档
  run_web.ps1     Windows 启动脚本
  run_web.sh      Linux/macOS 启动脚本
```

## 快速启动

Windows：

```powershell
cd F:\c\12\webapp
powershell -NoProfile -ExecutionPolicy Bypass -File .\run_web.ps1
```

Linux/macOS：

```bash
cd /path/to/webapp
./run_web.sh
```

然后打开浏览器访问 `http://127.0.0.1:8080`。

详细说明见 [docs/usage_web.md](docs/usage_web.md)。
