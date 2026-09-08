#requires -Version 7.0
param(
    [ValidateSet('basic', 'idle', 'ties-unsorted', 'long-first', 'short-first', 'sjf-comparison', 'sjf-future', 'sjf-ties', 'sjf-tradeoff', 'rr-basic', 'rr-boundary', 'rr-idle', 'rr-response')]
    [string] $Example,
    [ValidateSet('fcfs', 'sjf', 'rr', 'compare')]
    [string] $Algorithm = 'fcfs',
    [ValidateRange(1, 1000000)]
    [long] $Quantum = 2,
    [string] $InputFile,
    [string] $OutputDirectory,
    [string] $Compiler = 'gcc'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if ($Example -and $InputFile) { throw 'Use either -Example or -InputFile.' }
if ($PSBoundParameters.ContainsKey('Quantum') -and $Algorithm -notin @('rr', 'compare')) {
    throw '-Quantum applies only to rr or compare.'
}
& (Join-Path $PSScriptRoot 'build.ps1') -Compiler $Compiler
$executableName = if ($IsWindows) { 'simulator.exe' } else { 'simulator' }
$executablePath = Join-Path (Join-Path $projectRoot 'build') $executableName
$options = @('--algorithm', $Algorithm)
if ($Algorithm -in @('rr', 'compare')) { $options += @('--quantum', [string]$Quantum) }

if ($InputFile) {
    $inputPath = if ([IO.Path]::IsPathRooted($InputFile)) { $InputFile } else { Join-Path $projectRoot $InputFile }
    $options += @('--input', $inputPath)
}
if ($OutputDirectory) {
    $outputPath = if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $projectRoot $OutputDirectory }
    New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
    $options += @('--save-input', (Join-Path $outputPath 'input.txt'),
                  '--csv', (Join-Path $outputPath 'results.csv'),
                  '--json', (Join-Path $outputPath 'results.json'))
}

if ($Example) {
    $inputPath = Join-Path $projectRoot "examples/$Example.txt"
    & $executablePath @options --input $inputPath
} else {
    & $executablePath @options
}
exit $LASTEXITCODE
