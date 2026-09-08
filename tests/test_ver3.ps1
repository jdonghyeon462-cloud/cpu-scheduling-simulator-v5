#requires -Version 7.0
param([string] $Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
& (Join-Path $project 'scripts/build.ps1') -Compiler $Compiler
$program = Join-Path $project $(if ($IsWindows) { 'build/simulator.exe' } else { 'build/simulator' })
$script:checks = 0

function Invoke-Simulator {
    param([string[]] $InputLines, [string[]] $Options = @('--algorithm', 'rr', '--quantum', '2'), [int] $ExpectedExit = 0)
    $result = ($InputLines | & $program @Options 2>&1 | Out-String)
    if ($LASTEXITCODE -ne $ExpectedExit) { throw "Unexpected exit $LASTEXITCODE. $result" }
    return $result
}
function Assert-Contains([string] $Result, [string] $Expected) {
    if (-not $Result.Contains($Expected)) { throw "Missing: $Expected`n$Result" }
    $script:checks++
}
function Assert-Lines([string[]] $Actual, [string[]] $Expected) {
    if (($Actual -join "`n") -ne ($Expected -join "`n")) { throw "Expected:`n$($Expected -join "`n")`nActual:`n$($Actual -join "`n")" }
    $script:checks++
}
function Get-Rows([string] $Result) {
    @($Result -split '\r?\n' | Where-Object { $_ -match '^P\d+\s+\d' } | ForEach-Object { ($_ -replace '\s+', ' ').Trim() })
}
function Get-Timeline([string] $Result) {
    @($Result -split '\r?\n' | Where-Object { $_ -match '^\[\d+, \d+\) ' })
}

$basicInput = Get-Content (Join-Path $project 'examples/rr-basic.txt')
$basic = Invoke-Simulator $basicInput
Assert-Lines (Get-Timeline $basic) @('[0, 2) P1', '[2, 4) P2', '[4, 5) P3', '[5, 7) P1', '[7, 8) P2', '[8, 9) P1')
Assert-Lines (Get-Rows $basic) @('P1 0 5 0 9 4 9 0', 'P2 1 3 2 8 4 7 1', 'P3 2 1 4 5 2 3 2')
Assert-Contains $basic 'Average waiting time: 3.33'
Assert-Contains $basic 'Average turnaround time: 6.33'
Assert-Contains $basic 'Average response time: 1.00'
$default = Invoke-Simulator $basicInput @('--algorithm', 'rr')
Assert-Lines (Get-Timeline $default) (Get-Timeline $basic)
$reordered = Invoke-Simulator $basicInput @('--quantum', '2', '--algorithm', 'rr')
Assert-Lines (Get-Rows $reordered) (Get-Rows $basic)

$boundary = Invoke-Simulator (Get-Content (Join-Path $project 'examples/rr-boundary.txt'))
Assert-Lines (Get-Timeline $boundary) @('[0, 2) P1', '[2, 3) P2', '[3, 5) P1')
$arrivalOrder = Invoke-Simulator @('3', '0', '4', '2', '1', '1', '1')
Assert-Lines (Get-Timeline $arrivalOrder) @('[0, 2) P1', '[2, 3) P3', '[3, 4) P2', '[4, 6) P1')
$ties = Invoke-Simulator @('3', '0', '3', '2', '1', '2', '1')
Assert-Lines (Get-Timeline $ties) @('[0, 2) P1', '[2, 3) P2', '[3, 4) P3', '[4, 5) P1')
$idle = Invoke-Simulator (Get-Content (Join-Path $project 'examples/rr-idle.txt'))
Assert-Lines (Get-Timeline $idle) @('[0, 3) IDLE', '[3, 5) P1', '[5, 6) P1', '[6, 10) IDLE', '[10, 11) P2')
$complete = Invoke-Simulator @('2', '0', '2', '2', '1')
Assert-Lines (Get-Timeline $complete) @('[0, 2) P1', '[2, 3) P2')
$single = Invoke-Simulator @('1', '0', '5')
Assert-Lines (Get-Timeline $single) @('[0, 2) P1', '[2, 4) P1', '[4, 5) P1')
Assert-Contains $single 'Average waiting time: 0.00'
$large = Invoke-Simulator $basicInput @('--algorithm', 'rr', '--quantum', '1000000')
$fcfs = Invoke-Simulator $basicInput @('--algorithm', 'fcfs')
Assert-Lines (Get-Timeline $large) (Get-Timeline $fcfs)
$quantumOne = Invoke-Simulator $basicInput @('--algorithm', 'rr', '--quantum', '1')
Assert-Contains $quantumOne 'Average waiting time: 2.67'
Assert-Contains $quantumOne 'Average response time: 0.33'

$compare = Invoke-Simulator $basicInput @('--algorithm', 'compare', '--quantum', '2')
$rrSection = [regex]::Match($compare, '(?s)=== RR Timeline ===(.*?)=== Comparison').Groups[1].Value
Assert-Lines (Get-Timeline $rrSection) (Get-Timeline $basic)
Assert-Lines (Get-Rows $rrSection) (Get-Rows $basic)
if ($compare -notmatch '(?m)^RR\s+3\.33\s+6\.33\s+9\s+1\.00\s*$') { throw 'RR comparison summary mismatch.' }
$script:checks++
$menu = Invoke-Simulator (@('4', '0', '-1', '2') + $basicInput) @('--menu')
Assert-Lines (Get-Rows $menu) (Get-Rows $basic)
Assert-Contains $menu 'Invalid input:'
$menuEof = Invoke-Simulator @('4') @('--menu') 1
Assert-Contains $menuEof 'Input ended before all values were entered.'
foreach ($q in @('0', '-1', '1000001', '1.5', 'abc', '9999999999999999999999', '')) {
    $bad = Invoke-Simulator @() @('--algorithm', 'rr', '--quantum', $q) 1
    Assert-Contains $bad 'Invalid command-line arguments.'
}
foreach ($flags in @(@('--quantum'), @('--quantum', '2'), @('--algorithm', 'fcfs', '--quantum', '2'), @('--algorithm', 'rr', '--quantum', '2', '--quantum', '3'), @('--algorithm', 'rr', '--algorithm', 'rr'))) {
    $bad = Invoke-Simulator @() $flags 1
    Assert-Contains $bad 'Invalid command-line arguments.'
}
$limit = Invoke-Simulator @('1', '0', '100001') @('--algorithm', 'rr', '--quantum', '1') 1
Assert-Contains $limit 'RR requires more than 100000 execution slices.'
$limitCompare = Invoke-Simulator @('1', '0', '100001') @('--algorithm', 'compare', '--quantum', '1') 1
if ($limitCompare.Contains('=== FCFS Timeline ===')) { throw 'Partial comparison was printed after RR failure.' }
$script:checks++
$truncated = Invoke-Simulator @('1', '0', '201') @('--algorithm', 'rr', '--quantum', '1')
Assert-Contains $truncated 'Timeline truncated: showing 200 of 201 intervals.'
Assert-Lines (Get-Rows $truncated) @('P1 0 201 0 201 0 201 0')

# 참조 구현은 매 tick 도착을 처리하며, C 원형 큐 대신 List를 사용합니다.
$random = [System.Random]::new(202609083)
$culture = [Globalization.CultureInfo]::InvariantCulture
for ($case = 0; $case -lt 60; $case++) {
    $count = $random.Next(1, 11)
    $quantum = $random.Next(1, 9)
    $jobs = @()
    $lines = @([string]$count)
    for ($i = 0; $i -lt $count; $i++) {
        $arrival = $random.Next(0, 13)
        $burst = $random.Next(1, 7)
        $jobs += [pscustomobject]@{ Index=$i; Arrival=$arrival; Burst=$burst; Remaining=$burst; Start=-1; Completion=0 }
        $lines += @([string]$arrival, [string]$burst)
    }
    $ready = [Collections.Generic.List[int]]::new()
    $expectedTimeline = [Collections.Generic.List[string]]::new()
    $time = 0; $finished = 0; $active = -1; $requeue = -1; $used = 0; $sliceStart = 0; $lastEnd = 0
    while ($finished -lt $count) {
        foreach ($job in $jobs) { if ($job.Arrival -eq $time) { $ready.Add($job.Index) } }
        if ($requeue -ge 0) { $ready.Add($requeue); $requeue = -1 }
        if ($active -lt 0) {
            if ($ready.Count -eq 0) { $time++; continue }
            $active = $ready[0]; $ready.RemoveAt(0)
            $sliceStart = $time; $used = 0
            if ($jobs[$active].Start -lt 0) { $jobs[$active].Start = $time }
        }
        $jobs[$active].Remaining--; $time++; $used++
        if ($used -eq $quantum -or $jobs[$active].Remaining -eq 0) {
            if ($lastEnd -lt $sliceStart) { $expectedTimeline.Add("[$lastEnd, $sliceStart) IDLE") }
            $expectedTimeline.Add("[$sliceStart, $time) P$($active + 1)")
            $lastEnd = $time
            if ($jobs[$active].Remaining -eq 0) { $jobs[$active].Completion = $time; $finished++ }
            else { $requeue = $active }
            $active = -1
        }
    }
    $expectedRows = @(); $waitingTotal = 0; $turnaroundTotal = 0; $responseTotal = 0
    foreach ($job in $jobs) {
        $turnaround = $job.Completion - $job.Arrival
        $waiting = $turnaround - $job.Burst
        $response = $job.Start - $job.Arrival
        $expectedRows += "P$($job.Index + 1) $($job.Arrival) $($job.Burst) $($job.Start) $($job.Completion) $waiting $turnaround $response"
        $waitingTotal += $waiting; $turnaroundTotal += $turnaround; $responseTotal += $response
    }
    $actual = Invoke-Simulator $lines @('--algorithm', 'rr', '--quantum', [string]$quantum)
    Assert-Lines (Get-Timeline $actual) $expectedTimeline.ToArray()
    Assert-Lines (Get-Rows $actual) $expectedRows
    Assert-Contains $actual ('Average waiting time: ' + ($waitingTotal / $count).ToString('F2', $culture))
    Assert-Contains $actual ('Average turnaround time: ' + ($turnaroundTotal / $count).ToString('F2', $culture))
    Assert-Contains $actual ('Average response time: ' + ($responseTotal / $count).ToString('F2', $culture))
}
$core = Join-Path $project $(if ($IsWindows) { 'build/test_round_robin.exe' } else { 'build/test_round_robin' })
& $Compiler '-std=c11' '-Wall' '-Wextra' '-Wpedantic' '-Werror' '-I' (Join-Path $project 'src') `
    (Join-Path $PSScriptRoot 'test_round_robin.c') (Join-Path $project 'src/scheduler.c') '-o' $core
if ($LASTEXITCODE -ne 0) { throw 'RR C test build failed.' }
& $core
if ($LASTEXITCODE -ne 0) { throw 'RR C tests failed.' }
Write-Output "PASS: $script:checks ver3 CLI checks, including 60 RR reference workloads."
