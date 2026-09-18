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
    $m=[regex]::Match($log,"(?m)^.*WORKSHOP_QUEUE_RUNTIME phase=$Phase result=(PASS|FAIL).*$")
    if(-not $m.Success){throw "Workshop-queue runtime marker missing: $Phase"}
    if($m.Groups[1].Value -ne 'PASS'){throw "Workshop-queue runtime phase failed: $Phase"}
    return $m.Value
}
function Field([string]$Line,[string]$Name){
    $m=[regex]::Match($Line,"(?:^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if(-not $m.Success){throw "Field '$Name' missing from marker: $Line"}
    return $m.Groups[1].Value
}
function IntField([string]$Line,[string]$Name){return [int](Field $Line $Name)}

$begin=[regex]::Match($log,'(?m)^.*WORKSHOP_QUEUE_RUNTIME_BEGIN version=1 route=after-hours-book-checkpoint-substitute-exact-service .*exact_vehicle=required no_precharge=required single_debit=required.*$')
if(-not $begin.Success){throw 'Workshop-queue runtime begin marker missing or wrong route/schema.'}
$book=Marker 'BOOK'
$checkpoint=Marker 'CHECKPOINT_LOAD'
$substitute=Marker 'SUBSTITUTE'
$service=Marker 'EXACT_SERVICE'
$cargo=Marker 'CARGO'
$completeMatch=[regex]::Match($log,'(?m)^.*WORKSHOP_QUEUE_RUNTIME_COMPLETE result=(PASS|FAIL).*$')
if(-not $completeMatch.Success){throw 'Workshop-queue runtime completion marker missing.'}
if($completeMatch.Groups[1].Value -ne 'PASS'){throw 'Workshop-queue runtime completion did not PASS.'}
$complete=$completeMatch.Value

foreach($field in @('booked','no_precharge','checkpoint_loaded','locked_quote_preserved','substitute_rejected','reservation_preserved','single_debit','exact_execution','sidecar_cleared','identity_preserved','repaired','refuelled','cargo_continuity')){
    if((IntField $complete $field) -ne 1){throw "Workshop-queue completion gate '$field' is not PASS."}
}
foreach($field in @('accepted','no_precharge','exact_id','sidecar_saved')){if((IntField $book $field) -ne 1){throw "Booking gate '$field' failed."}}
foreach($field in @('loaded','exact_id','locked_quote_preserved','no_precharge')){if((IntField $checkpoint $field) -ne 1){throw "Checkpoint gate '$field' failed."}}
foreach($field in @('substitute_rejected','reservation_preserved','no_charge')){if((IntField $substitute $field) -ne 1){throw "Substitute gate '$field' failed."}}
foreach($field in @('completed','single_debit','sidecar_cleared','identity_preserved','repaired','refuelled')){if((IntField $service $field) -ne 1){throw "Exact-service gate '$field' failed."}}
if((IntField $cargo 'authority_preserved') -ne 1){throw 'Farm Cargo authority continuity failed.'}

$vehicle=Field $complete 'vehicle'
$locked=IntField $complete 'locked_quote'
$charged=IntField $service 'charged'
if([string]::IsNullOrWhiteSpace($vehicle) -or $vehicle -eq 'None'){throw 'Stable vehicle id is empty.'}
if($locked -le 0 -or $charged -ne $locked){throw "Queued service debit mismatch: charged=$charged locked=$locked"}
$acceptedCount=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_ACCEPTED vehicle=$([regex]::Escape($vehicle)) .*charged=NO exact_id=YES.*$").Count
$completedCount=[regex]::Matches($log,"(?m)^.*WORKSHOP_QUEUE_COMPLETED vehicle=$([regex]::Escape($vehicle)) charged=$locked locked_quote_match=YES exact_id=YES saved=YES.*$").Count
if($acceptedCount -lt 1){throw 'Production WORKSHOP_QUEUE_ACCEPTED marker is missing.'}
if($completedCount -ne 1){throw "Expected exactly one production WORKSHOP_QUEUE_COMPLETED marker, found $completedCount."}
$diagnosticFailures=[regex]::Matches($log,'(?m)^.*WORKSHOP_QUEUE_RUNTIME phase=DIAGNOSTIC result=FAIL.*$').Count
if($diagnosticFailures -ne 0){throw "Workshop-queue route reported $diagnosticFailures diagnostic failures."}

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
$gitSha=$ExpectedGitSha
if(Test-Path $buildPath){
    $build=Get-Content -Raw $buildPath|ConvertFrom-Json
    if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
    $gitSha=$build.git_sha
}

$evidence=[ordered]@{
    schema='gtt.workshop-queue-runtime.v1'
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$gitSha
    route='after-hours-book-checkpoint-substitute-exact-service'
    stable_vehicle_id=$vehicle
    locked_quote=$locked
    no_precharge=$true
    checkpoint_disk_roundtrip=$true
    exact_id_persisted=$true
    locked_quote_persisted=$true
    substitute_vehicle_rejected=$true
    reservation_preserved_until_exact_vehicle=$true
    exact_vehicle_service=$true
    single_debit=$true
    sidecar_cleared=$true
    identity_preserved=$true
    mechanical_repaired=$true
    refuelled=$true
    farm_cargo_authority_preserved=$true
    production_accept_markers=$acceptedCount
    production_complete_markers=$completedCount
    diagnostic_failure_count=$diagnosticFailures
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$out=Join-Path $PackageDirectory 'WORKSHOP_QUEUE_RUNTIME.json'
$evidence|ConvertTo-Json -Depth 5|Set-Content -Encoding UTF8 $out
Write-Host '[GTT] Workshop repair queue packaged runtime evidence: PASS'
Write-Host "[GTT] Vehicle: $vehicle | locked/charged: $locked | exact-ID persistence + substitute rejection: PASS"
Write-Host "[GTT] Evidence: $out"
