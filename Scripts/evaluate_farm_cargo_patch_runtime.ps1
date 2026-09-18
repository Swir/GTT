param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)
$OutputPath = Join-Path $PackageDirectory 'FARM_CARGO_PATCH_RUNTIME.json'
$BuildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$SmokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'

foreach ($path in @($BuildPath, $SmokePath, $RuntimeLog)) {
    if (-not (Test-Path $path)) { throw "Required Farm Cargo patch evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

function Find-Phase([string]$Phase, [string]$Result = 'PASS') {
    return @($lines | Where-Object { $_ -match "FARM_CARGO_PATCH_RUNTIME\s+phase=$([regex]::Escape($Phase))\s+result=$([regex]::Escape($Result))(?:\s|$)" }) | Select-Object -Last 1
}
function Read-Number([string]$Line, [string]$Key) {
    if (-not $Line) { return $null }
    $match = [regex]::Match($Line, "(?:^|\s)$([regex]::Escape($Key))=(-?[0-9]+(?:\.[0-9]+)?)")
    if (-not $match.Success) { return $null }
    return [double]::Parse($match.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
}
function Read-Integer([string]$Line, [string]$Key) {
    $value = Read-Number $Line $Key
    if ($null -eq $value) { return $null }
    return [int]$value
}
function Read-Token([string]$Line, [string]$Key) {
    if (-not $Line) { return $null }
    $match = [regex]::Match($Line, "(?:^|\s)$([regex]::Escape($Key))=([^\s]+)")
    if (-not $match.Success) { return $null }
    return $match.Groups[1].Value
}
function Require-IntegerField([string]$Line, [string]$Key, [string]$Context) {
    $value = Read-Integer $Line $Key
    if ($null -eq $value) { $failures.Add("$Context missing required integer field '$Key'"); return $null }
    return $value
}
function Require-NumberField([string]$Line, [string]$Key, [string]$Context) {
    $value = Read-Number $Line $Key
    if ($null -eq $value) { $failures.Add("$Context missing required numeric field '$Key'"); return $null }
    return $value
}

if ($build.platform -ne 'Win64') { $failures.Add('build platform is not Win64') }
if ($smoke.result -ne 'PASS') { $failures.Add('packaged runtime smoke did not PASS') }
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    $failures.Add("build SHA mismatch package=$($build.git_sha) expected=$ExpectedGitSha")
}
if (-not (@($smoke.launch_arguments) -contains '-GTTFarmCargoPatchScenario')) {
    $failures.Add('runtime smoke did not explicitly enable -GTTFarmCargoPatchScenario')
}

$begin = @($lines | Where-Object { $_ -match 'FARM_CARGO_PATCH_RUNTIME_BEGIN\s+version=1\s+route=feed-breakdown-patch-hill-wood' }) | Select-Object -Last 1
$prepare = Find-Phase 'PREPARE'
$accept = Find-Phase 'ACCEPT'
$pickup = Find-Phase 'PICKUP'
$request = Find-Phase 'BREAKDOWN_PATCH_REQUEST'
$patch = Find-Phase 'PATCH_COMPLETE'
$wrong = Find-Phase 'WRONG_VEHICLE_AFTER_PATCH'
$hill = Find-Phase 'HILL_HANDOFF'
$final = Find-Phase 'FINAL_HANDOFF'
$persistence = Find-Phase 'PERSISTENCE'
$complete = @($lines | Where-Object { $_ -match 'FARM_CARGO_PATCH_RUNTIME_COMPLETE\s+result=PASS\s+route=feed-breakdown-patch-hill-wood' }) | Select-Object -Last 1
$diagnostics = @($lines | Where-Object { $_ -match 'FARM_CARGO_PATCH_RUNTIME\s+phase=DIAGNOSTIC\s+result=FAIL' })
$preCheckpoint = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RECOVERY\s+event=PATCH_CHECKPOINT\s+vehicle=.*\s+saved=YES\s+timer_paused=NO\s+transfer_allowed=NO' }) | Select-Object -Last 1
$postPatch = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RECOVERY\s+event=POST_PATCH_VERIFY\s+result=PASS\s+vehicle=.*\s+identity_preserved=YES\s+saved=YES\s+timer_paused=NO\s+transfer_allowed=NO' }) | Select-Object -Last 1
$nativePatchRequested = @($lines | Where-Object { $_ -match 'NATIVE_ROADSIDE_PATCH_REQUESTED\s+vehicle=.*\s+patch_quote=[0-9]+\s+player_authorized=YES' }) | Select-Object -Last 1
$nativePatchComplete = @($lines | Where-Object { $_ -match 'NATIVE_ROADSIDE_PATCH_COMPLETE\s+vehicle=.*\s+cost=[0-9]+\s+result=PASS\s+identity_preserved=YES\s+body_preserved=YES\s+workshop_repair_still_required=YES' }) | Select-Object -Last 1

$required = [ordered]@{
    PREPARE=$prepare; ACCEPT=$accept; PICKUP=$pickup; BREAKDOWN_PATCH_REQUEST=$request; PATCH_COMPLETE=$patch;
    WRONG_VEHICLE_AFTER_PATCH=$wrong; HILL_HANDOFF=$hill; FINAL_HANDOFF=$final; PERSISTENCE=$persistence
}
if (-not $begin) { $failures.Add('Farm Cargo patch runtime begin marker is missing') }
foreach ($entry in $required.GetEnumerator()) { if (-not $entry.Value) { $failures.Add("required patch phase $($entry.Key) was not proven") } }
if (-not $complete) { $failures.Add('Farm Cargo patch route did not complete with PASS') }
if ($diagnostics.Count -gt 0) { $failures.Add("scenario logged $($diagnostics.Count) diagnostic failure(s)") }
if (-not $preCheckpoint) { $failures.Add('production cargo recovery did not prove the pre-patch primary-save checkpoint') }
if (-not $postPatch) { $failures.Add('production cargo recovery did not prove exact-ID post-patch verification') }
if (-not $nativePatchRequested) { $failures.Add('native roadside service did not prove player-authorized paid patch dispatch') }
if (-not $nativePatchComplete) { $failures.Add('native roadside service did not prove completed patch with body preservation') }

$breakdown = Require-IntegerField $request 'breakdown' 'BREAKDOWN_PATCH_REQUEST'
$patchAvailable = Require-IntegerField $request 'patch_available' 'BREAKDOWN_PATCH_REQUEST'
$patchRequested = Require-IntegerField $request 'patch_requested' 'BREAKDOWN_PATCH_REQUEST'
$severity = Require-NumberField $request 'severity' 'BREAKDOWN_PATCH_REQUEST'
$tireBefore = Require-NumberField $request 'tire_before' 'BREAKDOWN_PATCH_REQUEST'
$conditionBefore = Require-NumberField $request 'condition_before' 'BREAKDOWN_PATCH_REQUEST'
$fuelBefore = Require-NumberField $request 'fuel_before' 'BREAKDOWN_PATCH_REQUEST'
$timerBefore = Require-NumberField $request 'timer_before' 'BREAKDOWN_PATCH_REQUEST'
$integrityBefore = Require-NumberField $request 'integrity_before' 'BREAKDOWN_PATCH_REQUEST'
$patchComplete = Require-IntegerField $patch 'patch_complete' 'PATCH_COMPLETE'
$identity = Require-IntegerField $patch 'identity_preserved' 'PATCH_COMPLETE'
$timerContinued = Require-IntegerField $patch 'timer_continued' 'PATCH_COMPLETE'
$integrityStable = Require-IntegerField $patch 'integrity_not_improved' 'PATCH_COMPLETE'
$bodyPreserved = Require-IntegerField $patch 'body_preserved' 'PATCH_COMPLETE'
$limpFloors = Require-IntegerField $patch 'limp_home_floors' 'PATCH_COMPLETE'
$patchCost = Require-IntegerField $patch 'patch_cost_delta' 'PATCH_COMPLETE'
$timerAfter = Require-NumberField $patch 'timer_after' 'PATCH_COMPLETE'
$integrityAfter = Require-NumberField $patch 'integrity_after' 'PATCH_COMPLETE'
$tireAfter = Require-NumberField $patch 'tire_after' 'PATCH_COMPLETE'
$conditionAfter = Require-NumberField $patch 'condition_after' 'PATCH_COMPLETE'
$fuelAfter = Require-NumberField $patch 'fuel_after' 'PATCH_COMPLETE'
$wrongRejected = Require-IntegerField $wrong 'wrong_vehicle_rejected' 'WRONG_VEHICLE_AFTER_PATCH'
$hillPass = Require-IntegerField $hill 'hill' 'HILL_HANDOFF'
$finalPass = Require-IntegerField $final 'final' 'FINAL_HANDOFF'
$payoutDelta = Require-IntegerField $final 'payout_delta' 'FINAL_HANDOFF'
$cargoRunsDelta = Require-IntegerField $final 'cargo_runs_delta' 'FINAL_HANDOFF'
$reputationDelta = Require-IntegerField $final 'reputation_delta' 'FINAL_HANDOFF'
$authorityCleared = Require-IntegerField $final 'authority_cleared' 'FINAL_HANDOFF'
$finalSave = Require-IntegerField $persistence 'explicit_save' 'PERSISTENCE'

foreach ($gate in @(
    @{Name='breakdown';Value=$breakdown}, @{Name='patch_available';Value=$patchAvailable}, @{Name='patch_requested';Value=$patchRequested},
    @{Name='patch_complete';Value=$patchComplete}, @{Name='identity_preserved';Value=$identity}, @{Name='timer_continued';Value=$timerContinued},
    @{Name='integrity_not_improved';Value=$integrityStable}, @{Name='body_preserved';Value=$bodyPreserved}, @{Name='limp_home_floors';Value=$limpFloors},
    @{Name='wrong_vehicle_rejected';Value=$wrongRejected}, @{Name='hill';Value=$hillPass}, @{Name='final';Value=$finalPass},
    @{Name='authority_cleared';Value=$authorityCleared}, @{Name='final_save';Value=$finalSave}
)) {
    if ($null -ne $gate.Value -and $gate.Value -ne 1) { $failures.Add("patch gate '$($gate.Name)' was not 1 (value=$($gate.Value))") }
}
if ($null -ne $severity -and $severity -le 0) { $failures.Add("breakdown severity was not positive (severity=$severity)") }
if ($null -ne $tireBefore -and $tireBefore -gt 0.08) { $failures.Add("damage probe did not reach patch-eligible tire failure (tire=$tireBefore)") }
if ($null -ne $patchCost -and $patchCost -le 0) { $failures.Add("roadside patch did not charge positive cash delta (delta=$patchCost)") }
if ($null -ne $timerBefore -and $null -ne $timerAfter -and $timerAfter -ge $timerBefore) { $failures.Add("cargo timer did not continue during patch ($timerBefore -> $timerAfter)") }
if ($null -ne $integrityBefore -and $null -ne $integrityAfter -and $integrityAfter -gt ($integrityBefore + 0.0001)) { $failures.Add("cargo integrity improved during patch ($integrityBefore -> $integrityAfter)") }
if ($null -ne $tireAfter -and $tireAfter -lt 0.319) { $failures.Add("roadside patch did not restore the tire limp-home floor (tire=$tireAfter)") }
if ($null -ne $conditionAfter -and $conditionAfter -lt 0.299) { $failures.Add("roadside patch did not restore the condition limp-home floor (condition=$conditionAfter)") }
if ($null -ne $fuelAfter -and $null -ne $fuelBefore -and $fuelAfter + 0.001 -lt $fuelBefore) { $failures.Add("roadside patch reduced fuel ($fuelBefore -> $fuelAfter)") }
if ($null -ne $payoutDelta -and $payoutDelta -le 0) { $failures.Add("delivery payout did not increase cash after patch (delta=$payoutDelta)") }
if ($null -ne $cargoRunsDelta -and $cargoRunsDelta -ne 1) { $failures.Add("cargo completion history delta was not exactly one (delta=$cargoRunsDelta)") }
if ($null -ne $reputationDelta -and $reputationDelta -le 0) { $failures.Add("logistics reputation did not increase (delta=$reputationDelta)") }

$completeFields = @('accepted','pickup','breakdown','patch_available','patch_requested','patch_complete','identity_preserved','timer_continued','integrity_not_improved','body_preserved','limp_home_floors','wrong_vehicle_rejected','hill','final','save','authority_cleared')
$completeValues = @{}
foreach ($field in $completeFields) {
    $completeValues[$field] = Require-IntegerField $complete $field 'COMPLETE'
    if ($null -ne $completeValues[$field] -and $completeValues[$field] -ne 1) { $failures.Add("completion marker gate '$field' was not 1 (value=$($completeValues[$field]))") }
}
$completePatchCost = Require-IntegerField $complete 'patch_cost_delta' 'COMPLETE'
$completePayout = Require-IntegerField $complete 'payout_delta' 'COMPLETE'
$completeRuns = Require-IntegerField $complete 'cargo_runs_delta' 'COMPLETE'
$completeReputation = Require-IntegerField $complete 'reputation_delta' 'COMPLETE'
$completeVehicle = Read-Token $complete 'vehicle'
if (-not $completeVehicle -or $completeVehicle -eq 'None') { $failures.Add('completion marker did not carry a stable cargo vehicle ID') }
if ($null -ne $patchCost -and $null -ne $completePatchCost -and $patchCost -ne $completePatchCost) { $failures.Add("patch cost disagrees between PATCH_COMPLETE and COMPLETE ($patchCost vs $completePatchCost)") }
if ($null -ne $payoutDelta -and $null -ne $completePayout -and $payoutDelta -ne $completePayout) { $failures.Add("payout disagrees between FINAL_HANDOFF and COMPLETE ($payoutDelta vs $completePayout)") }
if ($null -ne $cargoRunsDelta -and $null -ne $completeRuns -and $cargoRunsDelta -ne $completeRuns) { $failures.Add("cargo runs disagree between FINAL_HANDOFF and COMPLETE ($cargoRunsDelta vs $completeRuns)") }
if ($null -ne $reputationDelta -and $null -ne $completeReputation -and $reputationDelta -ne $completeReputation) { $failures.Add("reputation disagrees between FINAL_HANDOFF and COMPLETE ($reputationDelta vs $completeReputation)") }

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.farm-cargo-patch-runtime.v1'; result = $result; game = 'Grand Theft Tractor'
    version = $build.version; platform = $build.platform; git_sha = $build.git_sha
    route = 'Feed Depot -> native Mulebox breakdown -> paid emergency patch -> Hill Farm -> North Wood Yard'
    stable_vehicle_id = $completeVehicle; native_breakdown_proven = ($breakdown -eq 1); emergency_patch_available = ($patchAvailable -eq 1)
    player_authorized_patch = ($patchRequested -eq 1); patch_completed = ($patchComplete -eq 1)
    exact_vehicle_identity_preserved = ($identity -eq 1); timer_continued = ($timerContinued -eq 1)
    cargo_integrity_not_improved = ($integrityStable -eq 1); body_damage_preserved = ($bodyPreserved -eq 1)
    limp_home_floors_applied = ($limpFloors -eq 1); wrong_vehicle_rejected_after_patch = ($wrongRejected -eq 1)
    patch_cost_delta = $patchCost; tire_before = $tireBefore; tire_after = $tireAfter; condition_before = $conditionBefore; condition_after = $conditionAfter
    fuel_before = $fuelBefore; fuel_after = $fuelAfter; payout_delta = $payoutDelta; cargo_completed_runs_delta = $cargoRunsDelta
    logistics_reputation_delta = $reputationDelta; authority_cleared = ($authorityCleared -eq 1); final_save = ($finalSave -eq 1)
    production_pre_patch_checkpoint = [bool]$preCheckpoint; production_post_patch_identity_verification = [bool]$postPatch
    native_patch_request_marker = [bool]$nativePatchRequested; native_patch_complete_marker = [bool]$nativePatchComplete
    diagnostic_failure_count = $diagnostics.Count; failures = @($failures); evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath
if ($result -ne 'PASS') { Write-Error "Farm Cargo emergency-patch packaged runtime exercise failed: $($failures -join '; ')"; exit 10 }
Write-Host "[GTT] Farm Cargo emergency-patch runtime: PASS (vehicle=$completeVehicle, patch=$patchCost, payout=+$payoutDelta, rep=+$reputationDelta)."
Write-Host "[GTT] Evidence: $OutputPath"