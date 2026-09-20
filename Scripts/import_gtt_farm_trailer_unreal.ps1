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

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

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

Write-Host "GTT 0.1.60 authored trailer UE import: PASS"
Write-Host "Log: $Log"
Write-Host "This proves editor import/socket/PhysicsAsset setup only; packaged Win64 runtime + visual acceptance remain open."
