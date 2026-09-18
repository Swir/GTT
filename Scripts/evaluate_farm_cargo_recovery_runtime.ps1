param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)
$OutputPath = Join-Path $PackageDirectory 'FARM_CARGO_RECOVERY_RUNTIME.json'
$BuildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$SmokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'

foreach ($path in @($BuildPath, $SmokePath, $RuntimeLog)) {
    if (-not (Test-Path $path)) { throw "Required Farm Cargo recovery evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

function Find-Phase([string]$Phase, [string]$Result = 'PASS') {
    return @($lines | Where-Object { $_ -match "FARM_CARGO_RECOVERY_RUNTIME\s+phase=$([regex]::Escape($Phase))\s+result=$([regex]::Escape($Result))(?:\s|$)" }) | Select-Object -Last 1
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
    if ($null -eq $value) {
        $failures.Add("$Context missing required integer field '$Key'")
        return $null
    }
    return $value
}

if ($build.platform -ne 'Win64') { $failures.Add('build platform is not Win64') }
if ($smoke.result -ne 'PASS') { $failures.Add('packaged runtime smoke did not PASS') }
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    $failures.Add("build SHA mismatch package=$($build.git_sha) expected=$ExpectedGitSha")
}
if (-not (@($smoke.launch_arguments) -contains '-GTTFarmCargoRecoveryScenario')) {
    $failures.Add('runtime smoke did not explicitly enable -GTTFarmCargoRecoveryScenario')
}

$begin = @($lines | Where-Object { $_ -match 'FARM_CARGO_RECOVERY_RUNTIME_BEGIN\s+version=1\s+route=feed-hill-wood' }) | Select-Object -Last 1
$prepare = Find-Phase 'PREPARE'
$accept = Find-Phase 'ACCEPT'
$pickup = Find-Phase 'PICKUP'
$saveLoaded = Find-Phase 'SAVE_LOADED'
$reloadLoaded = Find-Phase 'RELOAD_LOADED'
$wrongVehicle = Find-Phase 'WRONG_VEHICLE_AFTER_RELOAD'
$hill = Find-Phase 'HILL_HANDOFF'
$saveRelay = Find-Phase 'SAVE_RELAY'
$reloadRelay = Find-Phase 'RELOAD_RELAY'
$final = Find-Phase 'FINAL_HANDOFF'
$completionReload = Find-Phase 'COMPLETION_RELOAD'
$persistence = Find-Phase 'PERSISTENCE'
$complete = @($lines | Where-Object { $_ -match 'FARM_CARGO_RECOVERY_RUNTIME_COMPLETE\s+result=PASS\s+route=feed-hill-wood' }) | Select-Object -Last 1
$diagnostics = @($lines | Where-Object { $_ -match 'FARM_CARGO_RECOVERY_RUNTIME\s+phase=DIAGNOSTIC\s+result=FAIL' })

$requiredPhases = [ordered]@{
    PREPARE = $prepare
    ACCEPT = $accept
    PICKUP = $pickup
    SAVE_LOADED = $saveLoaded
    RELOAD_LOADED = $reloadLoaded
    WRONG_VEHICLE_AFTER_RELOAD = $wrongVehicle
    HILL_HANDOFF = $hill
    SAVE_RELAY = $saveRelay
    RELOAD_RELAY = $reloadRelay
    FINAL_HANDOFF = $final
    COMPLETION_RELOAD = $completionReload
    PERSISTENCE = $persistence
}
if (-not $begin) { $failures.Add('Farm Cargo recovery runtime begin marker is missing') }
foreach ($entry in $requiredPhases.GetEnumerator()) {
    if (-not $entry.Value) { $failures.Add("required recovery phase $($entry.Key) was not proven") }
}
if (-not $complete) { $failures.Add('Farm Cargo recovery route did not complete with PASS') }
if ($diagnostics.Count -gt 0) { $failures.Add("scenario logged $($diagnostics.Count) diagnostic failure(s)") }

$identityUnique = Require-IntegerField $prepare 'identity_unique' 'PREPARE'
$loadedSave = Require-IntegerField $saveLoaded 'explicit_save' 'SAVE_LOADED'
$loadedStage = Require-IntegerField $reloadLoaded 'stage_restored' 'RELOAD_LOADED'
$loadedId = Require-IntegerField $reloadLoaded 'id_restored' 'RELOAD_LOADED'
$actorRebound = Require-IntegerField $reloadLoaded 'actor_rebound' 'RELOAD_LOADED'
$loadedTimer = Require-IntegerField $reloadLoaded 'timer_restored' 'RELOAD_LOADED'
$loadedIntegrity = Require-IntegerField $reloadLoaded 'integrity_restored' 'RELOAD_LOADED'
$loadedStock = Require-IntegerField $reloadLoaded 'stock_stable' 'RELOAD_LOADED'
$wrongRejected = Require-IntegerField $wrongVehicle 'wrong_vehicle_rejected' 'WRONG_VEHICLE_AFTER_RELOAD'
$relaySave = Require-IntegerField $saveRelay 'explicit_save' 'SAVE_RELAY'
$relayStage = Require-IntegerField $reloadRelay 'stage_restored' 'RELOAD_RELAY'
$relayId = Require-IntegerField $reloadRelay 'id_restored' 'RELOAD_RELAY'
$relaySame = Require-IntegerField $reloadRelay 'relay_same_vehicle' 'RELOAD_RELAY'
$relayTimer = Require-IntegerField $reloadRelay 'timer_restored' 'RELOAD_RELAY'
$relayIntegrity = Require-IntegerField $reloadRelay 'integrity_restored' 'RELOAD_RELAY'
$relayStock = Require-IntegerField $reloadRelay 'stock_stable' 'RELOAD_RELAY'
$finalPass = Require-IntegerField $final 'final' 'FINAL_HANDOFF'
$payoutDelta = Require-IntegerField $final 'payout_delta' 'FINAL_HANDOFF'
$cargoRunsDelta = Require-IntegerField $final 'cargo_runs_delta' 'FINAL_HANDOFF'
$reputationDelta = Require-IntegerField $final 'reputation_delta' 'FINAL_HANDOFF'
$finalAuthority = Require-IntegerField $final 'authority_cleared' 'FINAL_HANDOFF'
$completionStable = Require-IntegerField $completionReload 'completion_reload_stable' 'COMPLETION_RELOAD'
$completionAuthority = Require-IntegerField $completionReload 'authority_cleared' 'COMPLETION_RELOAD'
$finalSave = Require-IntegerField $persistence 'explicit_save' 'PERSISTENCE'

foreach ($gate in @(
    @{ Name='identity_unique'; Value=$identityUnique },
    @{ Name='loaded_save'; Value=$loadedSave },
    @{ Name='loaded_stage_restored'; Value=$loadedStage },
    @{ Name='loaded_id_restored'; Value=$loadedId },
    @{ Name='actor_rebound'; Value=$actorRebound },
    @{ Name='loaded_timer_restored'; Value=$loadedTimer },
    @{ Name='loaded_integrity_restored'; Value=$loadedIntegrity },
    @{ Name='loaded_stock_stable'; Value=$loadedStock },
    @{ Name='wrong_vehicle_after_reload'; Value=$wrongRejected },
    @{ Name='relay_save'; Value=$relaySave },
    @{ Name='relay_stage_restored'; Value=$relayStage },
    @{ Name='relay_id_restored'; Value=$relayId },
    @{ Name='relay_same_vehicle'; Value=$relaySame },
    @{ Name='relay_timer_restored'; Value=$relayTimer },
    @{ Name='relay_integrity_restored'; Value=$relayIntegrity },
    @{ Name='relay_stock_stable'; Value=$relayStock },
    @{ Name='final'; Value=$finalPass },
    @{ Name='final_authority_cleared'; Value=$finalAuthority },
    @{ Name='completion_reload_stable'; Value=$completionStable },
    @{ Name='completion_authority_cleared'; Value=$completionAuthority },
    @{ Name='final_save'; Value=$finalSave }
)) {
    if ($null -ne $gate.Value -and $gate.Value -ne 1) {
        $failures.Add("recovery gate '$($gate.Name)' was not 1 (value=$($gate.Value))")
    }
}
if ($null -ne $payoutDelta -and $payoutDelta -le 0) { $failures.Add("delivery payout did not increase cash (delta=$payoutDelta)") }
if ($null -ne $cargoRunsDelta -and $cargoRunsDelta -ne 1) { $failures.Add("cargo completion history delta was not exactly one (delta=$cargoRunsDelta)") }
if ($null -ne $reputationDelta -and $reputationDelta -le 0) { $failures.Add("logistics reputation did not increase (delta=$reputationDelta)") }

$completeFields = @(
    'identity_unique','accepted','pickup','loaded_save','loaded_reload','actor_rebound',
    'wrong_vehicle_after_reload','hill','relay_save','relay_reload','relay_same_vehicle',
    'final','completion_reload','final_save','authority_cleared'
)
$completeValues = @{}
foreach ($field in $completeFields) {
    $completeValues[$field] = Require-IntegerField $complete $field 'COMPLETE'
    if ($null -ne $completeValues[$field] -and $completeValues[$field] -ne 1) {
        $failures.Add("completion marker gate '$field' was not 1 (value=$($completeValues[$field]))")
    }
}
$completePayout = Require-IntegerField $complete 'payout_delta' 'COMPLETE'
$completeRuns = Require-IntegerField $complete 'cargo_runs_delta' 'COMPLETE'
$completeReputation = Require-IntegerField $complete 'reputation_delta' 'COMPLETE'
$completeVehicle = Read-Token $complete 'vehicle'
if (-not $completeVehicle -or $completeVehicle -eq 'None') { $failures.Add('completion marker did not carry a stable cargo vehicle ID') }

if ($null -ne $payoutDelta -and $null -ne $completePayout -and $payoutDelta -ne $completePayout) {
    $failures.Add("payout evidence disagrees between FINAL_HANDOFF and COMPLETE ($payoutDelta vs $completePayout)")
}
if ($null -ne $cargoRunsDelta -and $null -ne $completeRuns -and $cargoRunsDelta -ne $completeRuns) {
    $failures.Add("cargo-run evidence disagrees between FINAL_HANDOFF and COMPLETE ($cargoRunsDelta vs $completeRuns)")
}
if ($null -ne $reputationDelta -and $null -ne $completeReputation -and $reputationDelta -ne $completeReputation) {
    $failures.Add("reputation evidence disagrees between FINAL_HANDOFF and COMPLETE ($reputationDelta vs $completeReputation)")
}

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.farm-cargo-recovery-runtime.v1'
    result = $result
    game = 'Grand Theft Tractor'
    version = $build.version
    platform = $build.platform
    git_sha = $build.git_sha
    route = 'Feed Depot -> save/load -> Hill Farm -> save/load -> North Wood Yard'
    same_model_vehicle_identity_unique = ($identityUnique -eq 1)
    loaded_checkpoint_saved = ($loadedSave -eq 1)
    loaded_stage_restored = ($loadedStage -eq 1)
    loaded_vehicle_id_restored = ($loadedId -eq 1)
    recreated_actor_rebound = ($actorRebound -eq 1)
    loaded_timer_restored = ($loadedTimer -eq 1)
    loaded_integrity_restored = ($loadedIntegrity -eq 1)
    loaded_stock_stable = ($loadedStock -eq 1)
    wrong_vehicle_rejected_after_reload = ($wrongRejected -eq 1)
    relay_checkpoint_saved = ($relaySave -eq 1)
    relay_stage_restored = ($relayStage -eq 1)
    relay_vehicle_id_restored = ($relayId -eq 1)
    relay_same_vehicle = ($relaySame -eq 1)
    relay_timer_restored = ($relayTimer -eq 1)
    relay_integrity_restored = ($relayIntegrity -eq 1)
    relay_stock_stable = ($relayStock -eq 1)
    payout_delta = $payoutDelta
    cargo_completed_runs_delta = $cargoRunsDelta
    logistics_reputation_delta = $reputationDelta
    completion_reload_stable = ($completionStable -eq 1)
    authority_cleared = ($completionAuthority -eq 1 -and $completeValues['authority_cleared'] -eq 1)
    final_save = ($finalSave -eq 1)
    stable_vehicle_id = $completeVehicle
    diagnostic_failure_count = $diagnostics.Count
    failures = @($failures)
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Error "Farm Cargo recovery packaged runtime exercise failed: $($failures -join '; ')"
    exit 7
}

Write-Host "[GTT] Farm Cargo save/load recovery runtime: PASS (vehicle=$completeVehicle, payout=+$payoutDelta, rep=+$reputationDelta)."
Write-Host "[GTT] Evidence: $OutputPath"