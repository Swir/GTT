param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha=$env:GITHUB_SHA
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
$smokePath=Join-Path $PackageDirectory 'FARM_CARGO_SMOKE.json'
if(-not(Test-Path $smokePath)){throw "Farm Cargo smoke evidence missing: $smokePath"}
if(-not(Test-Path $RuntimeLog)){throw "Farm Cargo runtime log missing: $RuntimeLog"}

$smoke=Get-Content -Raw $smokePath|ConvertFrom-Json
$log=Get-Content -Raw $RuntimeLog
if($smoke.schema -ne 'gtt.farm-cargo-smoke.v1' -or $smoke.result -ne 'PASS'){throw 'Farm Cargo packaged smoke did not PASS the expected v1 schema.'}
if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $smoke.git_sha -and $smoke.git_sha -ne $ExpectedGitSha){throw "Farm Cargo smoke SHA mismatch: smoke=$($smoke.git_sha), expected=$ExpectedGitSha"}
if($log -notmatch 'FARM_CARGO_SCENARIO_BEGIN version=1 mode=exact-loaded-vehicle-authority'){throw 'Farm Cargo scenario begin marker missing.'}

$requiredSteps=@('WORLD','CONTRACT','LOAD_LOCK','WRONG_VEHICLE_REJECT','HILL_HANDOFF','PAYOUT_REPUTATION','SAVE','COMPLETE')
foreach($step in $requiredSteps){
    if($log -notmatch "FARM_CARGO_SCENARIO_STEP step=$step result=PASS") { throw "Farm Cargo scenario step missing: $step" }
}
if($log -notmatch 'FARM_CARGO_SCENARIO_COMPLETE result=PASS[^\r\n]*cash_delta=([0-9]+)[^\r\n]*cargo_runs_delta=([0-9]+)'){throw 'Farm Cargo completion evidence with payout/reputation deltas is missing.'}
$cashDelta=[int]$Matches[1]
$cargoRunsDelta=[int]$Matches[2]
if($cashDelta -le 0 -or $cargoRunsDelta -le 0){throw 'Farm Cargo completion did not prove positive payout and completed-run progression.'}

$loadMatches=[regex]::Matches($log,'FARM_CARGO_AUTHORITY event=LOAD_LOCK result=PASS vehicle_id=([^\s]+)')
if($loadMatches.Count -ne 1){throw "Expected exactly one authoritative cargo load lock, found $($loadMatches.Count)."}
$vehicleId=$loadMatches[0].Groups[1].Value
if([string]::IsNullOrWhiteSpace($vehicleId) -or $vehicleId -eq 'None'){throw 'Farm Cargo load lock did not identify a persistent vehicle.'}

$wrong=[regex]::Match($log,'FARM_CARGO_AUTHORITY event=HANDOFF_REJECT reason=wrong-vehicle-or-loaded-vehicle-away vehicle_id=([^\s]+) distance_cm=([0-9.]+)')
if(-not $wrong.Success){throw 'Farm Cargo runtime did not prove rejection while the exact loaded vehicle was outside the yard.'}
if($wrong.Groups[1].Value -ne $vehicleId){throw 'Wrong-vehicle rejection referenced a different vehicle than the load lock.'}
$wrongDistance=[double]$wrong.Groups[2].Value
if($wrongDistance -le 750.0){throw 'Wrong-vehicle rejection did not exercise the >750 cm exact-vehicle distance gate.'}

$handoffMatches=[regex]::Matches($log,'FARM_CARGO_AUTHORITY event=HANDOFF_ACCEPT result=PASS stop=([^\s]+) vehicle_id=([^\s]+) speed_kmh=([0-9.]+) distance_cm=([0-9.]+) contract_complete=([01])')
if($handoffMatches.Count -lt 1){throw 'Farm Cargo runtime did not record an accepted handoff.'}
$stops=@()
$maxSpeed=0.0
$maxDistance=0.0
$contractComplete=$false
foreach($m in $handoffMatches){
    $stop=$m.Groups[1].Value
    $acceptedVehicle=$m.Groups[2].Value
    $speed=[double]$m.Groups[3].Value
    $distance=[double]$m.Groups[4].Value
    $complete=[int]$m.Groups[5].Value
    if($acceptedVehicle -ne $vehicleId){throw "Accepted handoff used a different vehicle: $acceptedVehicle vs $vehicleId"}
    if($speed -gt 3.001){throw "Accepted handoff exceeded 3.0 km/h: $speed"}
    if($distance -gt 750.01){throw "Accepted handoff exceeded 750 cm yard radius: $distance"}
    $stops += $stop
    $maxSpeed=[Math]::Max($maxSpeed,$speed)
    $maxDistance=[Math]::Max($maxDistance,$distance)
    if($complete -eq 1){$contractComplete=$true}
}
if(-not $contractComplete){throw 'Farm Cargo runtime never recorded a contract-completing exact-vehicle handoff.'}

$evidence=[ordered]@{
    schema='gtt.farm-cargo-runtime.v1'
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$smoke.git_sha
    version=$smoke.version
    locked_vehicle_id=$vehicleId
    wrong_vehicle_rejection_passed=$true
    wrong_vehicle_distance_cm=[Math]::Round($wrongDistance,2)
    accepted_handoff_count=$handoffMatches.Count
    accepted_stops=$stops
    max_accepted_handoff_speed_kmh=[Math]::Round($maxSpeed,3)
    max_accepted_handoff_distance_cm=[Math]::Round($maxDistance,2)
    contract_complete=$true
    cash_delta=$cashDelta
    cargo_runs_delta=$cargoRunsDelta
    save_passed=$true
    runtime_log=[IO.Path]::GetFileName($RuntimeLog)
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$outPath=Join-Path $PackageDirectory 'FARM_CARGO_RUNTIME.json'
$evidence|ConvertTo-Json -Depth 6|Set-Content -Encoding UTF8 $outPath
Write-Host "[GTT] Farm Cargo packaged runtime acceptance: PASS (vehicle=$vehicleId, handoffs=$($handoffMatches.Count), cash_delta=$cashDelta, cargo_runs_delta=$cargoRunsDelta)"
