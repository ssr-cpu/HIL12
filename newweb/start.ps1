# Virtual HIL Web Console launcher (Windows)
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File .\start.ps1 [-Port 8090]

param(
    [int]$Port = 8090
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe  = Join-Path $root 'backend\build\hil_web.exe'

if (-not (Test-Path $exe)) {
    Write-Host 'Backend executable not found, building first...' -ForegroundColor Yellow
    Push-Location (Join-Path $root 'backend')
    try {
        $src  = Get-ChildItem -Path 'src' -Filter '*.c' | ForEach-Object { $_.FullName }
        $core = Get-ChildItem -Path (Join-Path $root '..\virtual_hil\src') -Filter '*.c' |
                Where-Object { $_.Name -ne 'main.c' } | ForEach-Object { $_.FullName }
        if (-not (Test-Path 'build')) { New-Item -ItemType Directory -Path 'build' | Out-Null }
        $inc = Join-Path $root '..\virtual_hil\include'
        & gcc -std=c17 -Wall -Wextra -Wconversion -Wshadow -Wpedantic -O1 `
            -Iinclude "-I$inc" `
            -o $exe @src @core -lws2_32
        if ($LASTEXITCODE -ne 0) { throw 'build failed' }
    } finally {
        Pop-Location
    }
}

Write-Host "Starting Web console: http://127.0.0.1:$Port" -ForegroundColor Green
Write-Host 'Press Ctrl+C to stop.'
Set-Location $root
& $exe --port $Port --out data/out
