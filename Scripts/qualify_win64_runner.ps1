param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = "",
    [string]$OutputPath = "",
    [string]$ExpectedGitSha = "",
    [string]$ExpectedVersion = "",
    [int]$MinimumFreeGiB = 25,
    [switch]$SkipEditorProbe,
    [int]$EditorProbeTimeoutSeconds = 120
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Preflight = Join-Path $PSScriptRoot "preflight_win64_unreal.ps1"
$ConfigPath = Join-Path $ProjectRoot "Config\DefaultGame.ini"

if ([string]::IsNullOrWhiteSpace($ProjectFile)) {
    $ProjectFile = Join-Path $ProjectRoot "GTT.uproject"
}
$ProjectFile = [System.IO.Path]::GetFullPath($ProjectFile)

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $ProjectRoot "Saved\Win64\WIN64_RUNNER_QUALIFICATION.json"
}
$OutputPath = [System.IO.Path]::GetFullPath($OutputPath)
$OutputParent = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $OutputParent | Out-Null

$PreflightPath = "$OutputPath.preflight.json"
$ProbeLog = "$OutputPath.editor-probe.log"
$checks = New-Object System.Collections.Generic.List[object]
$requiredFailure = $false

function Add-QualificationCheck {
    param(
        [string]$Name,
        [bool]$Passed,
        [string]$Detail,
        [bool]$Required = $true
    )
    $script:checks.Add([ordered]@{
        name = $Name
        required = $Required
        passed = $Passed
        detail = $Detail
    })
    if ($Required -and -not $Passed) {
        $script:requiredFailure = $true
    }
}

function Write-QualificationReport {
    param(
        [string]$Result,
        [string]$GitSha,
        [string]$Version,
        [string]$EngineVersion,
        [string]$PreflightResult,
        [string]$EditorProbeResult
    )
    $passed = @($script:checks | Where-Object { $_.passed }).Count
    $required = @($script:checks | Where-Object { $_.required }).Count
    $requiredPassed = @($script:checks | Where-Object { $_.required -and $_.passed }).Count

    $report = [ordered]@{
        schema = "gtt.win64-runner-qualification.v1"
        result = $Result
        game = "Grand Theft Tractor"
        platform = "Win64"
        engine = "Unreal Engine 5.8"
        engine_root = $EngineRoot
        detected_engine_version = $EngineVersion
        git_sha = $GitSha
        version = $Version
        preflight = $PreflightResult
        editor_probe = $EditorProbeResult
        human_visual_review = "REQUIRED"
        demo_release_authorized = $false
        generated_utc = (Get-Date).ToUniversalTime().ToString("o")
        host = [ordered]@{
            machine = $env:COMPUTERNAME
            os = [System.Environment]::OSVersion.VersionString
            powershell = $PSVersionTable.PSVersion.ToString()
        }
        summary = [ordered]@{
            passed = $passed
            total = $script:checks.Count
            required_passed = $requiredPassed
            required_total = $required
        }
        checks = $script:checks
        evidence = [ordered]@{
            preflight_json = [System.IO.Path]::GetFileName($PreflightPath)
            editor_probe_log = $(if ($SkipEditorProbe) { $null } else { [System.IO.Path]::GetFileName($ProbeLog) })
        }
    }
    $report | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath
}

$gitSha = "unknown"
$projectVersion = "unknown"
$engineVersion = "unknown"
$preflightResult = "NOT_RUN"
$editorProbeResult = "NOT_RUN"

try {
    Add-QualificationCheck "qualification-script-version" $true "schema=gtt.win64-runner-qualification.v1"

    $is64BitHost = [System.Environment]::Is64BitOperatingSystem -and [System.Environment]::Is64BitProcess
    $runnerArch = [string]$env:RUNNER_ARCH
    $archDetail = "os64=$([System.Environment]::Is64BitOperatingSystem); process64=$([System.Environment]::Is64BitProcess); RUNNER_ARCH=$runnerArch"
    Add-QualificationCheck "x64-host-process" $is64BitHost $archDetail
    if (-not [string]::IsNullOrWhiteSpace($runnerArch)) {
        Add-QualificationCheck "github-runner-arch-label" ($runnerArch -eq "X64") "RUNNER_ARCH=$runnerArch"
    }

    if (-not (Test-Path $Preflight -PathType Leaf)) {
        Add-QualificationCheck "preflight-script" $false "Missing $Preflight"
    } else {
        Add-QualificationCheck "preflight-script" $true $Preflight
    }

    if (-not (Test-Path $ConfigPath -PathType Leaf)) {
        Add-QualificationCheck "project-version-config" $false "Missing $ConfigPath"
    } else {
        $ini = Get-Content -Raw $ConfigPath
        $versionMatch = [regex]::Match($ini, '(?m)^ProjectVersion=(.+)$')
        if ($versionMatch.Success) {
            $projectVersion = $versionMatch.Groups[1].Value.Trim()
            Add-QualificationCheck "project-version-config" (-not [string]::IsNullOrWhiteSpace($projectVersion)) "ProjectVersion=$projectVersion"
        } else {
            Add-QualificationCheck "project-version-config" $false "ProjectVersion missing from Config/DefaultGame.ini"
        }
    }

    try {
        $candidateSha = (& git -C $ProjectRoot rev-parse HEAD 2>$null).Trim()
        if ($LASTEXITCODE -eq 0 -and $candidateSha -match '^[0-9a-fA-F]{40}$') {
            $gitSha = $candidateSha.ToLowerInvariant()
            Add-QualificationCheck "exact-git-sha" $true $gitSha
        } else {
            Add-QualificationCheck "exact-git-sha" $false "Unable to resolve exact 40-character HEAD SHA"
        }
    } catch {
        Add-QualificationCheck "exact-git-sha" $false "git rev-parse failed: $($_.Exception.Message)"
    }

    if (-not [string]::IsNullOrWhiteSpace($ExpectedGitSha)) {
        $expectedShaNormalized = $ExpectedGitSha.Trim().ToLowerInvariant()
        Add-QualificationCheck "expected-git-sha" ($gitSha -eq $expectedShaNormalized) "expected=$expectedShaNormalized actual=$gitSha"
    }

    if (-not [string]::IsNullOrWhiteSpace($ExpectedVersion)) {
        Add-QualificationCheck "expected-project-version" ($projectVersion -eq $ExpectedVersion) "expected=$ExpectedVersion actual=$projectVersion"
    }

    try {
        $dirty = (& git -C $ProjectRoot status --porcelain --untracked-files=no 2>$null) -join "`n"
        Add-QualificationCheck "clean-tracked-tree" ([string]::IsNullOrWhiteSpace($dirty)) $(if ([string]::IsNullOrWhiteSpace($dirty)) { "clean" } else { $dirty })
    } catch {
        Add-QualificationCheck "clean-tracked-tree" $false "git status failed: $($_.Exception.Message)"
    }

    try {
        $lfsFsck = (& git -C $ProjectRoot lfs fsck 2>&1) -join "`n"
        $lfsOk = $LASTEXITCODE -eq 0
        Add-QualificationCheck "git-lfs-fsck" $lfsOk $(if ($lfsOk) { "PASS" } else { $lfsFsck })
    } catch {
        Add-QualificationCheck "git-lfs-fsck" $false "git lfs fsck failed: $($_.Exception.Message)"
    }

    $writeProbeDir = Join-Path $ProjectRoot "Intermediate\GTT\RunnerQualification"
    try {
        New-Item -ItemType Directory -Force -Path $writeProbeDir | Out-Null
        $writeProbeFile = Join-Path $writeProbeDir "write-probe.tmp"
        [System.IO.File]::WriteAllText($writeProbeFile, "GTT runner qualification")
        $writeOk = (Test-Path $writeProbeFile -PathType Leaf) -and ((Get-Item $writeProbeFile).Length -gt 0)
        if (Test-Path $writeProbeFile) { Remove-Item -Force $writeProbeFile }
        Add-QualificationCheck "workspace-write" $writeOk $writeProbeDir
    } catch {
        Add-QualificationCheck "workspace-write" $false "Workspace write probe failed: $($_.Exception.Message)"
    }

    if (Test-Path $EngineRoot -PathType Container) {
        $buildVersionPath = Join-Path $EngineRoot "Engine\Build\Build.version"
        if (Test-Path $buildVersionPath -PathType Leaf) {
            try {
                $buildVersion = Get-Content -Raw $buildVersionPath | ConvertFrom-Json
                $engineVersion = "$($buildVersion.MajorVersion).$($buildVersion.MinorVersion).$($buildVersion.PatchVersion)"
            } catch {
                $engineVersion = "unparseable"
            }
        }
    }

    if (Test-Path $Preflight -PathType Leaf) {
        & $Preflight `
            -EngineRoot $EngineRoot `
            -ProjectFile $ProjectFile `
            -OutputPath $PreflightPath `
            -MinimumFreeGiB $MinimumFreeGiB
        $preflightExit = $LASTEXITCODE
        if (Test-Path $PreflightPath -PathType Leaf) {
            try {
                $preflightDoc = Get-Content -Raw $PreflightPath | ConvertFrom-Json
                $preflightResult = [string]$preflightDoc.result
            } catch {
                $preflightResult = "INVALID"
            }
        }
        Add-QualificationCheck "win64-unreal-preflight" ($preflightExit -eq 0 -and $preflightResult -eq "PASS") "exit=$preflightExit result=$preflightResult evidence=$PreflightPath"
    }

    $editorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    if ($SkipEditorProbe) {
        $editorProbeResult = "SKIPPED"
        Add-QualificationCheck "editor-nullrhi-project-probe" $true "Skipped by explicit -SkipEditorProbe" $false
    } elseif (-not (Test-Path $editorCmd -PathType Leaf)) {
        $editorProbeResult = "FAIL"
        Add-QualificationCheck "editor-nullrhi-project-probe" $false "UnrealEditor-Cmd.exe missing: $editorCmd"
    } elseif (-not (Test-Path $ProjectFile -PathType Leaf)) {
        $editorProbeResult = "FAIL"
        Add-QualificationCheck "editor-nullrhi-project-probe" $false "Project missing: $ProjectFile"
    } else {
        if (Test-Path $ProbeLog) { Remove-Item -Force $ProbeLog }
        $args = @(
            $ProjectFile,
            "-unattended",
            "-nop4",
            "-nosplash",
            "-NullRHI",
            "-NoSound",
            "-ExecCmds=quit",
            "-log=$ProbeLog"
        )
        try {
            $process = Start-Process -FilePath $editorCmd -ArgumentList $args -PassThru -WindowStyle Hidden
            $finished = $process.WaitForExit($EditorProbeTimeoutSeconds * 1000)
            if (-not $finished) {
                try { $process.Kill($true) } catch { try { $process.Kill() } catch { } }
                $editorProbeResult = "TIMEOUT"
                Add-QualificationCheck "editor-nullrhi-project-probe" $false "Timed out after $EditorProbeTimeoutSeconds seconds; log=$ProbeLog"
            } else {
                $exitCode = $process.ExitCode
                $probeText = if (Test-Path $ProbeLog -PathType Leaf) { Get-Content -Raw $ProbeLog } else { "" }
                $fatal = $probeText -match '(?im)Fatal error:|Assertion failed:|Unhandled Exception:'
                $editorProbeResult = if ($exitCode -eq 0 -and -not $fatal) { "PASS" } else { "FAIL" }
                Add-QualificationCheck "editor-nullrhi-project-probe" ($editorProbeResult -eq "PASS") "exit=$exitCode fatal=$fatal log=$ProbeLog"
            }
        } catch {
            $editorProbeResult = "FAIL"
            Add-QualificationCheck "editor-nullrhi-project-probe" $false "Launch failed: $($_.Exception.Message)"
        }
    }

    $result = if ($requiredFailure) { "FAIL" } else { "PASS" }
    Write-QualificationReport -Result $result -GitSha $gitSha -Version $projectVersion -EngineVersion $engineVersion -PreflightResult $preflightResult -EditorProbeResult $editorProbeResult

    Write-Host "[GTT][RUNNER] Win64 UE 5.8 runner qualification: $result"
    Write-Host "[GTT][RUNNER] Evidence: $OutputPath"
    foreach ($check in $checks) {
        $mark = if ($check.passed) { "PASS" } elseif ($check.required) { "FAIL" } else { "INFO" }
        Write-Host "[GTT][$mark] $($check.name): $($check.detail)"
    }

    if ($requiredFailure) { exit 2 }
    exit 0
} catch {
    Add-QualificationCheck "qualification-unhandled-error" $false $_.Exception.Message
    Write-QualificationReport -Result "FAIL" -GitSha $gitSha -Version $projectVersion -EngineVersion $engineVersion -PreflightResult $preflightResult -EditorProbeResult $editorProbeResult
    Write-Error "[GTT][RUNNER] Qualification failed: $($_.Exception.Message)"
    exit 2
}
