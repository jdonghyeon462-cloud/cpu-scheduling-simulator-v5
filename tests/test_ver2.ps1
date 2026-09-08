#requires -Version 7.0
param([string] $Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
& (Join-Path $project 'scripts/build.ps1') -Compiler $Compiler
$programName = if ($IsWindows) { 'simulator.exe' } else { 'simulator' }
$program = Join-Path (Join-Path $project 'build') $programName
$script:checks = 0

function Invoke-Simulator {
    param([string[]] $InputLines, [string[]] $Options = @('--algorithm', 'sjf'), [int] $ExpectedExit = 0)
    $result = ($InputLines | & $program @Options 2>&1 | Out-String)
    if ($LASTEXITCODE -ne $ExpectedExit) { throw "Unexpected exit $LASTEXITCODE. $result" }
    return $result
}

function Assert-Contains([string] $Result, [string] $Expected) {
    if (-not $Result.Contains($Expected)) { throw "Missing: $Expected`n$Result" }
    $script:checks++
}

function Get-Rows([string] $Result) {
    return @($Result -split '\r?\n' | Where-Object { $_ -match '^P\d+\s+\d' } |
        ForEach-Object { ($_ -replace '\s+', ' ').Trim() })
}

function Assert-Rows([string] $Result, [string[]] $Expected) {
    $actual = Get-Rows $Result
    if (($actual -join "`n") -ne ($Expected -join "`n")) { throw "Unexpected process results:`n$Result" }
    $script:checks++
}

$comparisonInput = Get-Content (Join-Path $project 'examples/sjf-comparison.txt')
$sjf = Invoke-Simulator $comparisonInput
Assert-Rows $sjf @('P1 0 5 0 5 0 5', 'P3 2 1 5 6 3 4', 'P2 1 4 6 10 5 9')
Assert-Contains $sjf 'Average waiting time: 2.67'
Assert-Contains $sjf 'Average turnaround time: 6.00'

# 도착 전 작업을 선택하거나, 미래의 짧은 작업을 기다리며 일부러 쉬면 실패합니다.
$future = Invoke-Simulator (Get-Content (Join-Path $project 'examples/sjf-future.txt'))
Assert-Rows $future @('P1 0 4 0 4 0 4', 'P3 0 6 4 10 4 10', 'P2 10 1 10 11 0 1')
Assert-Contains $future 'Average waiting time: 1.33'

$ties = Invoke-Simulator (Get-Content (Join-Path $project 'examples/sjf-ties.txt'))
Assert-Rows $ties @('P1 0 5 0 5 0 5', 'P3 1 2 5 7 4 6', 'P4 1 2 7 9 6 8', 'P2 2 2 9 11 7 9')

$nonPreemptive = Invoke-Simulator @('2', '0', '10', '1', '1')
Assert-Rows $nonPreemptive @('P1 0 10 0 10 0 10', 'P2 1 1 10 11 9 10')
$boundary = Invoke-Simulator @('3', '0', '5', '0', '8', '5', '1')
Assert-Rows $boundary @('P1 0 5 0 5 0 5', 'P3 5 1 5 6 0 1', 'P2 0 8 6 14 6 14')
$idle = Invoke-Simulator (Get-Content (Join-Path $project 'examples/idle.txt'))
Assert-Contains $idle '[0, 3) IDLE'
Assert-Contains $idle '[5, 8) IDLE'
Assert-Rows $idle @('P1 3 2 3 5 0 2', 'P2 8 1 8 9 0 1')
$single = Invoke-Simulator @('1', '0', '1')
Assert-Rows $single @('P1 0 1 0 1 0 1')

$compare = Invoke-Simulator $comparisonInput @('--algorithm', 'compare')
$fcfs = Invoke-Simulator $comparisonInput @('--algorithm', 'fcfs')
$fcfsSection = [regex]::Match($compare, '(?s)=== FCFS Timeline ===(.*?)=== SJF Timeline ===').Groups[1].Value
$sjfSection = [regex]::Match($compare, '(?s)=== SJF Timeline ===(.*?)=== RR Timeline').Groups[1].Value
Assert-Rows $fcfsSection (Get-Rows $fcfs)
Assert-Rows $sjfSection (Get-Rows $sjf)
Assert-Contains $compare 'Waiting difference (FCFS - SJF): 1.00'
if ($compare -notmatch '(?m)^FCFS\s+3\.67\s+7\.00\s+10\s+3\.67\s*$' -or
    $compare -notmatch '(?m)^SJF\s+2\.67\s+6\.00\s+10\s+2\.67\s*$') { throw 'Comparison summary mismatch.' }
$script:checks++

$same = Invoke-Simulator @('1', '0', '1') @('--algorithm', 'compare')
Assert-Contains $same 'Waiting difference (FCFS - SJF): 0.00'
$tradeoff = Invoke-Simulator (Get-Content (Join-Path $project 'examples/sjf-tradeoff.txt')) @('--algorithm', 'compare')
Assert-Contains $tradeoff 'Waiting difference (FCFS - SJF): 0.33'

$menu = Invoke-Simulator (@('0', '5', 'abc', '3', '2') + $comparisonInput) @('--menu')
Assert-Contains $menu '=== Comparison (same input) ==='
if ([regex]::Matches($menu, 'Invalid input:').Count -ne 3) { throw 'Menu validation failed.' }
$script:checks++
$menuFcfs = Invoke-Simulator @('1', '1', '0', '1') @('--menu')
Assert-Contains $menuFcfs '=== FCFS Timeline ==='
$menuSjf = Invoke-Simulator @('2', '1', '0', '1') @('--menu')
Assert-Contains $menuSjf '=== SJF Timeline ==='
$help = Invoke-Simulator @() @('--help')
Assert-Contains $help 'Usage:'
foreach ($badOptions in @(@('--algorithm'), @('--algorithm', 'unknown'), @('--wat'), @('--help', 'extra'), @('--menu', 'extra'))) {
    $bad = Invoke-Simulator @() $badOptions 1
    Assert-Contains $bad 'Invalid command-line arguments.'
}
$eof = Invoke-Simulator @('2', '0', '1') @('--algorithm', 'sjf') 1
Assert-Contains $eof 'Input ended before all values were entered.'
$menuEof = Invoke-Simulator @() @('--menu') 1
Assert-Contains $menuEof 'Input ended before all values were entered.'
$invalid = Invoke-Simulator @('1', '-1', '0', '0', '1')
Assert-Contains $invalid 'Invalid input:'
Assert-Rows $invalid @('P1 0 1 0 1 0 1')

# 독립 참조 모델: 매 tick마다 현재 작업을 유지하거나 준비 목록에서 고릅니다.
$random = [System.Random]::new(20260908)
$culture = [Globalization.CultureInfo]::InvariantCulture
for ($case = 0; $case -lt 60; $case++) {
    $count = $random.Next(1, 15)
    $jobs = @()
    $lines = @([string]$count)
    for ($i = 1; $i -le $count; $i++) {
        $arrival = $random.Next(0, 30)
        $burst = $random.Next(1, 9)
        $jobs += [pscustomobject]@{ Id = $i; Arrival = $arrival; Burst = $burst }
        $lines += @([string]$arrival, [string]$burst)
    }
    $pending = @($jobs)
    $active = $null
    $time = 0
    $remaining = 0
    $start = 0
    $totalWaiting = 0
    $totalTurnaround = 0
    $expectedRows = @()
    while ($pending.Count -gt 0 -or $null -ne $active) {
        if ($null -eq $active) {
            $ready = @($pending | Where-Object { $_.Arrival -le $time } | Sort-Object Burst, Arrival, Id)
            if ($ready.Count -eq 0) { $time++; continue }
            $active = $ready[0]
            $pending = @($pending | Where-Object { $_.Id -ne $active.Id })
            $remaining = $active.Burst
            $start = $time
        }
        $time++
        $remaining--
        if ($remaining -eq 0) {
            $waiting = $start - $active.Arrival
            $turnaround = $time - $active.Arrival
            $expectedRows += "P$($active.Id) $($active.Arrival) $($active.Burst) $start $time $waiting $turnaround"
            $totalWaiting += $waiting
            $totalTurnaround += $turnaround
            $active = $null
        }
    }
    $actual = Invoke-Simulator $lines
    Assert-Rows $actual $expectedRows
    Assert-Contains $actual ('Average waiting time: ' + ($totalWaiting / $count).ToString('F2', $culture))
    Assert-Contains $actual ('Average turnaround time: ' + ($totalTurnaround / $count).ToString('F2', $culture))
}

# C 코어의 원본 보존, 동률 규칙, 최대 누적값 및 잘못된 API 입력 검증.
$coreName = if ($IsWindows) { 'test_scheduler.exe' } else { 'test_scheduler' }
$coreProgram = Join-Path (Join-Path $project 'build') $coreName
& $Compiler '-std=c11' '-Wall' '-Wextra' '-Wpedantic' '-Werror' '-I' (Join-Path $project 'src') `
    (Join-Path $PSScriptRoot 'test_scheduler.c') (Join-Path $project 'src/scheduler.c') '-o' $coreProgram
if ($LASTEXITCODE -ne 0) { throw 'C core test compilation failed.' }
& $coreProgram
if ($LASTEXITCODE -ne 0) { throw 'C core tests failed.' }
Write-Output "PASS: $script:checks ver2 CLI checks, including 60 SJF reference workloads."
