@echo off
setlocal
cd /d "%~dp0"

if not exist "backend\build\hil_web.exe" (
    echo [build] compiling backend...
    pushd backend
    if not exist build mkdir build
    gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Wpedantic -O1 ^
        -Iinclude -I..\..\virtual_hil\include ^
        -o build\hil_web.exe src\main.c src\http.c src\api.c src\session.c src\utils.c ^
        ..\..\virtual_hil\src\hil_common.c ..\..\virtual_hil\src\hil_logger.c ^
        ..\..\virtual_hil\src\hil_time.c ..\..\virtual_hil\src\hil_signal.c ^
        ..\..\virtual_hil\src\hil_ecu.c ..\..\virtual_hil\src\hil_bus.c ^
        ..\..\virtual_hil\src\hil_frame.c ..\..\virtual_hil\src\hil_fault.c ^
        ..\..\virtual_hil\src\hil_assert.c ..\..\virtual_hil\src\hil_script.c ^
        ..\..\virtual_hil\src\hil_config.c ..\..\virtual_hil\src\hil_csv.c ^
        ..\..\virtual_hil\src\hil_data_logger.c ..\..\virtual_hil\src\hil_report.c ^
        ..\..\virtual_hil\src\hil_executor.c -lws2_32
    if errorlevel 1 (
        echo [error] build failed, check gcc is installed
        popd
        pause
        exit /b 1
    )
    popd
)

echo.
echo Starting Web console at http://127.0.0.1:8090
echo Press Ctrl+C in this window to stop.
echo.
start "" http://127.0.0.1:8090
backend\build\hil_web.exe --port 8090 --out data/out
