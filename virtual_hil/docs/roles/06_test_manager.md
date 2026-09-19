# 角色 6：测试经理

## 职责

负责 CI/构建、系统测试、Sanitizer 和交付文档。本实现提供 `tests/test_main.c`、`Makefile`、`CMakeLists.txt`、`build.ps1` 和全部交付文档。

## 构建

- PowerShell：`.\build.ps1 -Target all`
- GNU Make：`make`
- CMake：`cmake -S . -B build/cmake && cmake --build build/cmake`

## 测试

- PowerShell：`.\build.ps1 -Target test`
- Make：`make test`
- CMake/CTest：`ctest --test-dir build/cmake`

当前测试入口为 `tests/test_main.c`，共 34 项测试，覆盖正常、边界、异常、故障注入和 12 个端到端用例。

## Sanitizer

```powershell
.\build.ps1 -Target sanitize
```

编译参数包含：

```text
-fsanitize=address,undefined
-fno-omit-frame-pointer
-g
```

用于检查越界、释放后使用、未定义行为和内存泄漏。

## 交付文档

- `README.md`
- `docs/design.md`
- `docs/file_format.md`
- `docs/traceability.md`
- `docs/test_report.md`
- `docs/roles/01` 至 `docs/roles/06`

## 验证

- `build.ps1 -Target test` 已在干净 `build` 目录复现通过。
- 严格警告 `-Wall -Wextra -Wconversion -Wshadow -Wpedantic` 下无编译警告。
- 测试程序退出码 0 表示全部通过。
