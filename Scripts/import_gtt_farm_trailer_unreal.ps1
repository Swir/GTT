param(
    [Parameter(Mandatory=$true)]
    [string]$UnrealEditorCmd,
    [string]$Project = (Join-Path $PSScriptRoot "..\GTT.uproject")
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Project = (Resolve-Path $Project).Path
$UnrealEditorCmd = (Resolve-Path $UnrealEditorCmd).Path
$OutDir = Join-Path $Root "Intermediate\GTT\AuthoredTrailer"
$Rig = Join-Path $OutDir "GTT_FarmTrailer_Rig.gltf"
$Py = Join-Path $Root "Scripts\Unreal\import_gtt_farm_trailer.py"
$Log = Join-Path $OutDir "AuthoredTrailerImport.log"
$Evidence = Join-Path $OutDir "AUTHORED_TRAILER_EDITOR_ACCEPTANCE.json"

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
if (Test-Path $Evidence) { Remove-Item -Force $Evidence }

$Python = Get-Command python -ErrorAction Stop
& $Python.Source (Join-Path $Root "Scripts\generate_gtt_farm_trailer_gltf.py") --output $Rig
if ($LASTEXITCODE -ne 0) { throw "Trailer glTF generation failed." }

& $Python.Source (Join-Path $Root "Scripts\verify_v0_1_60_authored_trailer_source_rig.py") --asset $Rig
if ($LASTEXITCODE -ne 0) { throw "Trailer glTF source verification failed." }

if (-not (Test-Path $Py)) { throw "Missing Unreal import script: $Py" }

$Args = @(
    $Project,
    "-unattended",
    "-nop4",
    "-nosplash",
    "-NullRHI",
    "-run=pythonscript",
    "-script=`"$Py`"",
    "-log=`"$Log`""
)
& $UnrealEditorCmd @Args
$ExitCode = $LASTEXITCODE

if (-not (Test-Path $Log)) { throw "Unreal import log was not produced: $Log" }
$LogText = Get-Content -Raw -Path $Log
if ($ExitCode -ne 0) { throw "UnrealEditor-Cmd failed with exit code $ExitCode. See $Log" }
if ($LogText -notmatch "AUTHORED_TRAILER_IMPORT result=PASS") {
    throw "Unreal import did not emit the PASS marker. See $Log"
}
if ($LogText -match "AUTHORED_TRAILER_IMPORT result=FAIL") {
    throw "Unreal import emitted a FAIL marker. See $Log"
}
if (-not (Test-Path $Evidence -PathType Leaf)) {
    throw "Unreal import did not produce machine-readable editor acceptance evidence: $Evidence"
}

$EditorEvidence = Get-Content -Raw -Path $Evidence | ConvertFrom-Json
if ($EditorEvidence.schema -ne "gtt.authored-trailer-editor-acceptance.v1" -or $EditorEvidence.result -ne "PASS") {
    throw "Authored trailer editor evidence schema/result mismatch."
}

$ExpectedGitSha = [string]$env:GITHUB_SHA
if (-not [string]::IsNullOrWhiteSpace($ExpectedGitSha)) {
    $ExpectedGitSha = $ExpectedGitSha.Trim().ToLowerInvariant()
    if ($ExpectedGitSha -notmatch '^[0-9a-f]{40}$') {
        throw "GITHUB_SHA must be an exact 40-character lowercase/uppercase hex SHA when provided."
    }
    if ([string]$EditorEvidence.git_sha -ne $ExpectedGitSha) {
        throw "Authored trailer editor evidence is not bound to expected GITHUB_SHA $ExpectedGitSha."
    }
}

$RigHash = (Get-FileHash -Algorithm SHA256 -Path $Rig).Hash.ToLowerInvariant()
if ([string]$EditorEvidence.source_gltf_sha256 -ne $RigHash) {
    throw "Authored trailer editor evidence source glTF hash mismatch."
}

$ExpectedBones = @("body", "wheel_l", "wheel_r") | Sort-Object
$RequiredBones = @($EditorEvidence.required_bones) | ForEach-Object { [string]$_ } | Sort-Object
$VerifiedBones = @($EditorEvidence.verified_bones) | ForEach-Object { [string]$_ } | Sort-Object
if (($RequiredBones -join ",") -ne ($ExpectedBones -join ",") -or
    ($VerifiedBones -join ",") -ne ($ExpectedBones -join ",")) {
    throw "Authored trailer editor evidence bone set mismatch."
}

$ExpectedSockets = @("socket_hitch", "socket_cargo", "socket_axle_l", "socket_axle_r") | Sort-Object
$RequiredSockets = @($EditorEvidence.required_sockets) | ForEach-Object { [string]$_ } | Sort-Object
$VerifiedSockets = @($EditorEvidence.verified_sockets) | ForEach-Object { [string]$_ } | Sort-Object
if (($RequiredSockets -join ",") -ne ($ExpectedSockets -join ",") -or
    ($VerifiedSockets -join ",") -ne ($ExpectedSockets -join ",")) {
    throw "Authored trailer editor evidence socket set mismatch."
}

foreach ($PropertyName in @(
    "skeletal_mesh_object_path",
    "skeleton_object_path",
    "physics_asset_object_path"
)) {
    if ([string]::IsNullOrWhiteSpace([string]$EditorEvidence.$PropertyName)) {
        throw "Authored trailer editor evidence is missing $PropertyName."
    }
}

Write-Host "GTT 0.1.60 authored trailer UE import: PASS"
Write-Host "Log: $Log"
Write-Host "Editor acceptance evidence: $Evidence"
Write-Host "This proves editor import/socket/PhysicsAsset setup only; packaged Win64 runtime + visual acceptance remain open."
