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
    $m=[regex]::Match($log,"(?m)^.*WORKSHOP_HOURS_RUNTIME phase=$Phase result=(PASS|FAIL).*$")
    if(-not $m.Success){throw "Workshop-hours runtime marker missing: $Phase"}
    if($m.Groups[1].Value -ne 'PASS'){throw "Workshop-hours runtime phase failed: $Phase"}
    return $m.Value
}
function Field([string]$Line,[string]$Name){
    $m=[regex]::Match($Line,"(?:^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if(-not $m.Success){throw "Field '$Name' missing from marker: $Line"}
    return $m.Groups[1].Value
}
function IntField([string]$Line,[string]$Name){return [int](Field $Line $Name)}

$begin=[regex]::Match($log,'(?m)^.*WORKSHOP_HOURS_RUNTIME_BEGIN version=1 route=clock-closed-reject-tow-hold-emergency-service .*opening=06:30 closing=20:00 emergency_surcharge_percent=35 exact_vehicle=required.*$')
if(-not $begin.Success){throw 'Workshop-hours runtime begin marker missing or wrong route/schema.'}
$prepare=Marker 'PREPARE'
$boundaries=Marker 'BOUNDARIES'
$closed=Marker 'CLOSED_ORDINARY'
$towRequest=Marker 'TOW_REQUEST'
$towComplete=Marker 'TOW_COMPLETE'
$quote=Marker 'EMERGENCY_QUOTE'
$service=Marker 'EMERGENCY_SERVICE'
$completeMatch=[regex]::Match($log,'(?m)^.*WORKSHOP_HOURS_RUNTIME_COMPLETE result=(PASS|FAIL).*$')
if(-not $completeMatch.Success){throw 'Workshop-hours runtime completion marker missing.'}
if($completeMatch.Groups[1].Value -ne 'PASS'){throw 'Workshop-hours runtime completion did not PASS.'}
$complete=$completeMatch.Value

foreach($field in @('boundaries','closed_rejected','closed_no_charge','closed_no_mutation','tow_requested','tow_completed','tow_single_charge','hold_detected','emergency_quote','emergency_single_charge','emergency_service','hold_cleared','identity_preserved','repaired','refuelled')){
    if((IntField $complete $field) -ne 1){throw "Workshop-hours completion gate '$field' is not PASS."}
}
foreach($field in @('pre_open_closed','opening_open','last_minute_open','closing_closed')){
    if((IntField $boundaries $field) -ne 1){throw "Workshop clock boundary '$field' is not PASS."}
}
if([double](Field $boundaries 'opening') -ne 6.5 -or [double](Field $boundaries 'closing') -ne 20.0){throw 'Workshop runtime boundaries do not match 06:30-20:00.'}
foreach($field in @('rejected','no_charge','no_mutation')){if((IntField $closed $field) -ne 1){throw "Closed ordinary service gate '$field' failed."}}
if((IntField $closed 'hold_before') -ne 0){throw 'Closed ordinary service was tested with a hard workshop hold; expected ordinary non-hold rejection.'}
if((IntField $towRequest 'requested') -ne 1 -or (IntField $towRequest 'target_pinned') -ne 1 -or (IntField $towRequest 'no_precharge') -ne 1){throw 'Tow request did not prove pinned exact target with no pre-charge.'}

$towQuote=IntField $complete 'tow_quote'
$baseQuote=IntField $complete 'base_quote'
$emergencyQuote=IntField $complete 'emergency_quote_total'
$surcharge=IntField $complete 'surcharge_percent'
$vehicle=Field $complete 'vehicle'
if($towQuote -le 0 -or $baseQuote -le 0 -or $emergencyQuote -le $baseQuote){throw 'Workshop-hours runtime did not prove positive tow/base/emergency quotes.'}
if($surcharge -ne 35){throw "Emergency surcharge mismatch: $surcharge"}
$expectedEmergency=$baseQuote + [int][math]::Ceiling($baseQuote * 0.35)
if($emergencyQuote -ne $expectedEmergency){throw "Emergency quote mismatch: observed=$emergencyQuote expected=$expectedEmergency"}
if([string]::IsNullOrWhiteSpace($vehicle) -or $vehicle -eq 'None'){throw 'Workshop-hours runtime did not report a stable vehicle id.'}
if((IntField $towComplete 'single_charge') -ne 1 -or (IntField $towComplete 'charged') -ne $towQuote){throw 'Tow was not charged exactly once at the locked quote.'}
if((IntField $towComplete 'hold_detected') -ne 1 -or (IntField $towComplete 'identity_preserved') -ne 1){throw 'Tow did not produce workshop hold on the same vehicle.'}
if((IntField $quote 'closed') -ne 1 -or (IntField $quote 'hold') -ne 1 -or (IntField $quote 'exact') -ne 1){throw 'Emergency checkout quote did not prove closed+hold exact calculation.'}
if((IntField $quote 'base_quote') -ne $baseQuote -or (IntField $quote 'checkout_quote') -ne $emergencyQuote -or (IntField $quote 'expected_quote') -ne $emergencyQuote){throw 'Emergency quote marker is inconsistent with completion evidence.'}
if((IntField $service 'charged') -ne $emergencyQuote -or (IntField $service 'checkout_quote') -ne $emergencyQuote -or (IntField $service 'single_charge') -ne 1){throw 'Emergency service was not charged exactly once at checkout quote.'}
foreach($field in @('hold_cleared','identity_preserved','repaired','refuelled','closed')){if((IntField $service $field) -ne 1){throw "Emergency service gate '$field' failed."}}

$escapedVehicle=[regex]::Escape($vehicle)
$nativeTow=[regex]::Matches($log,"(?m)^.*NATIVE_ROADSIDE_TOW_COMPLETE vehicle=$escapedVehicle .*quote_locked=YES target_pinned=YES .*damage_preserved=YES identity_preserved=YES serviced=NO destination=WORKSHOP.*$").Count
if($nativeTow -lt 1){throw 'Production roadside tow completion marker is missing for workshop-hours evidence vehicle.'}
$diagnosticFailures=[regex]::Matches($log,'(?m)^.*WORKSHOP_HOURS_RUNTIME phase=DIAGNOSTIC result=FAIL.*$').Count
if($diagnosticFailures -ne 0){throw "Workshop-hours route reported $diagnosticFailures diagnostic failures."}

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
$gitSha=$ExpectedGitSha
if(Test-Path $buildPath){
    $build=Get-Content -Raw $buildPath|ConvertFrom-Json
    if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
    $gitSha=$build.git_sha
}

$evidence=[ordered]@{
    schema='gtt.workshop-hours-runtime.v1'
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$gitSha
    route='clock-closed-reject-tow-hold-emergency-service'
    stable_vehicle_id=$vehicle
    opening_hour=6.5
    closing_hour=20.0
    boundary_contract=$true
    ordinary_after_hours_rejected=$true
    ordinary_after_hours_no_charge=$true
    ordinary_after_hours_no_mutation=$true
    tow_locked_quote=$towQuote
    tow_single_charge=$true
    workshop_hold_detected=$true
    emergency_base_quote=$baseQuote
    emergency_surcharge_percent=$surcharge
    emergency_checkout_quote=$emergencyQuote
    emergency_quote_exact=$true
    emergency_single_charge=$true
    emergency_service_applied=$true
    workshop_hold_cleared=$true
    exact_vehicle_preserved=$true
    mechanical_repaired=$true
    refuelled=$true
    native_tow_complete_markers=$nativeTow
    diagnostic_failure_count=$diagnosticFailures
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$out=Join-Path $PackageDirectory 'WORKSHOP_HOURS_RUNTIME.json'
$evidence|ConvertTo-Json -Depth 5|Set-Content -Encoding UTF8 $out
Write-Host '[GTT] Workshop hours + after-hours emergency runtime evidence: PASS'
Write-Host "[GTT] Vehicle: $vehicle | base: $baseQuote | emergency: $emergencyQuote | surcharge: $surcharge%"
Write-Host "[GTT] Evidence: $out"
