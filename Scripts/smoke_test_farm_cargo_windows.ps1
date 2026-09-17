param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [string]$Version = "unknown",
    [int]$LaunchTimeoutSeconds = 70
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
if (-not (Test-Path $PackageDirectory -PathType Container)) { throw "Package directory does not exist: $PackageDirectory" }
if ($LaunchTimeoutSeconds -lt 30) { throw 'LaunchTimeoutSeconds must be at least 30 seconds.' }

$exeCandidates = @(Get-ChildItem -Path $PackageDirectory -Recurse -File -Filter 'GTT.exe')
if ($exeCandidates.Count -ne 1) { throw "Expected exactly one packaged GTT.exe, found $($exeCandidates.Count)." }
$exe = $exeCandidates[0]
$runtimeLog = Join-Path $PackageDirectory 'GTT_FARM_CARGO_RUNTIME.log'
$userDir = Join-Path $PackageDirectory 'FarmCargoRuntimeUser'
if (Test-Path $runtimeLog) { Remove-Item -Force $runtimeLog }
if (Test-Path $userDir) { Remove-Item -Recurse -Force $userDir }
New-Item -ItemType Directory -Force -Path $userDir | Out-Null

$arguments = @(
    '-unattended', '-nosplash', '-nullrhi', '-NoSound', '-GTTFarmCargoScenario',
    '-SaveToUserDir', "-UserDir=$userDir", '-log', "-abslog=$runtimeLog"
)
$startedUtc = (Get-Date).ToUniversalTime()
$process = $null
$passed = $false

try {
    Write-Host "[GTT] Starting packaged Farm Cargo vertical-slice runtime exercise: $($exe.FullName)"
    $process = Start-Process -FilePath $exe.FullName -ArgumentList $arguments -WorkingDirectory $exe.DirectoryName -PassThru
    $deadline = (Get-Date).AddSeconds($LaunchTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 750
        $process.Refresh()
        if ($process.HasExited) {
            throw "GTT.exe exited before Farm Cargo scenario completed (code $($process.ExitCode))."
        }
        if (-not (Test-Path $runtimeLog)) { continue }
        $tail = Get-Content -Raw $runtimeLog
        if ($tail -match 'FARM_CARGO_SCENARIO_COMPLETE result=FAIL') {
            throw 'Packaged Farm Cargo scenario reported FAIL.'
        }
        if ($tail -match 'FARM_CARGO_SCENARIO_COMPLETE result=PASS') {
            $passed = $true
            break
        }
    }
    if (-not $passed) { throw "Farm Cargo scenario did not PASS within $LaunchTimeoutSeconds seconds." }

    $observedUtc = (Get-Date).ToUniversalTime()
    $evidence = [ordered]@{
        schema = 'gtt.farm-cargo-smoke.v1'
        game = 'Grand Theft Tractor'
        version = $Version
        result = 'PASS'
        executable = [IO.Path]::GetRelativePath($PackageDirectory, $exe.FullName).Replace('\\','/')
        launch_arguments = $arguments
        started_utc = $startedUtc.ToString('o')
        observed_utc = $observedUtc.ToString('o')
        duration_seconds = [Math]::Round(($observedUtc - $startedUtc).TotalSeconds, 3)
        runner = $env:RUNNER_NAME
        git_sha = $env:GITHUB_SHA
        null_rhi = $true
        isolated_user_dir = 'FarmCargoRuntimeUser'
        runtime_log = 'GTT_FARM_CARGO_RUNTIME.log'
        terminated_by_smoke_test = $true
    }
    $evidencePath = Join-Path $PackageDirectory 'FARM_CARGO_SMOKE.json'
    $evidence | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 $evidencePath
    Write-Host "[GTT] Packaged Farm Cargo runtime exercise: PASS"
    Write-Host "[GTT] Evidence: $evidencePath"
}
finally {
    if ($process) {
        try {
            $process.Refresh()
            if (-not $process.HasExited) {
                Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
                Wait-Process -Id $process.Id -Timeout 10 -ErrorAction SilentlyContinue
            }
        } catch { }
    }
}
