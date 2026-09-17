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

$routeTier = Read-Integer $prepare 'active_order_tier'
$wrongVehiclePass = if ($wrongVehicle) { 1 } else { 0 }
$sameVehicle = Read-Integer $hill 'same_vehicle'
$payoutDelta = Read-Integer $final 'payout_delta'
$cargoRunsDelta = Read-Integer $final 'cargo_runs_delta'
$reputationDelta = Read-Integer $final 'reputation_delta'
$authorityCleared = Read-Integer $final 'authority_cleared'
$savePass = Read-Integer $persistence 'explicit_save'
$completeWrongVehicle = Read-Integer $complete 'wrong_vehicle_rejected'
$completeSameVehicle = Read-Integer $complete 'same_vehicle'
$completeAuthority = Read-Integer $complete 'authority_cleared'

if ($null -ne $routeTier -and $routeTier -lt 2) { $failures.Add("evidence route did not reach tier 2 (active_order_tier=$routeTier)") }
if ($wrongVehiclePass -ne 1) { $failures.Add('wrong-vehicle probe did not PASS') }
if ($null -ne $sameVehicle -and $sameVehicle -ne 1) { $failures.Add('Hill handoff did not retain exact physical vehicle identity') }
if ($null -ne $payoutDelta -and $payoutDelta -le 0) { $failures.Add("delivery payout did not increase cash (delta=$payoutDelta)") }
if ($null -ne $cargoRunsDelta -and $cargoRunsDelta -ne 1) { $failures.Add("cargo completion history delta was not exactly one (delta=$cargoRunsDelta)") }
if ($null -ne $reputationDelta -and $reputationDelta -le 0) { $failures.Add("logistics reputation did not increase (delta=$reputationDelta)") }
if ($null -ne $authorityCleared -and $authorityCleared -ne 1) { $failures.Add('physical cargo authority did not clear after final handoff') }
if ($null -ne $savePass -and $savePass -ne 1) { $failures.Add('explicit post-delivery SaveProgress check failed') }
if ($null -ne $completeWrongVehicle -and $completeWrongVehicle -ne 1) { $failures.Add('completion marker lost wrong-vehicle rejection proof') }
if ($null -ne $completeSameVehicle -and $completeSameVehicle -ne 1) { $failures.Add('completion marker lost same-vehicle continuity proof') }
if ($null -ne $completeAuthority -and $completeAuthority -ne 1) { $failures.Add('completion marker did not prove authority cleanup') }

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
    exact_vehicle_bound = [bool]$pickup
    wrong_vehicle_rejected = [bool]$wrongVehicle
    same_vehicle_hill_to_final = ($sameVehicle -eq 1)
    payout_delta = $payoutDelta
    cargo_completed_runs_delta = $cargoRunsDelta
    logistics_reputation_delta = $reputationDelta
    post_delivery_save = ($savePass -eq 1)
    authority_cleared = ($authorityCleared -eq 1)
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
