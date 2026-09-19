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
    $m=[regex]::Match($log,"(?m)^.*WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=$Phase result=(PASS|FAIL).*$")
    if(-not $m.Success){throw "Workshop priority/pickup runtime marker missing: $Phase"}
    if($m.Groups[1].Value -ne 'PASS'){throw "Workshop priority/pickup runtime phase failed: $Phase"}
    return $m.Value
}
function Field([string]$Line,[string]$Name){
    $m=[regex]::Match($Line,"(?:^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if(-not $m.Success){throw "Field '$Name' missing from marker: $Line"}
    return $m.Groups[1].Value
}
function IntField([string]$Line,[string]$Name){return [int](Field $Line $Name)}
function FloatField([string]$Line,[string]$Name){return [double]::Parse((Field $Line $Name),[Globalization.CultureInfo]::InvariantCulture)}

$begin=[regex]::Match($log,'(?m)^.*WORKSHOP_PRIORITY_PICKUP_RUNTIME_BEGIN version=1 route=standard-urgent-timed-checkout-pickup .*surcharge_percent=20 service_multiplier=0.80 exact_vehicle=required no_precharge=required single_debit=required pickup=required.*$')
if(-not $begin.Success){throw 'Workshop priority/pickup runtime begin marker missing or wrong route/schema.'}
$priority=Marker 'PRIORITY'
$checkin=Marker 'CHECKIN'
$checkout=Marker 'CHECKOUT'
$pickup=Marker 'PICKUP'
$completeMatch=[regex]::Match($log,'(?m)^.*WORKSHOP_PRIORITY_PICKUP_RUNTIME_COMPLETE result=(PASS|FAIL).*$')
if(-not $completeMatch.Success){throw 'Workshop priority/pickup completion marker missing.'}
if($completeMatch.Groups[1].Value -ne 'PASS'){throw 'Workshop priority/pickup completion did not PASS.'}
$complete=$completeMatch.Value

foreach($field in @('priority_promoted','no_precharge','priority_persisted','urgent_timing_x080','single_debit','pickup_persisted','pickup_hold','wrong_id_rejected','exact_pickup','no_second_charge','identity_preserved','cargo_continuity')){
    if((IntField $complete $field) -ne 1){throw "Workshop priority/pickup completion gate '$field' is not PASS."}
}
foreach($field in @('queued','promoted','exact_id','surcharge_20','no_precharge','disk_priority')){
    if((IntField $priority $field) -ne 1){throw "Priority gate '$field' failed."}
}
foreach($field in @('urgent','no_precharge')){if((IntField $checkin $field) -ne 1){throw "Check-in gate '$field' failed."}}
foreach($field in @('single_debit','pickup_persisted','pickup_hold','repair_complete','identity_preserved')){
    if((IntField $checkout $field) -ne 1){throw "Checkout gate '$field' failed."}
}
foreach($field in @('wrong_id_rejected','exact_pickup','no_second_charge','identity_preserved','cargo_continuity')){
    if((IntField $pickup $field) -ne 1){throw "Pickup gate '$field' failed."}
}
if((IntField $pickup 'queue_remaining') -ne 0){throw 'Exact pickup must release the completed appointment.'}

$vehicle=Field $complete 'vehicle'
$decoy=Field $complete 'decoy'
$standardQuote=IntField $complete 'standard_quote'
$urgentQuote=IntField $complete 'urgent_quote'
$charged=IntField $checkout 'charged'
$standardDuration=FloatField $complete 'standard_duration'
$urgentDuration=FloatField $complete 'urgent_duration'
if([string]::IsNullOrWhiteSpace($vehicle) -or [string]::IsNullOrWhiteSpace($decoy) -or $vehicle -eq $decoy){throw 'Runtime exact vehicle identities are invalid.'}
if($standardQuote -le 0 -or $urgentQuote -le $standardQuote){throw 'Runtime priority quotes are invalid.'}
$expectedUrgent=$standardQuote+[Math]::Max(1,[Math]::Ceiling($standardQuote*0.20))
if($urgentQuote -ne $expectedUrgent){throw "Urgent quote mismatch: expected=$expectedUrgent actual=$urgentQuote"}
if($charged -ne $urgentQuote){throw "Single debit mismatch: charged=$charged urgent_locked=$urgentQuote"}
$expectedDuration=[Math]::Max(0.40,[Math]::Min(1.20,$standardDuration*0.80))
if([Math]::Abs($urgentDuration-$expectedDuration) -gt 0.011){throw "Urgent service duration mismatch: expected=$expectedDuration actual=$urgentDuration"}

$upgradeMarkers=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_PRIORITY_UPGRADED vehicle=$([regex]::Escape($vehicle)) .*urgent_locked_quote=$urgentQuote .*surcharge_percent=20 .*service_multiplier=0.80 charged=NO exact_id=YES.*$").Count
$checkinMarkers=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_CHECKED_IN vehicle=$([regex]::Escape($vehicle)) priority=URGENT locked_quote=$urgentQuote .*charged=NO mutation=NO exact_id=YES.*$").Count
$readyMarkers=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_READY_FOR_PICKUP vehicle=$([regex]::Escape($vehicle)) priority=URGENT paid_amount=$urgentQuote .*exact_id=YES.*$").Count
$pickupMarkers=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_PICKUP_RELEASED vehicle=$([regex]::Escape($vehicle)) paid_amount=$urgentQuote exact_id=YES repair_complete=YES fleet_return=YES.*$").Count
if($upgradeMarkers -ne 1 -or $checkinMarkers -ne 1 -or $readyMarkers -ne 1 -or $pickupMarkers -ne 1){
    throw "Production marker count mismatch: priority=$upgradeMarkers checkin=$checkinMarkers ready=$readyMarkers pickup=$pickupMarkers"
}
$diagnosticFailures=[regex]::Matches($log,'(?m)^.*WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=DIAGNOSTIC result=FAIL.*$').Count
if($diagnosticFailures -ne 0){throw "Priority/pickup route reported $diagnosticFailures diagnostic failures."}

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
$gitSha=$ExpectedGitSha
if(Test-Path $buildPath){
    $build=Get-Content -Raw $buildPath|ConvertFrom-Json
    if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
    $gitSha=$build.git_sha
}

$evidence=[ordered]@{
    schema='gtt.workshop-priority-pickup-runtime.v1'
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$gitSha
    route='standard-urgent-timed-checkout-pickup'
    vehicle_id=$vehicle
    decoy_vehicle_id=$decoy
    standard_locked_quote=$standardQuote
    urgent_locked_quote=$urgentQuote
    urgent_surcharge_percent=20
    urgent_service_multiplier=0.80
    standard_service_duration_hours=$standardDuration
    urgent_service_duration_hours=$urgentDuration
    exact_id_priority_promotion=$true
    no_precharge=$true
    priority_disk_persistence=$true
    urgent_timing_verified=$true
    single_locked_quote_debit=$true
    ready_for_pickup_persisted=$true
    pickup_hold_observed=$true
    wrong_id_pickup_rejected=$true
    exact_id_pickup_release=$true
    pickup_has_no_second_charge=$true
    identity_preserved=$true
    farm_cargo_authority_preserved=$true
    production_priority_markers=$upgradeMarkers
    production_checkin_markers=$checkinMarkers
    production_ready_markers=$readyMarkers
    production_pickup_markers=$pickupMarkers
    diagnostic_failure_count=$diagnosticFailures
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$out=Join-Path $PackageDirectory 'WORKSHOP_PRIORITY_PICKUP_RUNTIME.json'
$evidence|ConvertTo-Json -Depth 5|Set-Content -Encoding UTF8 $out
Write-Host '[GTT] Workshop priority/pickup packaged runtime evidence: PASS'
Write-Host "[GTT] Vehicle: $vehicle | STANDARD $$standardQuote -> URGENT $$urgentQuote | timing x0.80 | pickup exact-ID: PASS"
Write-Host "[GTT] Evidence: $out"
