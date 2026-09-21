[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RunnerDirectory,

    [string]$RepositoryUrl = "https://github.com/Swir/GTT",

    [string]$RunnerName = "",

    [string]$WorkDirectory = "_work",

    [string]$EngineRoot = "",

    [string]$OutputPath = "",

    [switch]$PlanOnly,

    [switch]$Configure,

    [switch]$InstallService
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RequiredLabels = @("self-hosted", "windows", "x64", "unreal-5.8")
$CustomLabels = "unreal-5.8"
$Checks = @()
$MutationPerformed = $false
$ConfigurationAttempted = $false
$ServiceAttempted = $false

function Add-Check {
    param(
        [string]$Name,
        [bool]$Pass,
        [string]$Detail
    )
    $script:Checks += [ordered]@{
        name = $Name
        pass = $Pass
        detail = $Detail
    }
}

function Resolve-FullPath {
    param([string]$PathValue)
    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        return ""
    }
    return [System.IO.Path]::GetFullPath($PathValue)
}

function Write-ReadinessReport {
    param(
        [string]$Result,
        [string]$FailureReason = ""
    )

    $report = [ordered]@{
        schema = "gtt.win64-runner-provisioning.v1"
        result = $Result
        generated_utc = [DateTime]::UtcNow.ToString("o")
        repository_url = $RepositoryUrl
        runner_directory = $RunnerDirectory
        runner_name = $RunnerName
        engine_root = $EngineRoot
        required_labels = $RequiredLabels
        custom_labels_requested = @($CustomLabels)
        default_labels_required = @("self-hosted", "windows", "x64")
        configured_locally = (Test-Path (Join-Path $RunnerDirectory ".runner") -PathType Leaf)
        configure_attempted = $ConfigurationAttempted
        install_service_attempted = $ServiceAttempted
        mutation_performed = $MutationPerformed
        qualification_required = $true
        exact_candidate_binding = "NONE - provisioning/readiness only"
        roadmap_gate_closed = $false
        native_chaos_runtime_verified = $false
        authored_trailer_runtime_verified = $false
        packaged_exe_smoke_verified = $false
        human_visual_review = "REQUIRED"
        demo_release_authorized = $false
        failure_reason = $FailureReason
        checks = $Checks
    }

    if ([string]::IsNullOrWhiteSpace($OutputPath)) {
        $script:OutputPath = Join-Path $RunnerDirectory "GTT_WIN64_RUNNER_PROVISIONING.json"
    }
    $parent = Split-Path -Parent $OutputPath
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    $report | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath
    Write-Host "[GTT][RUNNER-PROVISION] $Result -> $OutputPath"
}

try {
    if ($PlanOnly -and $Configure) {
        throw "-PlanOnly and -Configure are mutually exclusive."
    }
    if (-not $PlanOnly -and -not $Configure) {
        $PlanOnly = $true
    }
    if ($InstallService -and -not $Configure) {
        throw "-InstallService requires -Configure."
    }

    $RunnerDirectory = Resolve-FullPath $RunnerDirectory
    if ([string]::IsNullOrWhiteSpace($RunnerName)) {
        $hostName = if ([string]::IsNullOrWhiteSpace($env:COMPUTERNAME)) { "gtt-host" } else { $env:COMPUTERNAME }
        $RunnerName = "$hostName-gtt-ue58"
    }

    if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
        foreach ($candidate in @(
            $env:GTT_UE58_ROOT,
            $env:UE58_ROOT,
            "C:\Program Files\Epic Games\UE_5.8"
        )) {
            if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate -PathType Container)) {
                $EngineRoot = $candidate
                break
            }
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) {
        $EngineRoot = Resolve-FullPath $EngineRoot
    }

    $hostIsWindows = $env:OS -eq "Windows_NT"
    Add-Check "windows-host" $hostIsWindows "Host must be Windows."
    Add-Check "x64-os" ([Environment]::Is64BitOperatingSystem) "Host OS must be 64-bit."
    Add-Check "x64-process" ([Environment]::Is64BitProcess) "Provisioning shell must be 64-bit."

    $runnerDirExists = Test-Path $RunnerDirectory -PathType Container
    Add-Check "runner-directory" $runnerDirExists "Extract the official GitHub Actions Windows x64 runner package here."

    $ConfigCmd = Join-Path $RunnerDirectory "config.cmd"
    $RunCmd = Join-Path $RunnerDirectory "run.cmd"
    $SvcCmd = Join-Path $RunnerDirectory "svc.cmd"
    Add-Check "runner-config-cmd" (Test-Path $ConfigCmd -PathType Leaf) "config.cmd must come from the official runner package."
    Add-Check "runner-run-cmd" (Test-Path $RunCmd -PathType Leaf) "run.cmd must come from the official runner package."
    if ($InstallService) {
        Add-Check "runner-svc-cmd" (Test-Path $SvcCmd -PathType Leaf) "svc.cmd is required for service installation."
    }

    $enginePresent = -not [string]::IsNullOrWhiteSpace($EngineRoot) -and (Test-Path $EngineRoot -PathType Container)
    Add-Check "ue58-root" $enginePresent "Set -EngineRoot, GTT_UE58_ROOT or UE58_ROOT to a real UE 5.8 installation."

    if ($enginePresent) {
        $BuildVersion = Join-Path $EngineRoot "Engine\Build\Build.version"
        $RunUat = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
        $EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
        $UbtDll = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
        $UbtExe = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

        $buildVersionOk = $false
        $buildDetail = "Build.version missing or unreadable."
        if (Test-Path $BuildVersion -PathType Leaf) {
            try {
                $versionDoc = Get-Content -Raw $BuildVersion | ConvertFrom-Json
                $buildVersionOk = ([int]$versionDoc.MajorVersion -eq 5 -and [int]$versionDoc.MinorVersion -eq 8)
                $buildDetail = "Detected UE $($versionDoc.MajorVersion).$($versionDoc.MinorVersion)."
            } catch {
                $buildDetail = "Build.version parse failed: $($_.Exception.Message)"
            }
        }
        Add-Check "ue58-version" $buildVersionOk $buildDetail
        Add-Check "runuat" (Test-Path $RunUat -PathType Leaf) "RunUAT.bat is required by the package pipeline."
        Add-Check "unreal-editor-cmd" (Test-Path $EditorCmd -PathType Leaf) "UnrealEditor-Cmd.exe is required by qualification/runtime evidence."
        Add-Check "unreal-build-tool" ((Test-Path $UbtDll -PathType Leaf) -or (Test-Path $UbtExe -PathType Leaf)) "UnrealBuildTool is required."
    }

    $git = Get-Command git -ErrorAction SilentlyContinue
    Add-Check "git" ($null -ne $git) "Git must be available to the runner service account."
    if ($null -ne $git) {
        & git lfs version *> $null
        Add-Check "git-lfs" ($LASTEXITCODE -eq 0) "Git LFS must be available to hydrate the exact candidate."
    } else {
        Add-Check "git-lfs" $false "Git unavailable, so Git LFS cannot be verified."
    }

    $hasFailure = @($Checks | Where-Object { -not $_.pass }).Count -gt 0
    if ($hasFailure) {
        Write-ReadinessReport -Result "FAIL" -FailureReason "Host/package prerequisites are incomplete."
        exit 2
    }

    if ($PlanOnly) {
        Write-ReadinessReport -Result "PASS"
        Write-Host "[GTT][RUNNER-PROVISION] Readiness only. No registration/service mutation was performed."
        exit 0
    }

    $ConfigurationAttempted = $true
    $runnerMarker = Join-Path $RunnerDirectory ".runner"
    if (-not (Test-Path $runnerMarker -PathType Leaf)) {
        $Token = $env:GTT_GITHUB_RUNNER_TOKEN
        if ([string]::IsNullOrWhiteSpace($Token)) {
            Add-Check "registration-token" $false "Set short-lived GTT_GITHUB_RUNNER_TOKEN only for the registration command."
            Write-ReadinessReport -Result "FAIL" -FailureReason "Registration token is required for -Configure."
            exit 3
        }

        Write-Host "[GTT][RUNNER-PROVISION] Registering runner without echoing the registration token."
        & $ConfigCmd `
            --url $RepositoryUrl `
            --token $Token `
            --name $RunnerName `
            --work $WorkDirectory `
            --labels $CustomLabels `
            --unattended `
            --replace *> $null
        $configExit = $LASTEXITCODE
        $Token = $null
        if ($configExit -ne 0) {
            Add-Check "runner-registration" $false "config.cmd failed with exit code $configExit."
            Write-ReadinessReport -Result "FAIL" -FailureReason "GitHub Actions runner registration failed."
            exit 4
        }
        $MutationPerformed = $true
    }

    $configuredNow = Test-Path $runnerMarker -PathType Leaf
    Add-Check "runner-registration" $configuredNow "A local .runner marker must exist after registration."
    if (-not $configuredNow) {
        Write-ReadinessReport -Result "FAIL" -FailureReason "Runner registration marker is missing."
        exit 5
    }

    if ($InstallService) {
        $ServiceAttempted = $true
        Write-Host "[GTT][RUNNER-PROVISION] Installing/starting GitHub Actions runner service."
        & $SvcCmd install *> $null
        $installExit = $LASTEXITCODE
        Add-Check "runner-service-install" ($installExit -eq 0) "svc.cmd install exit=$installExit"
        if ($installExit -ne 0) {
            Write-ReadinessReport -Result "FAIL" -FailureReason "Runner service installation failed."
            exit 6
        }
        & $SvcCmd start *> $null
        $startExit = $LASTEXITCODE
        Add-Check "runner-service-start" ($startExit -eq 0) "svc.cmd start exit=$startExit"
        if ($startExit -ne 0) {
            Write-ReadinessReport -Result "FAIL" -FailureReason "Runner service start failed."
            exit 7
        }
        $MutationPerformed = $true
    }

    Write-ReadinessReport -Result "PASS"
    Write-Host "[GTT][RUNNER-PROVISION] Provisioning passed. This still does NOT qualify a candidate."
    Write-Host "[GTT][RUNNER-PROVISION] Next authority: GTT Win64 UE 5.8 runner qualification workflow."
    exit 0
}
catch {
    try {
        Add-Check "script-exception" $false $_.Exception.Message
        Write-ReadinessReport -Result "FAIL" -FailureReason $_.Exception.Message
    } catch {
        Write-Error "[GTT][RUNNER-PROVISION] Unable to write failure report: $($_.Exception.Message)"
    }
    Write-Error "[GTT][RUNNER-PROVISION] $($_.Exception.Message)"
    exit 1
}
