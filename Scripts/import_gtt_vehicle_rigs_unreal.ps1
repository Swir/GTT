param(
    [Parameter(Mandatory=$true)][string]$UnrealEditorCmd,
    [Parameter(Mandatory=$true)][string]$Project
)
$ErrorActionPreference='Stop'; Set-StrictMode -Version Latest
$Root=Split-Path -Parent $PSScriptRoot
$Generated=Join-Path $Root 'Intermediate\GTT\NativeVehicles'
New-Item -ItemType Directory -Force -Path $Generated | Out-Null
& python (Join-Path $Root 'Scripts\generate_gtt_vehicle_rigs.py') --output-dir $Generated
if($LASTEXITCODE -ne 0){throw 'Native vehicle source rig generation failed.'}
$Script=(Join-Path $Root 'Scripts\Unreal\import_gtt_vehicle_rigs.py') -replace '\\','/'
& $UnrealEditorCmd $Project '-Unattended' '-NoSplash' '-NullRHI' "-ExecutePythonScript=`"$Script`""
if($LASTEXITCODE -ne 0){throw "Native vehicle Unreal import failed with exit code $LASTEXITCODE"}
$Evidence=Join-Path $Generated 'NATIVE_VEHICLE_RIG_EDITOR_ACCEPTANCE.json'
if(-not(Test-Path $Evidence -PathType Leaf)){throw "Native vehicle import evidence missing: $Evidence"}
$Result=Get-Content -Raw $Evidence | ConvertFrom-Json
if($Result.schema -ne 'gtt.native-vehicle-rig-editor-acceptance.v1' -or $Result.result -ne 'PASS' -or $Result.assets.Count -ne 3){throw 'Native vehicle import evidence is invalid.'}
Write-Host '[GTT][NATIVE-RIG] PASS: three Native Chaos vehicle rigs imported and validated.'
