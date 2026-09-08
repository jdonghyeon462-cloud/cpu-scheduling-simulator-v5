#requires -Version 7.0
param(
    [ValidateRange(1,10000)][int]$Cases=500,
    [ValidateRange(1,100)][int]$Processes=100,
    [uint32]$Seed=42,
    [ValidateSet('mixed','burst','sparse')][string]$Pattern='mixed',
    [string]$Csv,
    [string]$Compiler='gcc'
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Force -Path $build | Out-Null
$binary=Join-Path $build $(if($IsWindows){'benchmark.exe'}else{'benchmark'})
& $Compiler -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I (Join-Path $root 'src') `
    (Join-Path $root 'benchmarks/benchmark.c') (Join-Path $root 'benchmarks/reference_sjf.c') `
    (Join-Path $root 'src/scheduler.c') -o $binary
if($LASTEXITCODE -ne 0){throw 'Benchmark compilation failed.'}
$options=@('--cases',"$Cases",'--processes',"$Processes",'--seed',"$Seed",'--pattern',$Pattern)
if($Csv){
    $target=if([IO.Path]::IsPathRooted($Csv)){$Csv}else{Join-Path $root $Csv}
    $options+=@('--csv',$target)
}
& $binary @options
if($LASTEXITCODE -ne 0){throw 'Benchmark failed.'}
