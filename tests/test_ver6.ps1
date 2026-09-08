#requires -Version 7.0
param([string]$Compiler='gcc')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
& (Join-Path $root 'scripts/benchmark.ps1') -Cases 1 -Processes 1 -Compiler $Compiler | Out-Null
if($LASTEXITCODE -ne 0){throw 'Benchmark build failed.'}
$test=Join-Path $root $(if($IsWindows){'build/test_sjf_heap.exe'}else{'build/test_sjf_heap'})
& $Compiler -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I (Join-Path $root 'src') -I (Join-Path $root 'benchmarks') `
    (Join-Path $root 'tests/test_sjf_heap.c') (Join-Path $root 'benchmarks/reference_sjf.c') (Join-Path $root 'src/scheduler.c') -o $test
if($LASTEXITCODE -ne 0){throw 'Heap test build failed.'}
& $test
if($LASTEXITCODE -ne 0){throw 'Heap/reference comparisons failed.'}
node (Join-Path $root 'tests/test_ver6.js')
if($LASTEXITCODE -ne 0){throw 'Benchmark/filter tests failed.'}
