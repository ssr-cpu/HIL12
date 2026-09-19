param(
    [ValidateSet("all", "test", "sanitize", "clean")]
    [string]$Target = "all"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"
$Include = Join-Path $ProjectRoot "include"
$Src = Join-Path $ProjectRoot "src"
$Tests = Join-Path $ProjectRoot "tests"
$CC = "gcc"

function New-BuildDirectory {
    if (-not (Test-Path -LiteralPath $BuildDir)) {
        New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
    }
}

$AppSources = Get-ChildItem -LiteralPath $Src -Filter *.c | ForEach-Object { $_.FullName }
$LibrarySources = $AppSources | Where-Object { $_ -notlike "*\main.c" }
$TestSources = @($LibrarySources) + @((Get-ChildItem -LiteralPath $Tests -Filter *.c | ForEach-Object { $_.FullName }))
$CommonFlags = @("-std=c17", "-Wall", "-Wextra", "-Wconversion", "-Wshadow", "-Wpedantic", "-I$Include")

switch ($Target) {
    "all" {
        New-BuildDirectory
        & $CC @CommonFlags $AppSources -o (Join-Path $BuildDir "hil.exe") -lm
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    "test" {
        New-BuildDirectory
        & $CC @CommonFlags $TestSources -o (Join-Path $BuildDir "hil_tests.exe") -lm
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & (Join-Path $BuildDir "hil_tests.exe")
        exit $LASTEXITCODE
    }
    "sanitize" {
        New-BuildDirectory
        & $CC @CommonFlags "-fsanitize=address,undefined" "-fno-omit-frame-pointer" "-g" $TestSources -o (Join-Path $BuildDir "hil_tests_asan.exe") -lm
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & (Join-Path $BuildDir "hil_tests_asan.exe")
        exit $LASTEXITCODE
    }
    "clean" {
        if (Test-Path -LiteralPath $BuildDir) {
            Remove-Item -LiteralPath $BuildDir -Recurse -Force
        }
    }
}
