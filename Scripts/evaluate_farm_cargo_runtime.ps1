param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)
$OutputPath = Join-Path $PackageDirectory 'FARM_CARGO_RUNTIME.json'
$BuildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$SmokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'

foreach ($path in @($BuildPath, $SmokePath, $RuntimeLog)) {
    if (-not (Test-Path $path)) { throw "Required Farm Cargo runtime evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

function Find-Phase([string]$Phase, [string]$Result = 'PASS') {
    return @($lines | Where-Object { $_ -match "FARM_CARGO_RUNTIME\s+phase=$([regex]::Escape($Phase))\s+result=$([regex]::Escape($Result))(?:\s|$)" }) | Select-Object -Last 1
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
if (-not (@($smoke.launch_arguments) -contains '-GTTFarmCargoRuntimeScenario')) {
    $failures.Add('runtime smoke did not explicitly enable -GTTFarmCargoRuntimeScenario')
}

$begin = @($lines | Where-Object { $_ -match 'FARM_CARGO_RUNTIME_BEGIN\s+version=1\s+route=feed-hill-wood' }) | Select-Object -Last 1
$prepare = Find-Phase 'PREPARE'
$accept = Find-Phase 'ACCEPT'
$pickup = Find-Phase 'PICKUP'
$wrongVehicle = Find-Phase 'WRONG_VEHICLE'
$hill = Find-Phase 'HILL_HANDOFF'
$final = Find-Phase 'FINAL_HANDOFF'
$persistence = Find-Phase 'PERSISTENCE'
$complete = @($lines | Where-Object { $_ -match 'FARM_CARGO_RUNTIME_COMPLETE\s+result=PASS\s+route=feed-hill-wood' }) | Select-Object -Last 1
$diagnostics = @($lines | Where-Object { $_ -match 'FARM_CARGO_RUNTIME\s+phase=DIAGNOSTIC\s+result=FAIL' })

if (-not $begin) { $failures.Add('Farm Cargo runtime begin marker is missing') }
if (-not $prepare) { $failures.Add('Tier-2 stock-backed Farm Cargo preparation was not proven') }
if (-not $accept) { $failures.Add('real Farm Cargo contract acceptance was not proven') }
if (-not $pickup) { $failures.Add('physical cargo pickup/binding was not proven') }
if (-not $wrongVehicle) { $failures.Add('wrong-vehicle handoff rejection was not proven') }
if (-not $hill) { $failures.Add('Hill Farm relay handoff was not proven') }
if (-not $final) { $failures.Add('North Wood Yard final handoff was not proven') }
if (-not $persistence) { $failures.Add('post-delivery SaveProgress evidence was not proven') }
if (-not $complete) { $failures.Add('Farm Cargo route did not complete with PASS') }
if ($diagnostics.Count -gt 0) { $failures.Add("scenario logged $($diagnostics.Count) diagnostic failure(s)") }

$routeTier = Require-IntegerField $prepare 'active_order_tier' 'PREPARE'
$sameVehicle = Require-IntegerField $hill 'same_vehicle' 'HILL_HANDOFF'
$payoutDelta = Require-IntegerField $final 'payout_delta' 'FINAL_HANDOFF'
$cargoRunsDelta = Require-IntegerField $final 'cargo_runs_delta' 'FINAL_HANDOFF'
$reputationDelta = Require-IntegerField $final 'reputation_delta' 'FINAL_HANDOFF'
$authorityCleared = Require-IntegerField $final 'authority_cleared' 'FINAL_HANDOFF'
$savePass = Require-IntegerField $persistence 'explicit_save' 'PERSISTENCE'

$completeAccepted = Require-IntegerField $complete 'accepted' 'COMPLETE'
$completePickup = Require-IntegerField $complete 'pickup' 'COMPLETE'
$completeWrongVehicle = Require-IntegerField $complete 'wrong_vehicle_rejected' 'COMPLETE'
$completeHill = Require-IntegerField $complete 'hill' 'COMPLETE'
$completeFinal = Require-IntegerField $complete 'final' 'COMPLETE'
$completeSameVehicle = Require-IntegerField $complete 'same_vehicle' 'COMPLETE'
$completePayout = Require-IntegerField $complete 'payout_delta' 'COMPLETE'
$completeCargoRuns = Require-IntegerField $complete 'cargo_runs_delta' 'COMPLETE'
$completeReputation = Require-IntegerField $complete 'reputation_delta' 'COMPLETE'
$completeSave = Require-IntegerField $complete 'save' 'COMPLETE'
$completeAuthority = Require-IntegerField $complete 'authority_cleared' 'COMPLETE'

if ($null -ne $routeTier -and $routeTier -lt 2) { $failures.Add("evidence route did not reach tier 2 (active_order_tier=$routeTier)") }
if ($null -ne $sameVehicle -and $sameVehicle -ne 1) { $failures.Add('Hill handoff did not retain exact physical vehicle identity') }
if ($null -ne $payoutDelta -and $payoutDelta -le 0) { $failures.Add("delivery payout did not increase cash (delta=$payoutDelta)") }
if ($null -ne $cargoRunsDelta -and $cargoRunsDelta -ne 1) { $failures.Add("cargo completion history delta was not exactly one (delta=$cargoRunsDelta)") }
if ($null -ne $reputationDelta -and $reputationDelta -le 0) { $failures.Add("logistics reputation did not increase (delta=$reputationDelta)") }
if ($null -ne $authorityCleared -and $authorityCleared -ne 1) { $failures.Add('physical cargo authority did not clear after final handoff') }
if ($null -ne $savePass -and $savePass -ne 1) { $failures.Add('explicit post-delivery SaveProgress check failed') }

foreach ($gate in @(
    @{ Name = 'accepted'; Value = $completeAccepted },
    @{ Name = 'pickup'; Value = $completePickup },
    @{ Name = 'wrong_vehicle_rejected'; Value = $completeWrongVehicle },
    @{ Name = 'hill'; Value = $completeHill },
    @{ Name = 'final'; Value = $completeFinal },
    @{ Name = 'same_vehicle'; Value = $completeSameVehicle },
    @{ Name = 'save'; Value = $completeSave },
    @{ Name = 'authority_cleared'; Value = $completeAuthority }
)) {
    if ($null -ne $gate.Value -and $gate.Value -ne 1) {
        $failures.Add("completion marker gate '$($gate.Name)' was not 1 (value=$($gate.Value))")
    }
}
if ($null -ne $completePayout -and $completePayout -le 0) { $failures.Add("completion marker payout was not positive (delta=$completePayout)") }
if ($null -ne $completeCargoRuns -and $completeCargoRuns -ne 1) { $failures.Add("completion marker cargo run delta was not exactly one (delta=$completeCargoRuns)") }
if ($null -ne $completeReputation -and $completeReputation -le 0) { $failures.Add("completion marker reputation was not positive (delta=$completeReputation)") }

if ($null -ne $payoutDelta -and $null -ne $completePayout -and $payoutDelta -ne $completePayout) {
    $failures.Add("payout evidence disagrees between FINAL_HANDOFF and COMPLETE ($payoutDelta vs $completePayout)")
}
if ($null -ne $cargoRunsDelta -and $null -ne $completeCargoRuns -and $cargoRunsDelta -ne $completeCargoRuns) {
    $failures.Add("cargo run evidence disagrees between FINAL_HANDOFF and COMPLETE ($cargoRunsDelta vs $completeCargoRuns)")
}
if ($null -ne $reputationDelta -and $null -ne $completeReputation -and $reputationDelta -ne $completeReputation) {
    $failures.Add("reputation evidence disagrees between FINAL_HANDOFF and COMPLETE ($reputationDelta vs $completeReputation)")
}
if ($null -ne $sameVehicle -and $null -ne $completeSameVehicle -and $sameVehicle -ne $completeSameVehicle) {
    $failures.Add("same-vehicle evidence disagrees between HILL_HANDOFF and COMPLETE ($sameVehicle vs $completeSameVehicle)")
}
if ($null -ne $savePass -and $null -ne $completeSave -and $savePass -ne $completeSave) {
    $failures.Add("save evidence disagrees between PERSISTENCE and COMPLETE ($savePass vs $completeSave)")
}
if ($null -ne $authorityCleared -and $null -ne $completeAuthority -and $authorityCleared -ne $completeAuthority) {
    $failures.Add("authority cleanup evidence disagrees between FINAL_HANDOFF and COMPLETE ($authorityCleared vs $completeAuthority)")
}

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.farm-cargo-runtime.v1'
    result = $result
    game = 'Grand Theft Tractor'
    version = $build.version
    platform = $build.platform
    git_sha = $build.git_sha
    route = 'Feed Depot -> Hill Farm -> North Wood Yard'
    active_order_tier = $routeTier
    exact_vehicle_bound = ($completePickup -eq 1)
    wrong_vehicle_rejected = ($completeWrongVehicle -eq 1)
    same_vehicle_hill_to_final = ($sameVehicle -eq 1 -and $completeSameVehicle -eq 1)
    payout_delta = $payoutDelta
    cargo_completed_runs_delta = $cargoRunsDelta
    logistics_reputation_delta = $reputationDelta
    post_delivery_save = ($savePass -eq 1 -and $completeSave -eq 1)
    authority_cleared = ($authorityCleared -eq 1 -and $completeAuthority -eq 1)
    diagnostic_failure_count = $diagnostics.Count
    failures = @($failures)
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Error "Farm Cargo packaged runtime exercise failed: $($failures -join '; ')"
    exit 6
}

Write-Host "[GTT] Farm Cargo packaged runtime exercise: PASS (tier=$routeTier, payout=+$payoutDelta, rep=+$reputationDelta)."
Write-Host "[GTT] Evidence: $OutputPath"
