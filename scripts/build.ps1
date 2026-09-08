#requires -Version 7.0
param([string] $Compiler = 'gcc')
$ErrorActionPreference = 'Stop'

# 현재 터미널 위치가 아니라 이 스크립트 위치에서 경로를 구합니다.
$projectRoot = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $projectRoot 'src/main.c'
$schedulerPath = Join-Path $projectRoot 'src/scheduler.c'
$dataPath = Join-Path $projectRoot 'src/data_io.c'
$buildDirectory = Join-Path $projectRoot 'build'
$executableName = if ($IsWindows) { 'simulator.exe' } else { 'simulator' }
$executablePath = Join-Path $buildDirectory $executableName

if (-not (Get-Command $Compiler -ErrorAction SilentlyContinue)) {
    throw "C compiler not found: $Compiler. Install GCC or pass -Compiler with its path."
}

New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
& $Compiler '-std=c11' '-Wall' '-Wextra' '-Wpedantic' '-Werror' $sourcePath $schedulerPath $dataPath '-o' $executablePath
if ($LASTEXITCODE -ne 0) { throw "C compilation failed (exit $LASTEXITCODE)." }
Write-Host "Build succeeded: $executablePath"
