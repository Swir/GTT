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
$runtimeUserRoot = "$PackageDirectory.runtime-user"
if (Test-Path $runtimeLog) { Remove-Item -Force $runtimeLog }
if (Test-Path $runtimeUserRoot) { Remove-Item -Recurse -Force $runtimeUserRoot }
New-Item -ItemType Directory -Force -Path $runtimeUserRoot | Out-Null

function Stop-GTTPackageProcesses {
    param([string]$PackagePrefix)
    $prefix = $PackagePrefix.TrimEnd('\') + '\'
    $runtimeProcesses = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object {
        $_.Name -in @('GTT.exe','GTT-Win64-Shipping.exe') -and $_.ExecutablePath -like "$prefix*"
    })
    foreach ($runtimeProcess in $runtimeProcesses) {
        Stop-Process -Id $runtimeProcess.ProcessId -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $runtimeProcess.ProcessId -Timeout 10 -ErrorAction SilentlyContinue
    }
}

function Invoke-GTTRuntimePass {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string[]]$ScenarioArguments,
        [Parameter(Mandatory=$true)][int]$RequiredAliveSeconds,
        [Parameter(Mandatory=$true)][int]$TimeoutSeconds
    )
    if ($TimeoutSeconds -le $RequiredAliveSeconds) { throw "Runtime pass $Name timeout must exceed its minimum alive time." }

    $passLog = Join-Path $PackageDirectory ("GTT_RUNTIME_{0}.log" -f $Name)
    $userDir = Join-Path $runtimeUserRoot $Name
    if (Test-Path $passLog) { Remove-Item -Force $passLog }
    if (Test-Path $userDir) { Remove-Item -Recurse -Force $userDir }
    New-Item -ItemType Directory -Force -Path $userDir | Out-Null

    $arguments = @('-unattended','-nosplash','-nullrhi','-NoSound',"-UserDir=$userDir") +
        $ScenarioArguments + @('-log',"-abslog=$passLog","-GTTRuntimeEvidenceLog=$passLog")
    $startedUtc = (Get-Date).ToUniversalTime()
    $process = $null
    $survivedSeconds = 0
    $previousRuntimeEvidenceLog = $env:GTT_RUNTIME_EVIDENCE_LOG
    $env:GTT_RUNTIME_EVIDENCE_LOG = $passLog
    try {
        Write-Host "[GTT][RUNTIME-PASS] START name=$Name min=${RequiredAliveSeconds}s timeout=${TimeoutSeconds}s scenarios=$($ScenarioArguments -join ',') user_dir=$userDir"
        $process = Start-Process -FilePath $exe.FullName -ArgumentList $arguments -WorkingDirectory $exe.DirectoryName -PassThru
        $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Seconds 1
            $process.Refresh()
            $survivedSeconds = [int]((Get-Date).ToUniversalTime() - $startedUtc).TotalSeconds
            if ($process.HasExited) { throw "GTT.exe pass $Name exited after $survivedSeconds seconds with code $($process.ExitCode)." }
            if ($survivedSeconds -ge $RequiredAliveSeconds) { break }
        }
        if ($survivedSeconds -lt $RequiredAliveSeconds) { throw "GTT.exe pass $Name did not remain alive for required $RequiredAliveSeconds seconds." }
        if (-not (Test-Path $passLog -PathType Leaf)) { throw "Runtime pass $Name did not create expected log: $passLog" }
        Write-Host "[GTT][RUNTIME-PASS] PASS name=$Name survived=${survivedSeconds}s log=$passLog"
        return [ordered]@{
            name = $Name
            result = 'PASS'
            minimum_alive_seconds = $RequiredAliveSeconds
            survived_seconds = $survivedSeconds
            runtime_log = [IO.Path]::GetFileName($passLog)
            user_dir_isolated = $true
            scenario_arguments = $ScenarioArguments
            launch_arguments = $arguments
        }
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
        Stop-GTTPackageProcesses -PackagePrefix $PackageDirectory
        if ($null -eq $previousRuntimeEvidenceLog) {
            Remove-Item Env:GTT_RUNTIME_EVIDENCE_LOG -ErrorAction SilentlyContinue
        } else {
            $env:GTT_RUNTIME_EVIDENCE_LOG = $previousRuntimeEvidenceLog
        }
    }
}

$corePass = Invoke-GTTRuntimePass -Name 'CORE' -RequiredAliveSeconds 180 -TimeoutSeconds 210 -ScenarioArguments @(
    '-GTTDemoSmokeScenario',
    '-GTTDisableDrivetrainScenario',
    '-GTTDisableTrailerScenario',
    '-GTTDisableRecoveryChoiceScenario'
)
$nativePass = Invoke-GTTRuntimePass -Name 'NATIVE' -RequiredAliveSeconds 180 -TimeoutSeconds 210 -ScenarioArguments @(
    '-GTTDrivetrainRuntimeScenario',
    '-GTTTrailerRuntimeScenario'
)
$servicesPass = Invoke-GTTRuntimePass -Name 'SERVICES' -RequiredAliveSeconds $MinimumAliveSeconds -TimeoutSeconds $LaunchTimeoutSeconds -ScenarioArguments @(
    '-GTTFarmCargoRuntimeScenario',
    '-GTTFarmCargoRecoveryScenario',
    '-GTTFarmCargoBreakdownScenario',
    '-GTTFarmCargoDispatchScenario',
    '-GTTFarmCargoDispatchPersistenceScenario',
    '-GTTFarmCargoWorkshopRecoveryScenario',
    '-GTTWorkshopHoursRuntimeScenario',
    '-GTTWorkshopQueueRuntimeScenario',
    '-GTTWorkshopCapacityRuntimeScenario',
    '-GTTWorkshopPriorityPickupRuntimeScenario'
)
$passes = @($corePass,$nativePass,$servicesPass)

Set-Content -Encoding UTF8 -Path $runtimeLog -Value ""
foreach ($pass in $passes) {
    Add-Content -Encoding UTF8 -Path $runtimeLog -Value ("[GTT][RUNTIME-PASS] MERGE name={0} source={1}" -f $pass.name,$pass.runtime_log)
    $passPath = Join-Path $PackageDirectory $pass.runtime_log
    Add-Content -Encoding UTF8 -Path $runtimeLog -Value (Get-Content -Raw $passPath)
}
$totalSurvivedSeconds = [int](($passes | Measure-Object -Property survived_seconds -Sum).Sum)
$allLaunchArguments = @()
foreach ($pass in $passes) { $allLaunchArguments += @($pass.launch_arguments) }

$evidence = [ordered]@{
    game = 'Grand Theft Tractor'
    version = $Version
    result = 'PASS'
    executable = [IO.Path]::GetRelativePath($PackageDirectory, $exe.FullName).Replace('\','/')
    launch_arguments = $allLaunchArguments
    minimum_alive_seconds = $MinimumAliveSeconds
    survived_seconds = $totalSurvivedSeconds
    started_utc = (Get-Date).ToUniversalTime().ToString('o')
    observed_utc = (Get-Date).ToUniversalTime().ToString('o')
    runner = $env:RUNNER_NAME
    git_sha = $env:GITHUB_SHA
    null_rhi = $true
    deterministic_demo_scenario = $true
    native_drivetrain_runtime_scenario = $true
    native_trailer_runtime_scenario = $true
    farm_cargo_runtime_scenario = $true
    farm_cargo_recovery_runtime_scenario = $true
    farm_cargo_breakdown_runtime_scenario = $true
    farm_cargo_dispatch_runtime_scenario = $true
    farm_cargo_dispatch_persistence_runtime_scenario = $true
    farm_cargo_workshop_recovery_runtime_scenario = $true
    workshop_hours_runtime_scenario = $true
    workshop_queue_runtime_scenario = $true
    workshop_capacity_runtime_scenario = $true
    workshop_priority_pickup_runtime_scenario = $true
    isolated_user_dirs = $true
    runtime_passes = $passes
    runtime_log = 'GTT_RUNTIME.log'
    visual_acceptance = 'NOT_PERFORMED'
    terminated_by_smoke_test = $true
}
$evidencePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'
$evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $evidencePath
Write-Host "[GTT] Isolated packaged runtime smoke passed: CORE + NATIVE + SERVICES; total observed ${totalSurvivedSeconds}s."
Write-Host "[GTT] Canonical merged runtime log: $runtimeLog"
Write-Host "[GTT] Evidence: $evidencePath"
