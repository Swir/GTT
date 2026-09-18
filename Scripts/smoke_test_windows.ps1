param(
    [Parameter(Mandatory=$true)]
    [string]$PackageDirectory,
    [string]$Version = "unknown",
    [int]$MinimumAliveSeconds = 20,
    [int]$LaunchTimeoutSeconds = 35
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
if (-not (Test-Path $PackageDirectory -PathType Container)) { throw "Package directory does not exist: $PackageDirectory" }
if ($MinimumAliveSeconds -lt 20) { throw "MinimumAliveSeconds must be at least 20 seconds for deterministic gameplay evidence." }
if ($LaunchTimeoutSeconds -le $MinimumAliveSeconds) { throw "LaunchTimeoutSeconds must be greater than MinimumAliveSeconds." }

$exeCandidates = @(Get-ChildItem -Path $PackageDirectory -Recurse -File -Filter 'GTT.exe')
if ($exeCandidates.Count -ne 1) { throw "Expected exactly one packaged GTT.exe, found $($exeCandidates.Count)." }
$exe = $exeCandidates[0]
$runtimeLog = Join-Path $PackageDirectory 'GTT_RUNTIME.log'
if (Test-Path $runtimeLog) { Remove-Item -Force $runtimeLog }
$arguments = @(
    '-unattended', '-nosplash', '-nullrhi', '-NoSound',
    '-GTTDemoSmokeScenario', '-GTTFarmCargoRuntimeScenario', '-GTTFarmCargoRecoveryScenario',
    '-log', "-abslog=$runtimeLog"
)
$startedUtc = (Get-Date).ToUniversalTime()
$process = $null
$survivedSeconds = 0

try {
    Write-Host "[GTT] Starting deterministic packaged runtime smoke test: $($exe.FullName)"
    $process = Start-Process -FilePath $exe.FullName -ArgumentList $arguments -WorkingDirectory $exe.DirectoryName -PassThru
    $deadline = (Get-Date).AddSeconds($LaunchTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 1
        $process.Refresh()
        $survivedSeconds = [int]((Get-Date).ToUniversalTime() - $startedUtc).TotalSeconds
        if ($process.HasExited) { throw "GTT.exe exited during smoke test after $survivedSeconds seconds with code $($process.ExitCode)." }
        if ($survivedSeconds -ge $MinimumAliveSeconds) { break }
    }
    if ($survivedSeconds -lt $MinimumAliveSeconds) { throw "GTT.exe did not remain alive for required $MinimumAliveSeconds seconds." }
    if (-not (Test-Path $runtimeLog)) { throw "Packaged runtime did not create expected log: $runtimeLog" }

    $evidence = [ordered]@{
        game = 'Grand Theft Tractor'; version = $Version; result = 'PASS'
        executable = [IO.Path]::GetRelativePath($PackageDirectory, $exe.FullName).Replace('\','/')
        launch_arguments = $arguments; minimum_alive_seconds = $MinimumAliveSeconds; survived_seconds = $survivedSeconds
        started_utc = $startedUtc.ToString('o'); observed_utc = (Get-Date).ToUniversalTime().ToString('o')
        runner = $env:RUNNER_NAME; git_sha = $env:GITHUB_SHA; null_rhi = $true; deterministic_demo_scenario = $true
        farm_cargo_runtime_scenario = $true; farm_cargo_recovery_runtime_scenario = $true
        runtime_log = 'GTT_RUNTIME.log'; visual_acceptance = 'NOT_PERFORMED'; terminated_by_smoke_test = $true
    }
    $evidencePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'
    $evidence | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 $evidencePath
    Write-Host "[GTT] Runtime smoke test passed after $survivedSeconds seconds."
    Write-Host "[GTT] Evidence: $evidencePath"
}
finally {
    if ($process) {
        try { $process.Refresh(); if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue; Wait-Process -Id $process.Id -Timeout 10 -ErrorAction SilentlyContinue } } catch { }
    }
}
