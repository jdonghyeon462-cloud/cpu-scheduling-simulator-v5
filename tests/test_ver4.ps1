#requires -Version 7.0
param([string] $Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
& (Join-Path $project 'scripts/build.ps1') -Compiler $Compiler
$program = Join-Path $project $(if ($IsWindows) { 'build/simulator.exe' } else { 'build/simulator' })
$testRoot = Join-Path $project ('build/ver4-files-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$script:checks = 0
$utf8 = [Text.UTF8Encoding]::new($false)
$culture = [Globalization.CultureInfo]::InvariantCulture

function Check([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw $Message }
    $script:checks++
}
function Invoke-Simulator {
    param([string[]] $Options, [string[]] $InputLines = @(), [int] $ExpectedExit = 0)
    $result = ($InputLines | & $program @Options 2>&1 | Out-String)
    if ($LASTEXITCODE -ne $ExpectedExit) { throw "Unexpected exit $LASTEXITCODE (expected $ExpectedExit). $result" }
    return $result
}
function New-Input([string] $Name, [string] $Content) {
    $path = Join-Path $testRoot $Name
    [IO.File]::WriteAllText($path, $Content, $utf8)
    return $path
}

$source = Join-Path $project 'examples/rr-basic.txt'
$saved = Join-Path $testRoot 'saved input.txt'
$csvPath = Join-Path $testRoot 'results.csv'
$jsonPath = Join-Path $testRoot 'results.json'
$run = Invoke-Simulator @('--algorithm', 'compare', '--quantum', '2', '--input', $source,
    '--save-input', $saved, '--csv', $csvPath, '--json', $jsonPath)
Check ($run.Contains('Loaded 3 processes') -and $run.Contains('Saved JSON:')) 'File operation status missing.'
$json = Get-Content -LiteralPath $jsonPath -Raw | ConvertFrom-Json
$csv = @(Import-Csv -LiteralPath $csvPath)
Check ($json.schema_version -eq 1 -and $json.simulator_version -eq '5.0.0') 'JSON version metadata mismatch.'
Check ($json.time_unit -eq 'ticks' -and $json.assumptions.cpu_count -eq 1 -and
       $json.assumptions.context_switch_cost -eq 0 -and -not $json.assumptions.io_wait) 'Model metadata mismatch.'
Check ($json.input.Count -eq 3 -and $json.runs.Count -eq 3) 'Missing input or algorithm results.'
Check ($csv.Count -eq 12 -and $csv[0].PSObject.Properties.Name.Count -eq 20) 'CSV shape mismatch.'
Check (((Get-Content -LiteralPath $source) -join ',') -eq ((Get-Content -LiteralPath $saved) -join ',')) 'Saved input order changed.'

foreach ($algorithm in $json.runs) {
    $summary = @($csv | Where-Object { $_.record_type -eq 'SUMMARY' -and $_.algorithm -eq $algorithm.algorithm })
    Check ($summary.Count -eq 1) 'Missing CSV summary.'
    foreach ($field in @('average_waiting', 'average_turnaround', 'average_response', 'makespan')) {
        $value = [double]::Parse($summary[0].$field, $culture)
        Check ([math]::Abs($value - [double]$algorithm.summary.$field) -lt 1e-10) "Summary mismatch: $field"
    }
    Check ($summary[0].policy -eq $algorithm.policy -and $summary[0].time_unit -eq 'ticks' -and
           $summary[0].cpu_count -eq '1' -and $summary[0].context_switch_cost -eq '0' -and
           $summary[0].io_wait -eq 'false') 'CSV experiment metadata mismatch.'
    if ($algorithm.algorithm -eq 'RR') {
        Check ($algorithm.quantum -eq 2 -and $summary[0].quantum -eq '2') 'RR quantum missing.'
    } else {
        Check ($null -eq $algorithm.quantum -and $summary[0].quantum -eq '') 'Non-RR quantum should be empty/null.'
    }
    foreach ($p in $algorithm.processes) {
        $row = @($csv | Where-Object { $_.record_type -eq 'PROCESS' -and $_.algorithm -eq $algorithm.algorithm -and $_.pid -eq "P$($p.id)" })
        Check ($row.Count -eq 1) 'Missing process CSV row.'
        foreach ($field in @('arrival', 'burst', 'first_start', 'completion', 'waiting', 'turnaround', 'response')) {
            Check ([long]$row[0].$field -eq [long]$p.$field) "Process mismatch: $field"
        }
        $slices = @($algorithm.timeline | Where-Object { $null -ne $_.process_id -and $_.process_id -eq $p.id })
        $duration = ($slices | ForEach-Object { $_.end - $_.start } | Measure-Object -Sum).Sum
        Check ($duration -eq $p.burst -and $slices[0].start -eq $p.first_start -and
               $slices[-1].end -eq $p.completion) 'Timeline does not match process totals.'
    }
}
Check ($json.runs[2].timeline.Count -eq 6 -and $json.runs[2].summary.average_response -eq 1) 'RR reference example mismatch.'

# 저장한 입력을 다시 읽은 결과는 파일명과 무관하게 완전히 같아야 합니다.
$roundtrip = Join-Path $testRoot 'roundtrip.json'
$null = Invoke-Simulator @('--input', $saved, '--algorithm', 'compare', '--quantum', '2', '--json', $roundtrip)
Check ((Get-Content $jsonPath -Raw) -eq (Get-Content $roundtrip -Raw)) 'Round trip changed JSON result.'
$interactive = Join-Path $testRoot 'interactive.txt'
$null = Invoke-Simulator @('--save-input', $interactive) @('1', '4', '2')
Check (((Get-Content $interactive) -join ',') -eq '1,4,2') 'Interactive input saving failed.'

$validVariants = @("1`r`n0`r`n1`r`n", "1`n0`n1", " 1 `n 0 `n 1 `n `t`n")
for ($i = 0; $i -lt $validVariants.Count; $i++) {
    $path = New-Input "valid-$i.txt" $validVariants[$i]
    $result = Invoke-Simulator @('--input', $path)
    Check ($result.Contains('Average turnaround time: 1.00')) 'Valid line ending/spacing rejected.'
}
$bom = Join-Path $testRoot 'bom.txt'
[IO.File]::WriteAllText($bom, "1`r`n0`r`n1`r`n", [Text.UTF8Encoding]::new($true))
$result = Invoke-Simulator @('--input', $bom)
Check ($result.Contains('Average turnaround time: 1.00')) 'UTF-8 BOM rejected.'

$invalidInputs = @('', "0`n", "101`n", "1`n0`n", "1`n-1`n2`n", "1`n0`n0`n", "1`n1000001`n1`n",
    "1`n0`n1000001`n", "1`n0`n1.5`n", "1`n0`nabc`n", "1`n`n1`n", "1`n0`n1`n99`n",
    "1`n0`n1 garbage`n", "1`n0`n999999999999999999999999`n", "1`n0`n1`0`n", (('9' * 200) + "`n0`n1`n"))
for ($i = 0; $i -lt $invalidInputs.Count; $i++) {
    $path = New-Input "invalid-$i.txt" $invalidInputs[$i]
    $unused = Join-Path $testRoot "invalid-$i.json"
    $failure = Invoke-Simulator @('--input', $path, '--json', $unused) @() 1
    Check ($failure -match 'Invalid integer on line|Unexpected data after') 'Unclear file parse failure.'
    Check (-not (Test-Path -LiteralPath $unused)) 'Output created for malformed input.'
}
$badBom = Join-Path $testRoot 'bad-bom.txt'
[IO.File]::WriteAllBytes($badBom, [byte[]]@(239, 0, 0))
$failure = Invoke-Simulator @('--input', $badBom) @() 1
Check ($failure.Contains('Invalid input encoding')) 'Malformed BOM accepted.'
$failure = Invoke-Simulator @('--input', (Join-Path $testRoot 'absent.txt')) @() 1
Check ($failure.Contains('Cannot open input')) 'Missing input file not diagnosed.'

# 새 출력은 wx로 만들며 중간 실패 시 이번 호출에서 만든 파일만 정리합니다.
$existing = New-Input 'keep.txt' 'KEEP THIS CONTENT'
$rolledBack = Join-Path $testRoot 'rollback-input.txt'
$neverCreated = Join-Path $testRoot 'not-created.json'
$failure = Invoke-Simulator @('--input', $source, '--save-input', $rolledBack, '--csv', $existing, '--json', $neverCreated) @() 1
Check ($failure.Contains('Cannot create output')) 'Existing output accepted.'
Check ((Get-Content $existing -Raw) -eq 'KEEP THIS CONTENT') 'Existing file modified.'
Check (-not (Test-Path $rolledBack) -and -not (Test-Path $neverCreated)) 'Failed export left newly created files.'
$sameOutput = Join-Path $testRoot 'same-output.txt'
$failure = Invoke-Simulator @('--input', $source, '--csv', $sameOutput, '--json', $sameOutput) @() 1
Check (-not (Test-Path $sameOutput)) 'Output path collision left a partial file.'
$sourceHash = (Get-FileHash -LiteralPath $source).Hash
$null = Invoke-Simulator @('--input', $source, '--save-input', $source) @() 1
Check ((Get-FileHash -LiteralPath $source).Hash -eq $sourceHash) 'Input path collision modified source.'
$failure = Invoke-Simulator @('--input', $source, '--json', (Join-Path $testRoot 'missing-parent/result.json')) @() 1
Check ($failure.Contains('Cannot create output')) 'Missing parent not reported.'
$failure = Invoke-Simulator @('--input', $source, '--csv', $testRoot) @() 1
Check ($failure.Contains('Cannot create output')) 'Directory accepted as output file.'
foreach ($flags in @(@('--input'), @('--json'), @('--csv', ''), @('--save-input'), @('--input', $source, '--input', $source))) {
    $failure = Invoke-Simulator $flags @() 1
    Check ($failure.Contains('Invalid command-line arguments')) 'Malformed file option accepted.'
}

# 콘솔 표시 상한을 넘는 타임라인도 JSON에는 모두 저장합니다.
$longInput = New-Input 'long.txt' "1`n0`n201`n"
$longJson = Join-Path $testRoot 'long.json'
$run = Invoke-Simulator @('--input', $longInput, '--algorithm', 'rr', '--quantum', '1', '--json', $longJson)
$longResult = Get-Content $longJson -Raw | ConvertFrom-Json
Check ($run.Contains('Timeline truncated') -and $longResult.runs[0].timeline.Count -eq 201) 'JSON timeline was truncated.'
Check ($longResult.runs[0].timeline[-1].end -eq 201 -and $longResult.runs[0].summary.makespan -eq 201) 'Long JSON final time mismatch.'
$tooLong = New-Input 'too-long.txt' "1`n0`n100001`n"
$limitedJson = Join-Path $testRoot 'too-long.json'
$null = Invoke-Simulator @('--input', $tooLong, '--algorithm', 'rr', '--quantum', '1', '--json', $limitedJson) @() 1
Check (-not (Test-Path $limitedJson)) 'Output created after RR limit failure.'

foreach ($algorithm in @('fcfs', 'sjf', 'rr')) {
    $path = Join-Path $testRoot "idle-$algorithm.json"
    $null = Invoke-Simulator @('--input', (Join-Path $project 'examples/idle.txt'), '--algorithm', $algorithm, '--json', $path)
    $idle = Get-Content $path -Raw | ConvertFrom-Json
    $gaps = @($idle.runs[0].timeline | Where-Object { $null -eq $_.process_id })
    Check ($gaps.Count -eq 2 -and $gaps[0].start -eq 0 -and $gaps[0].end -eq 3 -and $gaps[1].start -eq 5 -and $gaps[1].end -eq 8) 'JSON idle records mismatch.'
    Check ($idle.runs.Count -eq 1 -and $idle.runs[0].algorithm -eq $algorithm.ToUpperInvariant()) 'Single algorithm export includes unwanted runs.'
}

$maxLines = @('100')
for ($i = 0; $i -lt 100; $i++) { $maxLines += @('1000000', '1000000') }
$maximum = New-Input 'maximum.txt' ($maxLines -join "`n")
$maximumJson = Join-Path $testRoot 'maximum.json'
$null = Invoke-Simulator @('--input', $maximum, '--algorithm', 'compare', '--quantum', '1000000', '--json', $maximumJson)
$maximumResult = Get-Content $maximumJson -Raw | ConvertFrom-Json
foreach ($algorithm in $maximumResult.runs) {
    Check ($algorithm.processes.Count -eq 100 -and $algorithm.summary.makespan -eq 101000000 -and
           $algorithm.summary.average_waiting -eq 49500000) 'Large integer/mean export mismatch.'
}

$pack = Join-Path $testRoot 'pack with spaces'
$scriptResult = pwsh -NoProfile -File (Join-Path $project 'scripts/run.ps1') -InputFile $saved -Algorithm compare -Quantum 2 -OutputDirectory $pack 2>&1 | Out-String
Check ($LASTEXITCODE -eq 0 -and (Test-Path (Join-Path $pack 'input.txt')) -and
       (Test-Path (Join-Path $pack 'results.csv')) -and (Test-Path (Join-Path $pack 'results.json'))) "Script export pack failed: $scriptResult"
$packResult = Get-Content (Join-Path $pack 'results.json') -Raw
Check ($packResult -eq (Get-Content $jsonPath -Raw)) 'Script pack differs from direct command.'
$scriptResult = pwsh -NoProfile -File (Join-Path $project 'scripts/run.ps1') -InputFile $saved -Example basic 2>&1 | Out-String
Check ($LASTEXITCODE -ne 0 -and $scriptResult.Contains('Use either -Example or -InputFile.')) 'Conflicting input sources accepted.'

Write-Output "PASS: $script:checks ver4 file I/O checks."
Write-Output "Fixtures and exports: $testRoot"
