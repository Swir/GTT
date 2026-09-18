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
    $m=[regex]::Match($log,"(?m)^.*FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=$Phase result=(PASS|FAIL).*$")
    if(-not $m.Success){throw "Dispatch persistence runtime marker missing: $Phase"}
    if($m.Groups[1].Value -ne 'PASS'){throw "Dispatch persistence runtime phase failed: $Phase"}
    return $m.Value
}
function Field([string]$Line,[string]$Name){
    $m=[regex]::Match($Line,"(?:^|\s)$([regex]::Escape($Name))=([^\s]+)")
    if(-not $m.Success){throw "Field '$Name' missing from marker: $Line"}
    return $m.Groups[1].Value
}
function IntField([string]$Line,[string]$Name){return [int](Field $Line $Name)}
function DoubleField([string]$Line,[string]$Name){return [double]::Parse((Field $Line $Name),[Globalization.CultureInfo]::InvariantCulture)}

$begin=[regex]::Match($log,'(?m)^.*FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME_BEGIN version=1 route=feed-tow-reload-wanted-reject-patch-reload-hill-wood.*$')
if(-not $begin.Success){throw 'Dispatch persistence runtime begin marker missing or wrong schema/route.'}
$prepare=Marker 'PREPARE'
$accept=Marker 'ACCEPT'
$pickup=Marker 'PICKUP'
$towRequest=Marker 'TOW_REQUEST'
$towReload=Marker 'TOW_RELOAD'
$towRestored=Marker 'TOW_RESTORED'
$towCancel=Marker 'TOW_CANCEL_AFTER_RESTORE'
$wantedReload=Marker 'WANTED_RELOAD'
$wantedReject=Marker 'WANTED_REJECT'
$patchRequest=Marker 'PATCH_REQUEST'
$patchReload=Marker 'PATCH_RELOAD'
$patchRestored=Marker 'PATCH_RESTORED'
$patchComplete=Marker 'PATCH_COMPLETE'
$wrongVehicle=Marker 'WRONG_VEHICLE'
$hill=Marker 'HILL_HANDOFF'
$final=Marker 'FINAL_HANDOFF'
$persistence=Marker 'PERSISTENCE'
$completeMatch=[regex]::Match($log,'(?m)^.*FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME_COMPLETE result=(PASS|FAIL).*$')
if(-not $completeMatch.Success){throw 'Dispatch persistence runtime completion marker missing.'}
if($completeMatch.Groups[1].Value -ne 'PASS'){throw 'Dispatch persistence runtime completion marker did not PASS.'}
$complete=$completeMatch.Value

$requiredOne=@(
    'accepted','pickup','tow_checkpoint','tow_primary_save','tow_primary_load','tow_rearm','tow_restored',
    'tow_quote_preserved','tow_eta_preserved','tow_exact_vehicle','tow_cancel_no_charge',
    'wanted_checkpoint','wanted_primary_save','wanted_primary_load','wanted_rearm','wanted_rejected_no_charge',
    'patch_checkpoint','patch_primary_save','patch_primary_load','patch_rearm','patch_restored','patch_quote_preserved',
    'patch_eta_preserved','patch_exact_vehicle','patch_no_charge_before_arrival','patch_completed','patch_single_charge',
    'timer_continued','integrity_not_improved','wrong_vehicle_rejected','hill','final','save','authority_cleared'
)
foreach($name in $requiredOne){if((IntField $complete $name) -ne 1){throw "Dispatch persistence completion gate '$name' is not PASS."}}

$towQuote=IntField $complete 'tow_quote'
$wantedTowQuote=IntField $complete 'wanted_tow_quote'
$patchQuote=IntField $complete 'patch_quote'
$payoutDelta=IntField $complete 'payout_delta'
$cargoRunsDelta=IntField $complete 'cargo_runs_delta'
$reputationDelta=IntField $complete 'reputation_delta'
$vehicle=Field $complete 'vehicle'
if($towQuote -le 0 -or $wantedTowQuote -le 0 -or $patchQuote -le 0){throw 'Dispatch persistence runtime did not prove positive locked quotes.'}
if($payoutDelta -le 0 -or $cargoRunsDelta -ne 1 -or $reputationDelta -le 0){throw 'Dispatch persistence runtime did not prove one authoritative route completion.'}
if([string]::IsNullOrWhiteSpace($vehicle) -or $vehicle -eq 'None'){throw 'Dispatch persistence runtime did not report a stable vehicle id.'}

$towSavedEta=DoubleField $towReload 'saved_eta'
$towRestoredEta=DoubleField $towRestored 'restored_eta'
$patchSavedEta=DoubleField $patchReload 'saved_eta'
$patchRestoredEta=DoubleField $patchRestored 'restored_eta'
if($towSavedEta -le 0 -or $towRestoredEta -le 0 -or $towRestoredEta -gt $towSavedEta + 0.40){throw 'Tow ETA was not preserved across SaveGame restore.'}
if($patchSavedEta -le 0 -or $patchRestoredEta -le 0 -or $patchRestoredEta -gt $patchSavedEta + 0.40){throw 'Patch ETA was not preserved across SaveGame restore.'}
if((IntField $towCancel 'cancel_no_charge') -ne 1 -or (IntField $towCancel 'cash_delta') -ne 0){throw 'Restored tow cancellation charged cash or failed.'}
if((IntField $wantedReject 'rejected_no_charge') -ne 1 -or (IntField $wantedReject 'sidecar_cleared') -ne 1 -or (IntField $wantedReject 'cash_delta') -ne 0){throw 'Wanted restore did not fail closed without charge.'}
if((IntField $patchComplete 'single_charge') -ne 1 -or (IntField $patchComplete 'charged') -ne $patchQuote){throw 'Restored patch was not charged exactly once at locked quote.'}
if((IntField $patchComplete 'exact_vehicle') -ne 1 -or (IntField $patchComplete 'timer_continued') -ne 1 -or (IntField $patchComplete 'integrity_not_improved') -ne 1){throw 'Restored patch broke Farm Cargo continuity.'}
if((IntField $wrongVehicle 'rejected') -ne 1){throw 'Wrong vehicle was not rejected after dispatch restore.'}
if((IntField $persistence 'explicit_save') -ne 1){throw 'Final primary save was not proven.'}

$reloadPass=[regex]::Matches($log,'(?m)^.*NATIVE_ROADSIDE_DISPATCH_EVIDENCE_RELOAD result=PASS checkpoint_pending=YES charged=NO.*$').Count
$restorePass=[regex]::Matches($log,'(?m)^.*NATIVE_ROADSIDE_DISPATCH_RESTORED .*exact_id=YES charged=NO.*$').Count
$wantedRejectNative=[regex]::Matches($log,'(?m)^.*NATIVE_ROADSIDE_DISPATCH_RESTORE_REJECTED .*reason=WANTED .*charged=NO.*$').Count
if($reloadPass -lt 3){throw "Expected at least three guarded sidecar reloads, found $reloadPass."}
if($restorePass -lt 2){throw "Expected restored tow + patch production markers, found $restorePass."}
if($wantedRejectNative -lt 1){throw 'Production Wanted rejection marker missing.'}

$diagnosticFailures=[regex]::Matches($log,'(?m)^.*FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=DIAGNOSTIC result=FAIL.*$').Count
if($diagnosticFailures -ne 0){throw "Dispatch persistence route reported $diagnosticFailures diagnostic failures."}

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
$gitSha=$ExpectedGitSha
if(Test-Path $buildPath){
    $build=Get-Content -Raw $buildPath|ConvertFrom-Json
    if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
    $gitSha=$build.git_sha
}

$evidence=[ordered]@{
    schema='gtt.farm-cargo-dispatch-persistence-runtime.v1'
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$gitSha
    route='feed-tow-reload-wanted-reject-patch-reload-hill-wood'
    stable_vehicle_id=$vehicle
    tow_locked_quote=$towQuote
    tow_checkpoint_saved=$true
    tow_primary_save=$true
    tow_primary_load=$true
    tow_restore_rearmed=$true
    tow_restored=$true
    tow_quote_preserved=$true
    tow_eta_preserved=$true
    tow_exact_vehicle=$true
    tow_cancel_no_charge=$true
    wanted_tow_locked_quote=$wantedTowQuote
    wanted_checkpoint_saved=$true
    wanted_primary_save=$true
    wanted_primary_load=$true
    wanted_restore_rearmed=$true
    wanted_restore_rejected_no_charge=$true
    wanted_sidecar_cleared=$true
    patch_locked_quote=$patchQuote
    patch_checkpoint_saved=$true
    patch_primary_save=$true
    patch_primary_load=$true
    patch_restore_rearmed=$true
    patch_restored=$true
    patch_quote_preserved=$true
    patch_eta_preserved=$true
    patch_exact_vehicle=$true
    patch_no_charge_before_arrival=$true
    patch_single_charge=$true
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
    guarded_reload_markers=$reloadPass
    restored_dispatch_markers=$restorePass
    wanted_reject_markers=$wantedRejectNative
    diagnostic_failure_count=$diagnosticFailures
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$out=Join-Path $PackageDirectory 'FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME.json'
$evidence|ConvertTo-Json -Depth 5|Set-Content -Encoding UTF8 $out
Write-Host '[GTT] Farm Cargo roadside dispatch persistence runtime evidence: PASS'
Write-Host "[GTT] Stable vehicle: $vehicle | tow quote: $towQuote | wanted quote: $wantedTowQuote | patch quote: $patchQuote"
Write-Host "[GTT] Evidence: $out"
