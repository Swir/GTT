param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha=$env:GITHUB_SHA
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)
if(-not(Test-Path $PackageDirectory -PathType Container)){throw "Package directory does not exist: $PackageDirectory"}
if(-not(Test-Path $RuntimeLog -PathType Leaf)){throw "Runtime log does not exist: $RuntimeLog"}

$log=Get-Content -Raw $RuntimeLog
function Marker([string]$Phase){
    $m=[regex]::Match($log,"(?m)^.*FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=$Phase result=(PASS|FAIL).*$")
    if(-not $m.Success){throw "Workshop recovery runtime marker missing: $Phase"}
    if($m.Groups[1].Value -ne 'PASS'){throw "Workshop recovery runtime phase failed: $Phase"}
    return $m.Value
}
function Field([string]$Line,[string]$Name){
    $m=[regex]::Match($Line,"(?:^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if(-not $m.Success){throw "Field '$Name' missing from marker: $Line"}
    return $m.Groups[1].Value
}
function IntField([string]$Line,[string]$Name){return [int](Field $Line $Name)}

$begin=[regex]::Match($log,'(?m)^.*FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_BEGIN version=1 route=feed-tow-workshop-hold-service-hill-wood.*$')
if(-not $begin.Success){throw 'Workshop recovery runtime begin marker missing or wrong route/schema.'}
$prepare=Marker 'PREPARE'
$accept=Marker 'ACCEPT'
$pickup=Marker 'PICKUP'
$towRequest=Marker 'TOW_REQUEST'
$towComplete=Marker 'TOW_COMPLETE'
$hold=Marker 'WORKSHOP_HOLD'
$garageReject=Marker 'GARAGE_RECALL_REJECT'
$service=Marker 'WORKSHOP_SERVICE'
$wrongVehicle=Marker 'WRONG_VEHICLE'
$hill=Marker 'HILL_HANDOFF'
$final=Marker 'FINAL_HANDOFF'
$persistence=Marker 'PERSISTENCE'
$completeMatch=[regex]::Match($log,'(?m)^.*FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_COMPLETE result=(PASS|FAIL).*$')
if(-not $completeMatch.Success){throw 'Workshop recovery runtime completion marker missing.'}
if($completeMatch.Groups[1].Value -ne 'PASS'){throw 'Workshop recovery runtime completion marker did not PASS.'}
$complete=$completeMatch.Value

$requiredOne=@(
    'accepted','pickup','tow_requested','tow_completed','tow_single_charge','tow_damage_preserved',
    'tow_identity_preserved','workshop_destination','hold_detected','garage_recall_blocked','garage_no_charge',
    'garage_no_move','workshop_service','workshop_single_charge','hold_cleared','repaired','refuelled',
    'exact_vehicle','timer_continued','integrity_not_improved','wrong_vehicle_rejected','hill','final','save','authority_cleared'
)
foreach($name in $requiredOne){if((IntField $complete $name) -ne 1){throw "Workshop recovery completion gate '$name' is not PASS."}}

$towQuote=IntField $complete 'tow_quote'
$workshopQuote=IntField $complete 'workshop_quote'
$payoutDelta=IntField $complete 'payout_delta'
$cargoRunsDelta=IntField $complete 'cargo_runs_delta'
$reputationDelta=IntField $complete 'reputation_delta'
$vehicle=Field $complete 'vehicle'
if($towQuote -le 0 -or $workshopQuote -le 0){throw 'Workshop recovery runtime did not prove positive tow and workshop quotes.'}
if($payoutDelta -le 0 -or $cargoRunsDelta -ne 1 -or $reputationDelta -le 0){throw 'Workshop recovery runtime did not prove one authoritative route completion.'}
if([string]::IsNullOrWhiteSpace($vehicle) -or $vehicle -eq 'None'){throw 'Workshop recovery runtime did not report a stable vehicle id.'}

if((IntField $towComplete 'single_charge') -ne 1 -or (IntField $towComplete 'charged') -ne $towQuote){throw 'Roadside tow was not charged exactly once at the locked quote.'}
foreach($field in @('damage_preserved','identity_preserved','workshop_destination')){if((IntField $towComplete $field) -ne 1){throw "Tow completion missing PASS gate: $field"}}
if((IntField $hold 'hold') -ne 1 -or (IntField $hold 'hold_count') -lt 1){throw 'Authoritative garage workshop hold was not observed after tow.'}
foreach($field in @('blocked','no_charge','no_move')){if((IntField $garageReject $field) -ne 1){throw "Garage recall bypass was not blocked safely: $field"}}
if((IntField $service 'single_charge') -ne 1 -or (IntField $service 'charged') -ne $workshopQuote){throw 'Workshop service was not charged exactly once at the authoritative quote.'}
foreach($field in @('service','hold_cleared','repaired','refuelled','exact_vehicle','timer_continued','integrity_not_improved')){
    if((IntField $service $field) -ne 1){throw "Workshop service missing PASS gate: $field"}
}
if((IntField $wrongVehicle 'rejected') -ne 1){throw 'Wrong vehicle was not rejected after workshop service.'}
if((IntField $persistence 'explicit_save') -ne 1){throw 'Final primary save was not proven.'}

$escapedVehicle=[regex]::Escape($vehicle)
$nativeTow=[regex]::Matches($log,"(?m)^.*NATIVE_ROADSIDE_TOW_COMPLETE vehicle=$escapedVehicle .*quote_locked=YES target_pinned=YES .*damage_preserved=YES identity_preserved=YES serviced=NO destination=WORKSHOP.*$").Count
if($nativeTow -lt 1){throw 'Production roadside tow completion marker with preserved damage/identity and workshop destination is missing.'}
$diagnosticFailures=[regex]::Matches($log,'(?m)^.*FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=DIAGNOSTIC result=FAIL.*$').Count
if($diagnosticFailures -ne 0){throw "Workshop recovery route reported $diagnosticFailures diagnostic failures."}

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
$gitSha=$ExpectedGitSha
if(Test-Path $buildPath){
    $build=Get-Content -Raw $buildPath|ConvertFrom-Json
    if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
    $gitSha=$build.git_sha
}

$evidence=[ordered]@{
    schema='gtt.farm-cargo-workshop-recovery-runtime.v1'
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$gitSha
    route='feed-tow-workshop-hold-service-hill-wood'
    stable_vehicle_id=$vehicle
    tow_locked_quote=$towQuote
    tow_completed=$true
    tow_single_charge=$true
    tow_damage_preserved=$true
    tow_identity_preserved=$true
    workshop_destination=$true
    workshop_hold_detected=$true
    garage_recall_blocked=$true
    garage_recall_no_charge=$true
    garage_recall_no_move=$true
    workshop_quote=$workshopQuote
    workshop_service_applied=$true
    workshop_single_charge=$true
    workshop_hold_cleared=$true
    mechanical_repaired=$true
    refuelled=$true
    exact_vehicle_preserved=$true
    cargo_timer_continued=$true
    cargo_integrity_not_improved=$true
    wrong_vehicle_rejected=$true
    hill_handoff=$true
    final_handoff=$true
    final_save=$true
    authority_cleared=$true
    payout_delta=$payoutDelta
    cargo_completed_runs_delta=$cargoRunsDelta
    logistics_reputation_delta=$reputationDelta
    native_tow_complete_markers=$nativeTow
    diagnostic_failure_count=$diagnosticFailures
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$out=Join-Path $PackageDirectory 'FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json'
$evidence|ConvertTo-Json -Depth 5|Set-Content -Encoding UTF8 $out
Write-Host '[GTT] Farm Cargo garage/workshop recovery runtime evidence: PASS'
Write-Host "[GTT] Stable vehicle: $vehicle | tow quote: $towQuote | workshop quote: $workshopQuote"
Write-Host "[GTT] Evidence: $out"
