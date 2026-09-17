param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [string]$Version = "unknown",
    [int]$MinimumAliveSeconds = 178,
    [int]$LaunchTimeoutSeconds = 205
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
if (-not (Test-Path $PackageDirectory -PathType Container)) { throw "Package directory does not exist: $PackageDirectory" }
if ($MinimumAliveSeconds -lt 178) { throw "Visual evidence needs at least 178 seconds so all deterministic scenes can be captured." }
if ($LaunchTimeoutSeconds -le $MinimumAliveSeconds) { throw "LaunchTimeoutSeconds must be greater than MinimumAliveSeconds." }

$exeCandidates = @(Get-ChildItem -Path $PackageDirectory -Recurse -File -Filter 'GTT.exe')
if ($exeCandidates.Count -ne 1) { throw "Expected exactly one packaged GTT.exe, found $($exeCandidates.Count)." }
$exe = $exeCandidates[0]
$runtimeLog = Join-Path $PackageDirectory 'GTT_VISUAL_RUNTIME.log'
$visualDirectory = Join-Path $PackageDirectory 'DemoVisualEvidence'
$runtimeEvidencePath = Join-Path $PackageDirectory 'VISUAL_RUNTIME_SMOKE.json'
if (Test-Path $runtimeLog) { Remove-Item -Force $runtimeLog }
if (Test-Path $runtimeEvidencePath) { Remove-Item -Force $runtimeEvidencePath }
if (Test-Path $visualDirectory) { Remove-Item -Recurse -Force $visualDirectory }
New-Item -ItemType Directory -Path $visualDirectory -Force | Out-Null

$arguments = @('-unattended','-nosplash','-NoSound','-RenderOffscreen','-windowed','-ResX=1280','-ResY=720','-ForceRes','-GTTDemoSmokeScenario','-GTTVisualEvidence',"-GTTVisualEvidenceDir=`"$visualDirectory`"",'-log',"-abslog=`"$runtimeLog`"")
$startedUtc = (Get-Date).ToUniversalTime()
$process = $null
$survivedSeconds = 0

try {
    Write-Host "[GTT] Starting rendered demo visual evidence run: $($exe.FullName)"
    Write-Host "[GTT] IMPORTANT: this run intentionally does NOT use NullRHI."
    $process = Start-Process -FilePath $exe.FullName -ArgumentList $arguments -WorkingDirectory $exe.DirectoryName -PassThru
    $deadline = (Get-Date).AddSeconds($LaunchTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 1
        $process.Refresh()
        $survivedSeconds = [int]((Get-Date).ToUniversalTime() - $startedUtc).TotalSeconds
        if ($process.HasExited) { throw "GTT.exe exited during rendered visual evidence after $survivedSeconds seconds with code $($process.ExitCode)." }
        if ($survivedSeconds -ge $MinimumAliveSeconds) { break }
    }
    if ($survivedSeconds -lt $MinimumAliveSeconds) { throw "GTT.exe did not remain alive for required visual evidence duration $MinimumAliveSeconds seconds." }
    if (-not (Test-Path $runtimeLog -PathType Leaf)) { throw "Rendered runtime did not create expected log: $runtimeLog" }

    $expectedScenes = @('world_gameplay','law_pressure','native_vehicle','loaded_trailer','hud_overview')
    $missing = @()
    foreach ($scene in $expectedScenes) { $path = Join-Path $visualDirectory "GTT_visual_$scene.png"; if (-not (Test-Path $path -PathType Leaf)) { $missing += $scene } }
    if ($missing.Count -gt 0) { throw "Rendered runtime did not produce all planned screenshots. Missing: $($missing -join ', ')" }

    $evidence = [ordered]@{
        schema='gtt.visual-runtime-smoke.v1'; game='Grand Theft Tractor'; version=$Version; result='PASS'
        executable=[IO.Path]::GetRelativePath($PackageDirectory,$exe.FullName).Replace('\','/'); minimum_alive_seconds=$MinimumAliveSeconds; survived_seconds=$survivedSeconds
        started_utc=$startedUtc.ToString('o'); observed_utc=(Get-Date).ToUniversalTime().ToString('o'); runner=$env:RUNNER_NAME; git_sha=$env:GITHUB_SHA
        null_rhi=$false; render_offscreen=$true; requested_resolution='1280x720'; deterministic_demo_scenario=$true; visual_capture=$true
        human_visual_acceptance='NOT_PERFORMED'; runtime_log='GTT_VISUAL_RUNTIME.log'; screenshot_directory='DemoVisualEvidence'; scene_ids=$expectedScenes; terminated_by_capture_harness=$true
    }
    $evidence | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 $runtimeEvidencePath
    Write-Host "[GTT] Rendered visual evidence runtime passed after $survivedSeconds seconds. Human aesthetic review is still required."
}
finally {
    if ($process) { try { $process.Refresh(); if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue; Wait-Process -Id $process.Id -Timeout 10 -ErrorAction SilentlyContinue } } catch { } }
}
