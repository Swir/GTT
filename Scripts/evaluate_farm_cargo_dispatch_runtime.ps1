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
    $m=[regex]::Match($log,"(?m)^.*FARM_CARGO_DISPATCH_RUNTIME phase=$Phase result=(PASS|FAIL).*$")
    if(-not $m.Success){throw "Farm Cargo dispatch runtime marker missing: $Phase"}
    if($m.Groups[1].Value -ne 'PASS'){throw "Farm Cargo dispatch runtime phase failed: $Phase"}
    return $m.Value
}
function Field([string]$Line,[string]$Name){
    $m=[regex]::Match($Line,"(?:^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if(-not $m.Success){throw "Field '$Name' missing from marker: $Line"}
    return $m.Groups[1].Value
}
function IntField([string]$Line,[string]$Name){ return [int](Field $Line $Name) }
function DoubleField([string]$Line,[string]$Name){ return [double]::Parse((Field $Line $Name),[Globalization.CultureInfo]::InvariantCulture) }

$begin=[regex]::Match($log,'(?m)^.*FARM_CARGO_DISPATCH_RUNTIME_BEGIN version=1 route=feed-dispatch-cancel-rerequest-hill-wood.*$')
if(-not $begin.Success){throw 'Farm Cargo dispatch runtime begin marker missing or wrong schema/route.'}
$prepare=Marker 'PREPARE'
$accept=Marker 'ACCEPT'
$pickup=Marker 'PICKUP'
$patchRequest=Marker 'PATCH_REQUEST'
$patchObserve=Marker 'PATCH_OBSERVE'
$patchCancel=Marker 'PATCH_CANCEL'
$towRequest=Marker 'TOW_REQUEST'
$towObserve=Marker 'TOW_OBSERVE'
$towCancel=Marker 'TOW_CANCEL'
$patchRerequest=Marker 'PATCH_REREQUEST'
$patchComplete=Marker 'PATCH_COMPLETE'
$wrongVehicle=Marker 'WRONG_VEHICLE'
$hill=Marker 'HILL_HANDOFF'
$final=Marker 'FINAL_HANDOFF'
$persistence=Marker 'PERSISTENCE'
$completeMatch=[regex]::Match($log,'(?m)^.*FARM_CARGO_DISPATCH_RUNTIME_COMPLETE result=(PASS|FAIL).*$')
if(-not $completeMatch.Success){throw 'Farm Cargo dispatch runtime complete marker missing.'}
if($completeMatch.Groups[1].Value -ne 'PASS'){throw 'Farm Cargo dispatch runtime complete marker did not PASS.'}
$complete=$completeMatch.Value

$requiredOne=@(
    'accepted','pickup','patch_locked','patch_eta_advanced','patch_cancel_no_charge','tow_locked','tow_eta_advanced',
    'tow_cancel_no_charge','patch_rerequested','patch_complete','patch_charge_matched','exact_vehicle','timer_continued',
    'integrity_not_improved','wrong_vehicle_rejected','hill','final','save','authority_cleared'
)
foreach($name in $requiredOne){if((IntField $complete $name) -ne 1){throw "Dispatch runtime completion gate '$name' is not PASS."}}

$initialPatchQuote=IntField $complete 'initial_patch_quote'
$towQuote=IntField $complete 'tow_quote'
$finalPatchQuote=IntField $complete 'final_patch_quote'
$payoutDelta=IntField $complete 'payout_delta'
$cargoRunsDelta=IntField $complete 'cargo_runs_delta'
$reputationDelta=IntField $complete 'reputation_delta'
$vehicle=Field $complete 'vehicle'
if($initialPatchQuote -le 0 -or $towQuote -le 0 -or $finalPatchQuote -le 0){throw 'Dispatch runtime did not prove positive locked quotes.'}
if($payoutDelta -le 0 -or $cargoRunsDelta -ne 1 -or $reputationDelta -le 0){throw 'Dispatch runtime did not prove one authoritative payout/completion/reputation result.'}
if([string]::IsNullOrWhiteSpace($vehicle) -or $vehicle -eq 'None'){throw 'Dispatch runtime did not report a stable vehicle id.'}

if((IntField $patchRequest 'quote_locked') -ne 1 -or (IntField $patchRequest 'target_pinned') -ne 1 -or (IntField $patchRequest 'no_charge_before_arrival') -ne 1){throw 'Initial patch request did not prove locked quote + pinned target + deferred charge.'}
if((IntField $towRequest 'quote_locked') -ne 1 -or (IntField $towRequest 'target_pinned') -ne 1 -or (IntField $towRequest 'no_charge_before_arrival') -ne 1){throw 'Tow request did not prove locked quote + pinned target + deferred charge.'}
if((IntField $patchCancel 'cash_delta') -ne 0 -or $patchCancel -notmatch 'charged=NO'){throw 'Patch cancellation was not proven free of charge.'}
if((IntField $towCancel 'cash_delta') -ne 0 -or $towCancel -notmatch 'charged=NO'){throw 'Tow cancellation was not proven free of charge.'}

$patchEtaInitial=DoubleField $patchObserve 'eta_initial'
$patchEtaObserved=DoubleField $patchObserve 'eta_observed'
$towEtaInitial=DoubleField $towObserve 'eta_initial'
$towEtaObserved=DoubleField $towObserve 'eta_observed'
if($patchEtaInitial -le 0 -or $patchEtaObserved -lt 0 -or $patchEtaObserved -ge $patchEtaInitial){throw 'Patch live ETA did not advance toward arrival.'}
if($towEtaInitial -le 0 -or $towEtaObserved -lt 0 -or $towEtaObserved -ge $towEtaInitial){throw 'Tow live ETA did not advance toward arrival.'}
if((IntField $patchComplete 'charge_matched') -ne 1 -or (IntField $patchComplete 'charged') -ne $finalPatchQuote){throw 'Completed patch charge did not match the locked quote.'}
if((IntField $patchComplete 'exact_vehicle') -ne 1 -or (IntField $patchComplete 'timer_continued') -ne 1 -or (IntField $patchComplete 'integrity_not_improved') -ne 1){throw 'Completed patch did not preserve exact cargo authority/timer/integrity rules.'}
if((IntField $wrongVehicle 'rejected') -ne 1){throw 'Wrong vehicle was not rejected after dispatch exercise.'}
if((IntField $persistence 'explicit_save') -ne 1){throw 'Post-dispatch primary save was not proven.'}

$patchCancelNative=[regex]::Matches($log,'(?m)^.*NATIVE_ROADSIDE_DISPATCH_CANCELLED .*mode=PATCH .*charged=NO.*$')
$towCancelNative=[regex]::Matches($log,'(?m)^.*NATIVE_ROADSIDE_DISPATCH_CANCELLED .*mode=TOW .*charged=NO.*$')
$patchRequestedNative=[regex]::Matches($log,'(?m)^.*NATIVE_ROADSIDE_PATCH_REQUESTED .*quote_locked=YES target_pinned=YES.*$')
$towRequestedNative=[regex]::Matches($log,'(?m)^.*NATIVE_ROADSIDE_TOW_REQUESTED .*quote_locked=YES target_pinned=YES.*$')
$patchCompleteNative=[regex]::Matches($log,'(?m)^.*NATIVE_ROADSIDE_PATCH_COMPLETE .*quote_locked=YES result=PASS identity_preserved=YES.*$')
if($patchCancelNative.Count -lt 1 -or $towCancelNative.Count -lt 1){throw 'Production roadside cancellation markers are missing.'}
if($patchRequestedNative.Count -lt 2 -or $towRequestedNative.Count -lt 1 -or $patchCompleteNative.Count -lt 1){throw 'Production roadside request/completion evidence is incomplete.'}

$diagnosticFailures=[regex]::Matches($log,'(?m)^.*FARM_CARGO_DISPATCH_RUNTIME phase=DIAGNOSTIC result=FAIL.*$').Count
if($diagnosticFailures -ne 0){throw "Farm Cargo dispatch runtime reported $diagnosticFailures diagnostic failures."}

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
$gitSha=$ExpectedGitSha
if(Test-Path $buildPath){
    $build=Get-Content -Raw $buildPath|ConvertFrom-Json
    if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
    $gitSha=$build.git_sha
}

$evidence=[ordered]@{
    schema='gtt.farm-cargo-dispatch-runtime.v1'
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$gitSha
    route='feed-dispatch-cancel-rerequest-hill-wood'
    stable_vehicle_id=$vehicle
    patch_locked_quote=$initialPatchQuote
    patch_live_eta_advanced=$true
    patch_cancel_no_charge=$true
    tow_locked_quote=$towQuote
    tow_live_eta_advanced=$true
    tow_cancel_no_charge=$true
    patch_rerequest_locked_quote=$finalPatchQuote
    patch_charge_matched=$true
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
    native_patch_cancel_markers=$patchCancelNative.Count
    native_tow_cancel_markers=$towCancelNative.Count
    native_patch_request_markers=$patchRequestedNative.Count
    native_tow_request_markers=$towRequestedNative.Count
    native_patch_complete_markers=$patchCompleteNative.Count
    diagnostic_failure_count=$diagnosticFailures
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$out=Join-Path $PackageDirectory 'FARM_CARGO_DISPATCH_RUNTIME.json'
$evidence|ConvertTo-Json -Depth 5|Set-Content -Encoding UTF8 $out
Write-Host "[GTT] Farm Cargo roadside dispatch runtime evidence: PASS"
Write-Host "[GTT] Stable vehicle: $vehicle | patch quotes: $initialPatchQuote -> $finalPatchQuote | tow quote: $towQuote"
Write-Host "[GTT] Evidence: $out"
