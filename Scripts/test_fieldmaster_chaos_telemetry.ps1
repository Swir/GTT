param()

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Evaluator=Join-Path $PSScriptRoot 'evaluate_fieldmaster_chaos_telemetry.ps1'
if(-not(Test-Path $Evaluator)){throw "Evaluator missing: $Evaluator"}
$Temp=Join-Path ([IO.Path]::GetTempPath()) ("gtt-fieldmaster-telemetry-"+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $Temp|Out-Null
try{
    [ordered]@{game='Grand Theft Tractor';version='0.1.15';configuration='Shipping';platform='Win64';engine='UE_5.8';git_sha='fixture-sha'}|ConvertTo-Json|Set-Content -Encoding UTF8 (Join-Path $Temp 'BUILD_INFO.json')
    $good=@(
        '[2026.09.17] LogTemp: Display: FIELDMASTER_CHAOS_TELEMETRY result=OBSERVED config_valid=1 active=1 gear=0 target_gear=1 rpm=780.0 max_rpm=2350.0 speed_kmh=0.05 requested_throttle=0.720 throttle=0.720 steering=0.350 drive_health=1.000 steering_grip=1.000 terrain_grip=1.000 valid_wheels=4 contacts=4 suspension_samples=4 slipping=0 skidding=0 suspension_min=0.421 suspension_max=0.511 spring_force=35210.0 max_slip=4.200 drive_torque=0.0 brake_torque=0.0',
        '[2026.09.17] LogTemp: Display: FIELDMASTER_CHAOS_TELEMETRY result=OBSERVED config_valid=1 active=1 gear=1 target_gear=1 rpm=1320.0 max_rpm=2350.0 speed_kmh=4.80 requested_throttle=0.720 throttle=0.720 steering=-0.350 drive_health=1.000 steering_grip=0.920 terrain_grip=0.920 valid_wheels=4 contacts=4 suspension_samples=4 slipping=1 skidding=0 suspension_min=0.390 suspension_max=0.548 spring_force=36125.0 max_slip=62.400 drive_torque=428.0 brake_torque=0.0',
        '[2026.09.17] LogTemp: Display: FIELDMASTER_CHAOS_TELEMETRY result=OBSERVED config_valid=1 active=1 gear=1 target_gear=1 rpm=1680.0 max_rpm=2350.0 speed_kmh=9.25 requested_throttle=0.720 throttle=0.680 steering=0.180 drive_health=0.950 steering_grip=0.870 terrain_grip=0.900 valid_wheels=4 contacts=3 suspension_samples=4 slipping=1 skidding=1 suspension_min=0.365 suspension_max=0.571 spring_force=34480.0 max_slip=94.100 drive_torque=391.0 brake_torque=18.0'
    )
    $log=Join-Path $Temp 'GTT_RUNTIME.log';$good|Set-Content -Encoding UTF8 $log
    & $Evaluator -PackageDirectory $Temp -RuntimeLog $log -ExpectedGitSha 'fixture-sha' -MinimumSamples 3
    $manifest=Get-Content -Raw (Join-Path $Temp 'FIELDMASTER_CHAOS_TELEMETRY.json')|ConvertFrom-Json
    if($manifest.schema -ne 'gtt.fieldmaster-chaos-telemetry.v1' -or $manifest.result -ne 'PASS'){throw 'Positive telemetry fixture did not create a PASS v1 manifest.'}
    if([int]$manifest.driven_sample_count -lt 2 -or [int]$manifest.grounded_sample_count -lt 3 -or [int]$manifest.torque_sample_count -lt 2){throw 'Positive telemetry fixture summary is weaker than expected.'}

    $bad=$good -replace 'drive_torque=428.0','drive_torque=0.0' -replace 'drive_torque=391.0','drive_torque=0.0'
    $badLog=Join-Path $Temp 'GTT_RUNTIME_BAD.log';$bad|Set-Content -Encoding UTF8 $badLog
    $pwsh=(Get-Command pwsh -ErrorAction Stop).Source
    & $pwsh -NoProfile -File $Evaluator -PackageDirectory $Temp -RuntimeLog $badLog -ExpectedGitSha 'fixture-sha' -MinimumSamples 3 *> $null
    $negativeExitCode=$LASTEXITCODE
    if($negativeExitCode -eq 0){throw 'Negative telemetry fixture unexpectedly passed without live drive torque.'}

    Write-Host "[OK] Fieldmaster telemetry evaluator accepted valid runtime evidence and rejected zero-drive-torque evidence (negative exit=$negativeExitCode)."
}
finally{
    if(Test-Path $Temp){Remove-Item -Recurse -Force $Temp}
}

# The negative child process is expected to return non-zero. Do not leak that
# expected native exit code to the GitHub Actions step after the assertion passed.
exit 0
