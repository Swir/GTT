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
    $m=[regex]::Match($log,"(?m)^.*WORKSHOP_CAPACITY_RUNTIME phase=$Phase result=(PASS|FAIL).*$")
    if(-not $m.Success){throw "Workshop-capacity runtime marker missing: $Phase"}
    if($m.Groups[1].Value -ne 'PASS'){throw "Workshop-capacity runtime phase failed: $Phase"}
    return $m.Value
}
function Field([string]$Line,[string]$Name){
    $m=[regex]::Match($Line,"(?:^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if(-not $m.Success){throw "Field '$Name' missing from marker: $Line"}
    return $m.Groups[1].Value
}
function IntField([string]$Line,[string]$Name){return [int](Field $Line $Name)}

$begin=[regex]::Match($log,'(?m)^.*WORKSHOP_CAPACITY_RUNTIME_BEGIN version=1 route=two-bookings-disk-cancel-rebook-underfunded-nonblocking .*capacity=4 spacing_hours=0.75 exact_vehicle=required no_precharge=required.*$')
if(-not $begin.Success){throw 'Workshop-capacity runtime begin marker missing or wrong route/schema.'}
$book=Marker 'BOOK_TWO'
$cancel=Marker 'DISK_CANCEL'
$rebook=Marker 'REBOOK'
$execute=Marker 'EXECUTE'
$completeMatch=[regex]::Match($log,'(?m)^.*WORKSHOP_CAPACITY_RUNTIME_COMPLETE result=(PASS|FAIL).*$')
if(-not $completeMatch.Success){throw 'Workshop-capacity runtime completion marker missing.'}
if($completeMatch.Groups[1].Value -ne 'PASS'){throw 'Workshop-capacity runtime completion did not PASS.'}
$complete=$completeMatch.Value

foreach($field in @('two_bookings','no_precharge','spacing_45m','disk_roundtrip','independent_cancel','rebook_preserved','underfunded_nonblocking','later_single_debit','earlier_preserved','later_exact_service','identity_preserved','cargo_continuity')){
    if((IntField $complete $field) -ne 1){throw "Workshop-capacity completion gate '$field' is not PASS."}
}
foreach($field in @('no_precharge','spacing_45m','quote_order')){if((IntField $book $field) -ne 1){throw "Booking capacity gate '$field' failed."}}
if((IntField $book 'count') -ne 2){throw 'Expected exactly two runtime bookings.'}
foreach($field in @('disk_roundtrip','independent_cancel','first_preserved','second_removed','no_charge')){if((IntField $cancel $field) -ne 1){throw "Cancel/disk gate '$field' failed."}}
foreach($field in @('rebooked','exact_second','locked_quote_preserved','spacing_45m','no_precharge')){if((IntField $rebook $field) -ne 1){throw "Rebook gate '$field' failed."}}
foreach($field in @('underfunded_nonblocking','earlier_preserved','later_single_debit','later_exact_service','identity_preserved','cargo_continuity')){if((IntField $execute $field) -ne 1){throw "Execution capacity gate '$field' failed."}}
if((IntField $execute 'remaining') -ne 1){throw 'Earlier underfunded appointment must remain queued.'}

$firstVehicle=Field $complete 'first_vehicle'
$secondVehicle=Field $complete 'second_vehicle'
$firstQuote=IntField $complete 'first_quote'
$secondQuote=IntField $complete 'second_quote'
$charged=IntField $execute 'charged'
if([string]::IsNullOrWhiteSpace($firstVehicle) -or [string]::IsNullOrWhiteSpace($secondVehicle) -or $firstVehicle -eq $secondVehicle){throw 'Stable vehicle identities are invalid.'}
if($firstQuote -le $secondQuote -or $secondQuote -le 0){throw "Quote ordering invalid: first=$firstQuote second=$secondQuote"}
if($charged -ne $secondQuote){throw "Later appointment debit mismatch: charged=$charged locked=$secondQuote"}
$acceptFirst=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_ACCEPTED vehicle=$([regex]::Escape($firstVehicle)) .*charged=NO exact_id=YES.*$").Count
$acceptSecond=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_ACCEPTED vehicle=$([regex]::Escape($secondVehicle)) .*charged=NO exact_id=YES.*$").Count
$completeSecond=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_COMPLETED vehicle=$([regex]::Escape($secondVehicle)) charged=$secondQuote locked_quote_match=YES exact_id=YES saved=YES.*$").Count
if($acceptFirst -lt 1 -or $acceptSecond -lt 2){throw 'Expected production appointment acceptance markers are missing.'}
if($completeSecond -ne 1){throw "Expected exactly one completed marker for later vehicle, found $completeSecond."}
$diagnosticFailures=[regex]::Matches($log,'(?m)^.*WORKSHOP_CAPACITY_RUNTIME phase=DIAGNOSTIC result=FAIL.*$').Count
if($diagnosticFailures -ne 0){throw "Workshop-capacity route reported $diagnosticFailures diagnostic failures."}

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
$gitSha=$ExpectedGitSha
if(Test-Path $buildPath){
    $build=Get-Content -Raw $buildPath|ConvertFrom-Json
    if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
    $gitSha=$build.git_sha
}

$evidence=[ordered]@{
    schema='gtt.workshop-capacity-runtime.v1'
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$gitSha
    route='two-bookings-disk-cancel-rebook-underfunded-nonblocking'
    capacity=4
    appointment_spacing_minutes=45
    first_vehicle_id=$firstVehicle
    second_vehicle_id=$secondVehicle
    first_locked_quote=$firstQuote
    second_locked_quote=$secondQuote
    two_exact_bookings=$true
    no_precharge=$true
    additive_disk_roundtrip=$true
    independent_exact_id_cancel=$true
    rebook_preserves_locked_quote=$true
    deterministic_45_minute_spacing=$true
    underfunded_earlier_nonblocking=$true
    earlier_reservation_preserved=$true
    later_exact_vehicle_service=$true
    later_single_debit=$true
    identity_preserved=$true
    farm_cargo_authority_preserved=$true
    production_accept_markers=($acceptFirst+$acceptSecond)
    production_complete_markers=$completeSecond
    diagnostic_failure_count=$diagnosticFailures
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$out=Join-Path $PackageDirectory 'WORKSHOP_CAPACITY_RUNTIME.json'
$evidence|ConvertTo-Json -Depth 5|Set-Content -Encoding UTF8 $out
Write-Host '[GTT] Multi-vehicle workshop capacity packaged runtime evidence: PASS'
Write-Host "[GTT] Capacity: 4 | runtime pair: $firstVehicle / $secondVehicle | spacing: 45 minutes | underfunded non-blocking: PASS"
Write-Host "[GTT] Evidence: $out"
