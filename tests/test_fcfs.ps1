#requires -Version 7.0
param([string] $Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
& (Join-Path $project 'scripts/build.ps1') -Compiler $Compiler
$programName = if ($IsWindows) { 'simulator.exe' } else { 'simulator' }
$program = Join-Path (Join-Path $project 'build') $programName
$script:checks = 0

function Invoke-Simulator([string[]] $InputLines, [int] $ExpectedExit = 0) {
    $result = ($InputLines | & $program 2>&1 | Out-String)
    if ($LASTEXITCODE -ne $ExpectedExit) {
        throw "Expected exit $ExpectedExit, received $LASTEXITCODE. $result"
    }
    return $result
}

function Assert-Contains([string] $Result, [string] $Expected) {
    if (-not $Result.Contains($Expected)) { throw "Missing: $Expected`n$Result" }
    $script:checks++
}

$basic = Invoke-Simulator (Get-Content (Join-Path $project 'examples\basic.txt'))
Assert-Contains $basic '[0, 5) P1'
Assert-Contains $basic '[5, 8) P2'
Assert-Contains $basic '[8, 9) P3'
Assert-Contains $basic 'Average waiting time: 3.33'
Assert-Contains $basic 'Average turnaround time: 6.33'

$idle = Invoke-Simulator (Get-Content (Join-Path $project 'examples\idle.txt'))
Assert-Contains $idle '[0, 3) IDLE'
Assert-Contains $idle '[5, 8) IDLE'
Assert-Contains $idle 'Average waiting time: 0.00'
Assert-Contains $idle 'Average turnaround time: 1.50'

$ties = Invoke-Simulator (Get-Content (Join-Path $project 'examples\ties-unsorted.txt'))
Assert-Contains $ties '[0, 3) P2'
Assert-Contains $ties '[3, 5) P3'
Assert-Contains $ties '[5, 7) P1'
Assert-Contains $ties 'Average waiting time: 1.00'

$single = Invoke-Simulator @('1', '0', '1')
Assert-Contains $single 'Average waiting time: 0.00'
Assert-Contains $single 'Average turnaround time: 1.00'

$limitInput = @('100')
for ($i = 0; $i -lt 100; $i++) { $limitInput += @('1000000', '1000000') }
$limit = Invoke-Simulator $limitInput
Assert-Contains $limit '[100000000, 101000000) P100'
Assert-Contains $limit 'Average waiting time: 49500000.00'
Assert-Contains $limit 'Average turnaround time: 50500000.00'

$invalid = Invoke-Simulator @('0', '101', 'abc', '1.5', '999999999999999999999999999', '1', '-1', '1000001', '0', '0', '-2', '1000001', '1')
Assert-Contains $invalid 'Average turnaround time: 1.00'
if ([regex]::Matches($invalid, 'Invalid input:').Count -ne 10) { throw 'Invalid input was not consistently rejected.' }
$script:checks++

$longLine = Invoke-Simulator @(('9' * 200), '1', '0', '1')
Assert-Contains $longLine 'Invalid input: line is too long.'
Assert-Contains $longLine 'Average turnaround time: 1.00'
$blank = Invoke-Simulator @('', '  1  ', ' 0 ', '1 ')
Assert-Contains $blank 'Invalid input:'
Assert-Contains $blank 'Average turnaround time: 1.00'
$eof = Invoke-Simulator @('2', '0', '1') 1
Assert-Contains $eof 'Input ended before all values were entered.'

$longFirst = Invoke-Simulator (Get-Content (Join-Path $project 'examples/long-first.txt'))
Assert-Contains $longFirst '[20, 22) P2'
Assert-Contains $longFirst 'Average waiting time: 14.00'
Assert-Contains $longFirst 'Average turnaround time: 21.67'
$shortFirst = Invoke-Simulator (Get-Content (Join-Path $project 'examples/short-first.txt'))
Assert-Contains $shortFirst '[3, 23) P3'
Assert-Contains $shortFirst 'Average waiting time: 1.33'
Assert-Contains $shortFirst 'Average turnaround time: 9.00'

# Deterministic workloads are checked against an independent tick-by-tick model.
$random = [System.Random]::new(42)
for ($case = 0; $case -lt 40; $case++) {
    $count = $random.Next(1, 15)
    $jobs = @()
    $inputLines = @([string]$count)
    for ($i = 1; $i -le $count; $i++) {
        $arrival = $random.Next(0, 30)
        $burst = $random.Next(1, 8)
        $jobs += [pscustomobject]@{ Id = $i; Arrival = $arrival; Burst = $burst }
        $inputLines += @([string]$arrival, [string]$burst)
    }
    $remaining = @($jobs)
    $time = 0
    $expectedRows = @()
    while ($remaining.Count -gt 0) {
        $ready = @($remaining | Where-Object { $_.Arrival -le $time } | Sort-Object Arrival, Id)
        if ($ready.Count -eq 0) { $time++; continue }
        $job = $ready[0]
        $start = $time
        for ($tick = 0; $tick -lt $job.Burst; $tick++) { $time++ }
        $expectedRows += "P$($job.Id) $($job.Arrival) $($job.Burst) $start $time $($start - $job.Arrival) $($time - $job.Arrival)"
        $remaining = @($remaining | Where-Object { $_.Id -ne $job.Id })
    }
    $actual = Invoke-Simulator $inputLines
    $actualRows = @($actual -split '\r?\n' | Where-Object { $_ -match '^P\d+\s+\d' } | ForEach-Object { ($_ -replace '\s+', ' ').Trim() })
    if (($actualRows -join "`n") -ne ($expectedRows -join "`n")) { throw "Random workload $case failed." }
    $script:checks++
}

Write-Output "PASS: $script:checks checks, including 40 independent reference workloads."
