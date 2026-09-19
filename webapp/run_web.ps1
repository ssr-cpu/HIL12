$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$BuildDir = Join-Path $Root "backend\build"
$Sources = Get-ChildItem -LiteralPath (Join-Path $Root "backend\src") -Filter *.c | ForEach-Object { $_.FullName }
$Include = Join-Path $Root "backend\include"
$Output = Join-Path $BuildDir "web_server.exe"
$CC = "gcc"

if (-not (Get-Command $CC -ErrorAction SilentlyContinue)) {
    $CC = "D:\msys64\ucrt64\bin\gcc.exe"
}

if (-not (Test-Path -LiteralPath $BuildDir)) {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
}

& $CC -std=c17 -Wall -Wextra -Wconversion -Wshadow -Wpedantic "-I$Include" $Sources -o $Output -lws2_32
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Web server built successfully: $Output"
Write-Host "Open http://127.0.0.1:8080"
Push-Location $Root
try {
    & $Output
} finally {
    Pop-Location
}
