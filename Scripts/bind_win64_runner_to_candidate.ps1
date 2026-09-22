param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RunnerQualificationPath,
    [Parameter(Mandatory=$true)][string]$Version,
    [ValidateSet("Development", "Shipping")]
    [Parameter(Mandatory=$true)][string]$Configuration,
    [Parameter(Mandatory=$true)][string]$ExpectedGitSha,
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RunnerQualificationPath = [IO.Path]::GetFullPath($RunnerQualificationPath)
if ($ExpectedGitSha -notmatch '^[0-9a-fA-F]{40}$') { throw "ExpectedGitSha must be an exact 40-character Git SHA." }
$ExpectedGitSha = $ExpectedGitSha.ToLowerInvariant()

$ZipPath = "$PackageDirectory.zip"
$ZipHashPath = "$ZipPath.sha256"
$ArchiveVerificationPath = "$ZipPath.verify.json"
$RunnerPreflightPath = "$RunnerQualificationPath.preflight.json"
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = "$ZipPath.runner-bind.json"
}
$OutputPath = [IO.Path]::GetFullPath($OutputPath)

function Read-JsonRequired {
    param([Parameter(Mandatory=$true)][string]$Path, [Parameter(Mandatory=$true)][string]$Name)
    if (-not (Test-Path $Path -PathType Leaf)) { throw "Required evidence missing: $Name ($Path)" }
    try { return (Get-Content -Raw $Path | ConvertFrom-Json) }
    catch { throw "Invalid JSON in $Name: $($_.Exception.Message)" }
}

function Get-RequiredCheck {
    param([Parameter(Mandatory=$true)][object]$Document, [Parameter(Mandatory=$true)][string]$Name, [Parameter(Mandatory=$true)][string]$Scope)
    $matches = @($Document.checks | Where-Object { [string]$_.name -eq $Name })
    if ($matches.Count -ne 1) { throw "$Scope must contain exactly one '$Name' check; found $($matches.Count)." }
    $check = $matches[0]
    if (-not [bool]$check.required -or -not [bool]$check.passed) {
        throw "$Scope required check '$Name' did not PASS."
    }
    return $check
}

foreach ($path in @($ZipPath, $ZipHashPath, $ArchiveVerificationPath, $RunnerQualificationPath, $RunnerPreflightPath)) {
    if (-not (Test-Path $path -PathType Leaf)) { throw "Current-gate binding input missing: $path" }
}

$qualification = Read-JsonRequired -Path $RunnerQualificationPath -Name "WIN64_RUNNER_QUALIFICATION.json"
if ([string]$qualification.schema -ne "gtt.win64-runner-qualification.v1" -or [string]$qualification.result -ne "PASS") {
    throw "Runner qualification is not PASS schema gtt.win64-runner-qualification.v1."
}
if ([string]$qualification.git_sha -ne $ExpectedGitSha) { throw "Runner qualification Git SHA does not match exact candidate." }
if ([string]$qualification.version -ne $Version) { throw "Runner qualification version does not match exact candidate." }
if ([string]$qualification.platform -ne "Win64") { throw "Runner qualification platform must be Win64." }
if ([string]$qualification.engine -ne "Unreal Engine 5.8") { throw "Runner qualification engine must be Unreal Engine 5.8." }
if ([string]$qualification.detected_engine_version -notmatch '^5\.8(?:\.|$)') { throw "Runner qualification did not detect Unreal Engine 5.8.x." }
if ([string]$qualification.preflight -ne "PASS" -or [string]$qualification.editor_probe -ne "PASS") {
    throw "Runner qualification must contain PASS preflight and editor probe evidence."
}
if ([string]$qualification.human_visual_review -ne "REQUIRED" -or [bool]$qualification.demo_release_authorized) {
    throw "Runner qualification crossed the human visual review / Demo Release boundary."
}
if ([string]::IsNullOrWhiteSpace([string]$qualification.host.machine)) { throw "Runner qualification host machine identity is missing." }

foreach ($name in @(
    "x64-host-process",
    "github-runner-arch-label",
    "exact-git-sha",
    "expected-git-sha",
    "expected-project-version",
    "clean-tracked-tree",
    "git-lfs-fsck",
    "win64-unreal-preflight",
    "editor-nullrhi-project-probe"
)) {
    [void](Get-RequiredCheck -Document $qualification -Name $name -Scope "runner qualification")
}

$preflight = Read-JsonRequired -Path $RunnerPreflightPath -Name "runner preflight"
if ([int]$preflight.evidence_schema -ne 2 -or [string]$preflight.gate -ne "GTT_WIN64_UNREAL_PREFLIGHT" -or [string]$preflight.result -ne "PASS") {
    throw "Runner preflight is not PASS evidence schema 2."
}
if ([string]$preflight.git_sha -ne $ExpectedGitSha) { throw "Runner preflight Git SHA does not match exact candidate." }
if ([string]$preflight.unreal.detected_version -notmatch '^5\.8(?:\.|$)') { throw "Runner preflight did not detect Unreal Engine 5.8.x." }
if ([string]$preflight.unreal.project_engine_association -ne "5.8") { throw "Runner preflight project EngineAssociation must be 5.8." }

$preflightChecks = @{}
foreach ($name in @(
    "windows-host",
    "run-uat",
    "unreal-editor-cmd",
    "unreal-build-tool",
    "engine-version-5.8",
    "project-engine-association",
    "chaos-vehicles-plugin",
    "gtt-runtime-module",
    "git-lfs",
    "msvc-toolchain",
    "windows-sdk",
    "free-disk"
)) {
    $check = Get-RequiredCheck -Document $preflight -Name $name -Scope "runner preflight"
    $preflightChecks[$name] = $check
}

$archiveVerification = Read-JsonRequired -Path $ArchiveVerificationPath -Name "candidate archive verification"
if ([string]$archiveVerification.schema -ne "gtt.win64-candidate-archive-verification.v1" -or [string]$archiveVerification.result -ne "PASS") {
    throw "Candidate archive verification is not PASS schema v1."
}
if ([string]$archiveVerification.git_sha -ne $ExpectedGitSha -or [string]$archiveVerification.version -ne $Version -or [string]$archiveVerification.configuration -ne $Configuration) {
    throw "Candidate archive verification identity does not match exact candidate."
}
if ([string]$archiveVerification.platform -ne "Win64" -or [string]$archiveVerification.engine -ne "Unreal Engine 5.8") {
    throw "Candidate archive verification platform/engine mismatch."
}
if ([string]$archiveVerification.human_visual_review -ne "REQUIRED" -or [bool]$archiveVerification.demo_release_authorized) {
    throw "Candidate archive verification crossed the human visual review / Demo Release boundary."
}

$sidecar = (Get-Content -Raw $ZipHashPath).Trim()
$sidecarMatch = [regex]::Match($sidecar, '^(?<hash>[0-9a-f]{64})  (?<name>[^/\\]+\.zip)$')
if (-not $sidecarMatch.Success) { throw "Candidate ZIP SHA256 sidecar has invalid canonical format." }
if ($sidecarMatch.Groups['name'].Value -ne [IO.Path]::GetFileName($ZipPath)) { throw "Candidate ZIP SHA256 sidecar names a different archive." }
$archiveSha256 = (Get-FileHash -Algorithm SHA256 -Path $ZipPath).Hash.ToLowerInvariant()
if ($archiveSha256 -ne $sidecarMatch.Groups['hash'].Value) { throw "Candidate ZIP SHA256 sidecar does not match archive bytes." }
if ([string]$archiveVerification.archive_sha256 -ne $archiveSha256) { throw "Candidate archive verification hash does not match sealed ZIP bytes." }

$qualificationSha256 = (Get-FileHash -Algorithm SHA256 -Path $RunnerQualificationPath).Hash.ToLowerInvariant()
$preflightSha256 = (Get-FileHash -Algorithm SHA256 -Path $RunnerPreflightPath).Hash.ToLowerInvariant()
$archiveVerificationSha256 = (Get-FileHash -Algorithm SHA256 -Path $ArchiveVerificationPath).Hash.ToLowerInvariant()

$binding = [ordered]@{
    schema = "gtt.win64-runner-candidate-binding.v1"
    result = "PASS"
    game = "Grand Theft Tractor"
    git_sha = $ExpectedGitSha
    version = $Version
    configuration = $Configuration
    platform = "Win64"
    engine = "Unreal Engine 5.8"
    runner = [ordered]@{
        machine = [string]$qualification.host.machine
        os = [string]$qualification.host.os
        powershell = [string]$qualification.host.powershell
        github_runner_name = [string]$env:RUNNER_NAME
        github_runner_os = [string]$env:RUNNER_OS
        github_runner_arch = [string]$env:RUNNER_ARCH
        detected_engine_version = [string]$qualification.detected_engine_version
    }
    toolchain = [ordered]@{
        msvc = [string]$preflightChecks["msvc-toolchain"].detail
        windows_sdk = [string]$preflightChecks["windows-sdk"].detail
        unreal_build_tool = [string]$preflightChecks["unreal-build-tool"].detail
    }
    qualification = [ordered]@{
        file = [IO.Path]::GetFileName($RunnerQualificationPath)
        sha256 = $qualificationSha256
        preflight_file = [IO.Path]::GetFileName($RunnerPreflightPath)
        preflight_sha256 = $preflightSha256
        preflight = "PASS"
        editor_probe = "PASS"
    }
    candidate_archive = [ordered]@{
        file = [IO.Path]::GetFileName($ZipPath)
        sha256 = $archiveSha256
        verification_file = [IO.Path]::GetFileName($ArchiveVerificationPath)
        verification_sha256 = $archiveVerificationSha256
    }
    github_actions = [ordered]@{
        workflow = [string]$env:GITHUB_WORKFLOW
        run_id = [string]$env:GITHUB_RUN_ID
        run_attempt = [string]$env:GITHUB_RUN_ATTEMPT
        job = [string]$env:GITHUB_JOB
    }
    human_visual_review = "REQUIRED"
    demo_release_authorized = $false
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
}

$outputParent = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outputParent)) { New-Item -ItemType Directory -Force -Path $outputParent | Out-Null }
$binding | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath

$roundTrip = Read-JsonRequired -Path $OutputPath -Name "runner/candidate binding"
if ([string]$roundTrip.schema -ne "gtt.win64-runner-candidate-binding.v1" -or [string]$roundTrip.result -ne "PASS" -or
    [string]$roundTrip.git_sha -ne $ExpectedGitSha -or [string]$roundTrip.version -ne $Version -or
    [string]$roundTrip.configuration -ne $Configuration -or [string]$roundTrip.qualification.sha256 -ne $qualificationSha256 -or
    [string]$roundTrip.qualification.preflight_sha256 -ne $preflightSha256 -or [string]$roundTrip.candidate_archive.sha256 -ne $archiveSha256 -or
    [string]$roundTrip.human_visual_review -ne "REQUIRED" -or [bool]$roundTrip.demo_release_authorized) {
    throw "Runner/candidate binding failed round-trip exact identity/hash/release-boundary validation."
}

Write-Host "[GTT][RUNNER-BIND] PASS: exact UE 5.8 runner/toolchain evidence is cryptographically bound to sealed candidate archive $archiveSha256."
Write-Host "[GTT][RUNNER-BIND] Binding: $OutputPath"
