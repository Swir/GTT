param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)
$OutputPath = Join-Path $PackageDirectory 'FARM_CARGO_BREAKDOWN_RUNTIME.json'
$BuildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$SmokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'

foreach ($path in @($BuildPath, $SmokePath, $RuntimeLog)) {
    if (-not (Test-Path $path)) { throw "Required Farm Cargo breakdown evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

function Find-Phase([string]$Phase, [string]$Result = 'PASS') {
    return @($lines | Where-Object { $_ -match "FARM_CARGO_BREAKDOWN_RUNTIME\s+phase=$([regex]::Escape($Phase))\s+result=$([regex]::Escape($Result))(?:\s|$)" }) | Select-Object -Last 1
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
if (-not (@($smoke.launch_arguments) -contains '-GTTFarmCargoBreakdownScenario')) {
    $failures.Add('runtime smoke did not explicitly enable -GTTFarmCargoBreakdownScenario')
}

$begin = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RUNTIME_BEGIN\s+version=2\s+route=feed-breakdown-patch-tow-hill-wood' }) | Select-Object -Last 1
$prepare = Find-Phase 'PREPARE'
$accept = Find-Phase 'ACCEPT'
$pickup = Find-Phase 'PICKUP'
$patchRequest = Find-Phase 'BREAKDOWN_PATCH_REQUEST'
$patchComplete = Find-Phase 'PATCH_COMPLETE'
$patchCooldown = Find-Phase 'PATCH_COOLDOWN'
$towRequest = Find-Phase 'BREAKDOWN_TOW_REQUEST'
$tow = Find-Phase 'TOW_COMPLETE'
$wrong = Find-Phase 'WRONG_VEHICLE_AFTER_TOW'
$hill = Find-Phase 'HILL_HANDOFF'
$final = Find-Phase 'FINAL_HANDOFF'
$persistence = Find-Phase 'PERSISTENCE'
$complete = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RUNTIME_COMPLETE\s+result=PASS\s+route=feed-breakdown-patch-tow-hill-wood' }) | Select-Object -Last 1
$diagnostics = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RUNTIME\s+phase=DIAGNOSTIC\s+result=FAIL' })
$prePatchCheckpoint = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RECOVERY\s+event=PATCH_CHECKPOINT\s+vehicle=.*\s+saved=YES\s+timer_paused=NO\s+transfer_allowed=NO' }) | Select-Object -Last 1
$postPatchRecovery = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RECOVERY\s+event=POST_PATCH_VERIFY\s+result=PASS\s+vehicle=.*\s+identity_preserved=YES\s+saved=YES\s+timer_paused=NO\s+transfer_allowed=NO' }) | Select-Object -Last 1
$nativePatchRequested = @($lines | Where-Object { $_ -match 'NATIVE_ROADSIDE_PATCH_REQUESTED\s+vehicle=.*\s+patch_quote=[0-9]+\s+player_authorized=YES' }) | Select-Object -Last 1
$nativePatchComplete = @($lines | Where-Object { $_ -match 'NATIVE_ROADSIDE_PATCH_COMPLETE\s+vehicle=.*\s+cost=[0-9]+\s+result=PASS\s+identity_preserved=YES\s+body_preserved=YES\s+workshop_repair_still_required=YES' }) | Select-Object -Last 1
$preTowCheckpoint = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RECOVERY\s+event=TOW_CHECKPOINT\s+vehicle=.*\s+saved=YES\s+timer_paused=NO\s+transfer_allowed=NO' }) | Select-Object -Last 1
$postTowRecovery = @($lines | Where-Object { $_ -match 'FARM_CARGO_BREAKDOWN_RECOVERY\s+event=POST_RECOVERY_VERIFY\s+result=PASS\s+vehicle=.*\s+.*identity_preserved=YES\s+saved=YES\s+timer_paused=NO' }) | Select-Object -Last 1
$nativeTowRequested = @($lines | Where-Object { $_ -match 'NATIVE_ROADSIDE_TOW_REQUESTED\s+vehicle=.*\s+tow_quote=[0-9]+\s+repair_quote=[0-9]+\s+player_authorized=YES' }) | Select-Object -Last 1
$nativeTowComplete = @($lines | Where-Object { $_ -match 'NATIVE_ROADSIDE_TOW_COMPLETE\s+vehicle=.*\s+tow_cost=[0-9]+.*damage_preserved=YES' }) | Select-Object -Last 1

$required = [ordered]@{
    PREPARE=$prepare; ACCEPT=$accept; PICKUP=$pickup; BREAKDOWN_PATCH_REQUEST=$patchRequest; PATCH_COMPLETE=$patchComplete;
    PATCH_COOLDOWN=$patchCooldown; BREAKDOWN_TOW_REQUEST=$towRequest; TOW_COMPLETE=$tow;
    WRONG_VEHICLE_AFTER_TOW=$wrong; HILL_HANDOFF=$hill; FINAL_HANDOFF=$final; PERSISTENCE=$persistence
}
if (-not $begin) { $failures.Add('Farm Cargo patch+breakdown runtime begin marker is missing') }
foreach ($entry in $required.GetEnumerator()) { if (-not $entry.Value) { $failures.Add("required breakdown phase $($entry.Key) was not proven") } }
if (-not $complete) { $failures.Add('Farm Cargo patch+breakdown route did not complete with PASS') }
if ($diagnostics.Count -gt 0) { $failures.Add("scenario logged $($diagnostics.Count) diagnostic failure(s)") }
if (-not $prePatchCheckpoint) { $failures.Add('production cargo recovery did not prove the pre-patch primary-save checkpoint') }
if (-not $postPatchRecovery) { $failures.Add('production cargo recovery did not prove exact-ID post-patch verification') }
if (-not $nativePatchRequested) { $failures.Add('native roadside service did not prove player-authorized emergency patch dispatch') }
if (-not $nativePatchComplete) { $failures.Add('native roadside service did not prove completed patch with body/identity preservation') }
if (-not $preTowCheckpoint) { $failures.Add('production cargo recovery did not prove the pre-tow primary-save checkpoint') }
if (-not $postTowRecovery) { $failures.Add('production cargo recovery did not prove exact-ID post-tow recovery') }
if (-not $nativeTowRequested) { $failures.Add('native roadside service did not prove player-authorized paid tow dispatch') }
if (-not $nativeTowComplete) { $failures.Add('native roadside service did not prove completed tow with preserved damage') }

$patchBreakdown = Require-IntegerField $patchRequest 'breakdown' 'BREAKDOWN_PATCH_REQUEST'
$patchRequested = Require-IntegerField $patchRequest 'patch_requested' 'BREAKDOWN_PATCH_REQUEST'
$patchSeverity = Require-NumberField $patchRequest 'severity' 'BREAKDOWN_PATCH_REQUEST'
$patchQuote = Require-IntegerField $patchRequest 'patch_quote' 'BREAKDOWN_PATCH_REQUEST'
$patchConditionBefore = Require-NumberField $patchRequest 'condition' 'BREAKDOWN_PATCH_REQUEST'
$patchTireBefore = Require-NumberField $patchRequest 'tire_integrity' 'BREAKDOWN_PATCH_REQUEST'
$patchTimerBefore = Require-NumberField $patchRequest 'timer_before' 'BREAKDOWN_PATCH_REQUEST'
$patchIntegrityBefore = Require-NumberField $patchRequest 'integrity_before' 'BREAKDOWN_PATCH_REQUEST'
$patchCompleteGate = Require-IntegerField $patchComplete 'patch_complete' 'PATCH_COMPLETE'
$patchIdentity = Require-IntegerField $patchComplete 'identity_preserved' 'PATCH_COMPLETE'
$patchBody = Require-IntegerField $patchComplete 'body_preserved' 'PATCH_COMPLETE'
$patchTimer = Require-IntegerField $patchComplete 'timer_continued' 'PATCH_COMPLETE'
$patchIntegrity = Require-IntegerField $patchComplete 'integrity_not_improved' 'PATCH_COMPLETE'
$patchWorkshop = Require-IntegerField $patchComplete 'workshop_still_required' 'PATCH_COMPLETE'
$patchCost = Require-IntegerField $patchComplete 'patch_cost_delta' 'PATCH_COMPLETE'
$patchTimerAfter = Require-NumberField $patchComplete 'timer_after' 'PATCH_COMPLETE'
$patchIntegrityAfter = Require-NumberField $patchComplete 'integrity_after' 'PATCH_COMPLETE'
$conditionAfterPatch = Require-NumberField $patchComplete 'condition_after' 'PATCH_COMPLETE'
$tireAfterPatch = Require-NumberField $patchComplete 'tire_after' 'PATCH_COMPLETE'
$fuelAfterPatch = Require-NumberField $patchComplete 'fuel_after' 'PATCH_COMPLETE'
$cooldownWait = Require-NumberField $patchCooldown 'waited' 'PATCH_COOLDOWN'

foreach ($gate in @(
    @{Name='patch_breakdown';Value=$patchBreakdown}, @{Name='patch_requested';Value=$patchRequested}, @{Name='patch_complete';Value=$patchCompleteGate},
    @{Name='patch_identity_preserved';Value=$patchIdentity}, @{Name='patch_body_preserved';Value=$patchBody},
    @{Name='patch_timer_continued';Value=$patchTimer}, @{Name='patch_integrity_not_improved';Value=$patchIntegrity},
    @{Name='patch_workshop_still_required';Value=$patchWorkshop}
)) {
    if ($null -ne $gate.Value -and $gate.Value -ne 1) { $failures.Add("emergency patch gate '$($gate.Name)' was not 1 (value=$($gate.Value))") }
}
if ($null -ne $patchSeverity -and $patchSeverity -le 0) { $failures.Add("patch breakdown severity was not positive (severity=$patchSeverity)") }
if ($null -ne $patchQuote -and $patchQuote -le 0) { $failures.Add("emergency patch quote was not positive (quote=$patchQuote)") }
if ($null -ne $patchCost -and $patchCost -le 0) { $failures.Add("emergency patch did not charge positive cash delta (delta=$patchCost)") }
if ($null -ne $patchQuote -and $null -ne $patchCost -and $patchQuote -ne $patchCost) { $failures.Add("emergency patch charge did not equal quote ($patchQuote vs $patchCost)") }
if ($null -ne $patchConditionBefore -and $patchConditionBefore -gt 0.20) { $failures.Add("patch probe did not stage a TowRecommended mechanical condition (condition=$patchConditionBefore)") }
if ($null -ne $patchTireBefore -and $patchTireBefore -gt 0.22) { $failures.Add("patch probe did not stage TowRecommended tire damage (tire=$patchTireBefore)") }
if ($null -ne $conditionAfterPatch -and $conditionAfterPatch -lt 0.299) { $failures.Add("patch did not restore condition limp-home floor (condition=$conditionAfterPatch)") }
if ($null -ne $tireAfterPatch -and $tireAfterPatch -lt 0.319) { $failures.Add("patch did not restore tire limp-home floor (tire=$tireAfterPatch)") }
if ($null -ne $fuelAfterPatch -and $fuelAfterPatch -lt 4.99) { $failures.Add("patch did not preserve/restore minimum roadside fuel (fuel=$fuelAfterPatch)") }
if ($null -ne $patchTimerBefore -and $null -ne $patchTimerAfter -and $patchTimerAfter -ge $patchTimerBefore) { $failures.Add("cargo timer did not continue during emergency patch ($patchTimerBefore -> $patchTimerAfter)") }
if ($null -ne $patchIntegrityBefore -and $null -ne $patchIntegrityAfter -and $patchIntegrityAfter -gt ($patchIntegrityBefore + 0.0001)) { $failures.Add("cargo integrity improved during emergency patch ($patchIntegrityBefore -> $patchIntegrityAfter)") }
if ($null -ne $cooldownWait -and $cooldownWait -lt 12.0) { $failures.Add("post-patch route did not survive the real recovery cooldown (waited=$cooldownWait)") }

$breakdown = Require-IntegerField $towRequest 'breakdown' 'BREAKDOWN_TOW_REQUEST'
$towRequested = Require-IntegerField $towRequest 'tow_requested' 'BREAKDOWN_TOW_REQUEST'
$severity = Require-NumberField $towRequest 'severity' 'BREAKDOWN_TOW_REQUEST'
$tireAfterDamage = Require-NumberField $towRequest 'tire_integrity' 'BREAKDOWN_TOW_REQUEST'
$timerBefore = Require-NumberField $towRequest 'timer_before' 'BREAKDOWN_TOW_REQUEST'
$integrityBefore = Require-NumberField $towRequest 'integrity_before' 'BREAKDOWN_TOW_REQUEST'
$towComplete = Require-IntegerField $tow 'tow_complete' 'TOW_COMPLETE'
$identity = Require-IntegerField $tow 'identity_preserved' 'TOW_COMPLETE'
$timerContinued = Require-IntegerField $tow 'timer_continued' 'TOW_COMPLETE'
$integrityStable = Require-IntegerField $tow 'integrity_not_improved' 'TOW_COMPLETE'
$damagePreserved = Require-IntegerField $tow 'damage_preserved' 'TOW_COMPLETE'
$towCost = Require-IntegerField $tow 'tow_cost_delta' 'TOW_COMPLETE'
$timerAfter = Require-NumberField $tow 'timer_after' 'TOW_COMPLETE'
$integrityAfter = Require-NumberField $tow 'integrity_after' 'TOW_COMPLETE'
$tireAfterTow = Require-NumberField $tow 'tire_after' 'TOW_COMPLETE'
$wrongRejected = Require-IntegerField $wrong 'wrong_vehicle_rejected' 'WRONG_VEHICLE_AFTER_TOW'
$hillPass = Require-IntegerField $hill 'hill' 'HILL_HANDOFF'
$finalPass = Require-IntegerField $final 'final' 'FINAL_HANDOFF'
$payoutDelta = Require-IntegerField $final 'payout_delta' 'FINAL_HANDOFF'
$cargoRunsDelta = Require-IntegerField $final 'cargo_runs_delta' 'FINAL_HANDOFF'
$reputationDelta = Require-IntegerField $final 'reputation_delta' 'FINAL_HANDOFF'
$authorityCleared = Require-IntegerField $final 'authority_cleared' 'FINAL_HANDOFF'
$finalSave = Require-IntegerField $persistence 'explicit_save' 'PERSISTENCE'

foreach ($gate in @(
    @{Name='breakdown';Value=$breakdown}, @{Name='tow_requested';Value=$towRequested}, @{Name='tow_complete';Value=$towComplete},
    @{Name='identity_preserved';Value=$identity}, @{Name='timer_continued';Value=$timerContinued},
    @{Name='integrity_not_improved';Value=$integrityStable}, @{Name='damage_preserved';Value=$damagePreserved},
    @{Name='wrong_vehicle_rejected';Value=$wrongRejected}, @{Name='hill';Value=$hillPass}, @{Name='final';Value=$finalPass},
    @{Name='authority_cleared';Value=$authorityCleared}, @{Name='final_save';Value=$finalSave}
)) {
    if ($null -ne $gate.Value -and $gate.Value -ne 1) { $failures.Add("breakdown gate '$($gate.Name)' was not 1 (value=$($gate.Value))") }
}
if ($null -ne $severity -and $severity -le 0) { $failures.Add("breakdown severity was not positive (severity=$severity)") }
if ($null -ne $tireAfterDamage -and $tireAfterDamage -gt 0.08) { $failures.Add("re-breakdown probe did not disable tires enough for native roadside recovery (tire=$tireAfterDamage)") }
if ($null -ne $towCost -and $towCost -le 0) { $failures.Add("roadside tow did not charge positive cash delta (delta=$towCost)") }
if ($null -ne $timerBefore -and $null -ne $timerAfter -and $timerAfter -ge $timerBefore) { $failures.Add("cargo timer did not continue during tow ($timerBefore -> $timerAfter)") }
if ($null -ne $integrityBefore -and $null -ne $integrityAfter -and $integrityAfter -gt ($integrityBefore + 0.0001)) { $failures.Add("cargo integrity improved during tow ($integrityBefore -> $integrityAfter)") }
if ($null -ne $tireAfterDamage -and $null -ne $tireAfterTow -and $tireAfterTow -gt ($tireAfterDamage + 0.001)) { $failures.Add("roadside tow repaired tire damage ($tireAfterDamage -> $tireAfterTow)") }
if ($null -ne $payoutDelta -and $payoutDelta -le 0) { $failures.Add("delivery payout did not increase cash after recovery (delta=$payoutDelta)") }
if ($null -ne $cargoRunsDelta -and $cargoRunsDelta -ne 1) { $failures.Add("cargo completion history delta was not exactly one (delta=$cargoRunsDelta)") }
if ($null -ne $reputationDelta -and $reputationDelta -le 0) { $failures.Add("logistics reputation did not increase (delta=$reputationDelta)") }

$completeFields = @(
    'accepted','pickup','patch_breakdown','patch_requested','patch_complete','patch_identity_preserved','patch_body_preserved',
    'patch_timer_continued','patch_integrity_not_improved','patch_workshop_required','breakdown','tow_requested','tow_complete',
    'identity_preserved','timer_continued','integrity_not_improved','damage_preserved','wrong_vehicle_rejected','hill','final','save','authority_cleared'
)
$completeValues = @{}
foreach ($field in $completeFields) {
    $completeValues[$field] = Require-IntegerField $complete $field 'COMPLETE'
    if ($null -ne $completeValues[$field] -and $completeValues[$field] -ne 1) { $failures.Add("completion marker gate '$field' was not 1 (value=$($completeValues[$field]))") }
}
$completePatchCost = Require-IntegerField $complete 'patch_cost_delta' 'COMPLETE'
$completeTowCost = Require-IntegerField $complete 'tow_cost_delta' 'COMPLETE'
$completePayout = Require-IntegerField $complete 'payout_delta' 'COMPLETE'
$completeRuns = Require-IntegerField $complete 'cargo_runs_delta' 'COMPLETE'
$completeReputation = Require-IntegerField $complete 'reputation_delta' 'COMPLETE'
$completeVehicle = Read-Token $complete 'vehicle'
if (-not $completeVehicle -or $completeVehicle -eq 'None') { $failures.Add('completion marker did not carry a stable cargo vehicle ID') }
if ($null -ne $patchCost -and $null -ne $completePatchCost -and $patchCost -ne $completePatchCost) { $failures.Add("patch cost disagrees between PATCH_COMPLETE and COMPLETE ($patchCost vs $completePatchCost)") }
if ($null -ne $towCost -and $null -ne $completeTowCost -and $towCost -ne $completeTowCost) { $failures.Add("tow cost disagrees between TOW_COMPLETE and COMPLETE ($towCost vs $completeTowCost)") }
if ($null -ne $payoutDelta -and $null -ne $completePayout -and $payoutDelta -ne $completePayout) { $failures.Add("payout disagrees between FINAL_HANDOFF and COMPLETE ($payoutDelta vs $completePayout)") }
if ($null -ne $cargoRunsDelta -and $null -ne $completeRuns -and $cargoRunsDelta -ne $completeRuns) { $failures.Add("cargo runs disagree between FINAL_HANDOFF and COMPLETE ($cargoRunsDelta vs $completeRuns)") }
if ($null -ne $reputationDelta -and $null -ne $completeReputation -and $reputationDelta -ne $completeReputation) { $failures.Add("reputation disagrees between FINAL_HANDOFF and COMPLETE ($reputationDelta vs $completeReputation)") }

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.farm-cargo-breakdown-runtime.v2'; result = $result; game = 'Grand Theft Tractor'
    version = $build.version; platform = $build.platform; git_sha = $build.git_sha
    route = 'Feed Depot -> TowRecommended native Mulebox -> paid emergency patch -> real cooldown -> deliberate re-breakdown -> paid roadside tow -> Hill Farm -> North Wood Yard'
    stable_vehicle_id = $completeVehicle
    emergency_patch_breakdown_proven = ($patchBreakdown -eq 1); player_authorized_patch = ($patchRequested -eq 1)
    patch_completed = ($patchCompleteGate -eq 1); patch_exact_vehicle_identity_preserved = ($patchIdentity -eq 1)
    patch_body_preserved = ($patchBody -eq 1); patch_timer_continued = ($patchTimer -eq 1)
    patch_cargo_integrity_not_improved = ($patchIntegrity -eq 1); patch_workshop_still_required = ($patchWorkshop -eq 1)
    patch_cost_delta = $patchCost; patch_condition_after = $conditionAfterPatch; patch_tire_after = $tireAfterPatch; patch_fuel_after = $fuelAfterPatch
    patch_cooldown_wait_seconds = $cooldownWait
    production_pre_patch_checkpoint = [bool]$prePatchCheckpoint; production_post_patch_identity_verification = [bool]$postPatchRecovery
    native_patch_request_marker = [bool]$nativePatchRequested; native_patch_complete_marker = [bool]$nativePatchComplete
    native_breakdown_proven = ($breakdown -eq 1); player_authorized_tow = ($towRequested -eq 1)
    tow_completed = ($towComplete -eq 1); exact_vehicle_identity_preserved = ($identity -eq 1)
    timer_continued = ($timerContinued -eq 1); cargo_integrity_not_improved = ($integrityStable -eq 1)
    damage_preserved = ($damagePreserved -eq 1); wrong_vehicle_rejected_after_tow = ($wrongRejected -eq 1)
    tow_cost_delta = $towCost; payout_delta = $payoutDelta; cargo_completed_runs_delta = $cargoRunsDelta
    logistics_reputation_delta = $reputationDelta; authority_cleared = ($authorityCleared -eq 1); final_save = ($finalSave -eq 1)
    production_pre_tow_checkpoint = [bool]$preTowCheckpoint; production_post_tow_identity_verification = [bool]$postTowRecovery
    native_tow_request_marker = [bool]$nativeTowRequested; native_tow_complete_marker = [bool]$nativeTowComplete
    diagnostic_failure_count = $diagnostics.Count; failures = @($failures); evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath
if ($result -ne 'PASS') { Write-Error "Farm Cargo patch+breakdown packaged runtime exercise failed: $($failures -join '; ')"; exit 9 }
Write-Host "[GTT] Farm Cargo emergency-patch + re-breakdown/tow runtime: PASS (vehicle=$completeVehicle, patch=$patchCost, tow=$towCost, payout=+$payoutDelta, rep=+$reputationDelta)."
Write-Host "[GTT] Evidence: $OutputPath"
