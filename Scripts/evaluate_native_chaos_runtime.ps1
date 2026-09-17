param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)
$OutputPath = Join-Path $PackageDirectory 'NATIVE_CHAOS_RUNTIME.json'
$BuildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$SmokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'
$ScenarioPath = Join-Path $PackageDirectory 'DEMO_SCENARIO.json'

foreach ($path in @($BuildPath, $SmokePath, $ScenarioPath, $RuntimeLog)) {
    if (-not (Test-Path $path)) { throw "Required native runtime evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$scenario = Get-Content -Raw $ScenarioPath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)

function Read-Number([string]$Line, [string]$Key) {
    $match = [regex]::Match($Line, "(?:^|\s)$([regex]::Escape($Key))=(-?[0-9]+(?:\.[0-9]+)?)")
    if (-not $match.Success) { return $null }
    return [double]::Parse($match.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
}

function Read-Integer([string]$Line, [string]$Key) {
    $value = Read-Number $Line $Key
    if ($null -eq $value) { return $null }
    return [int]$value
}

function Has-Token([string]$Line, [string]$Key, [string]$Value) {
    return $Line -match "(?:^|\s)$([regex]::Escape($Key))=$([regex]::Escape($Value))(?:\s|$)"
}

$failures = [System.Collections.Generic.List[string]]::new()
if ($build.platform -ne 'Win64') { $failures.Add('build platform is not Win64') }
if ($smoke.result -ne 'PASS') { $failures.Add('packaged runtime smoke did not PASS') }
if ($scenario.result -ne 'PASS') { $failures.Add('deterministic demo scenario did not PASS') }
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    $failures.Add("build SHA mismatch package=$($build.git_sha) expected=$ExpectedGitSha")
}

$telemetry = @($lines | Where-Object { $_ -match 'NATIVE_FIELDMASTER_RUNTIME_TELEMETRY\s+vehicle=RustyFieldmaster60' })
$physicsAccepted = @($lines | Where-Object { $_ -match 'NATIVE_PHYSICS_EVIDENCE\s+vehicle=RustyFieldmaster60.*accepted=YES' }).Count -gt 0
$wheelSetupObserved = @($lines | Where-Object { $_ -match 'NATIVE_WHEEL_SETUP_EVIDENCE\s+vehicle=RustyFieldmaster60' }).Count -gt 0
$nativeReady = @($lines | Where-Object { $_ -match 'NATIVE_CHAOS_SMOKE_READY.*vehicle=Fieldmaster' }).Count -gt 0
$fieldmasterMotion = @($lines | Where-Object { $_ -match 'DEMO_SCENARIO_STEP\s+step=FIELDMASTER_MOTION\s+result=PASS' }).Count -gt 0
$fieldmasterControl = @($lines | Where-Object { $_ -match 'DEMO_SCENARIO_STEP\s+step=FIELDMASTER_CONTROL\s+result=PASS' }).Count -gt 0
$physicsFallback = @($lines | Where-Object { $_ -match 'NATIVE_PHYSICS_FALLBACK\s+vehicle=RustyFieldmaster60' }).Count -gt 0
$automaticGearboxEvidence = @($lines | Where-Object { $_ -match 'NATIVE_AUTOMATIC_GEARBOX_EVIDENCE\s+vehicle=RustyFieldmaster60' })
$automaticShiftEvents = @($lines | Where-Object { $_ -match 'NATIVE_AUTOMATIC_GEAR_SHIFT\s+vehicle=RustyFieldmaster60' })
$directionShiftCommits = @($lines | Where-Object { $_ -match 'NATIVE_DIRECTION_SHIFT_COMMIT\s+vehicle=RustyFieldmaster60' })

$maxSpeed = 0.0
$maxSignedSpeed = 0.0
$maxContacts = 0
$maxValidWheels = 0
$maxSuspensionSamples = 0
$maxSlipMagnitude = 0.0
$maxSlipAngle = 0.0
$maxAxleImbalance = 0.0
$minTractionAuthority = 1.0
$maxForwardGear = 0
$maxConfiguredForwardGears = 0
$automaticGearSamples = 0
$observedGears = [System.Collections.Generic.HashSet[int]]::new()
$movementActiveSamples = 0
$suspensionReadySamples = 0
$commandSamples = 0
$trailerSamples = 0
$loadedTrailerSamples = 0

foreach ($line in $telemetry) {
    $speed = Read-Number $line 'speed_kmh'
    $signedSpeed = Read-Number $line 'signed_speed_kmh'
    $gear = Read-Integer $line 'current_gear'
    $forwardGears = Read-Integer $line 'forward_gears'
    $throttle = Read-Number $line 'throttle'
    $brake = Read-Number $line 'brake'
    $steer = Read-Number $line 'steer'
    $valid = Read-Integer $line 'valid_wheels'
    $contacts = Read-Integer $line 'contacts'
    $suspensionSamples = Read-Integer $line 'suspension_samples'
    $slipMagnitude = Read-Number $line 'max_slip_magnitude'
    $slipAngle = Read-Number $line 'max_slip_angle'
    $imbalance = Read-Number $line 'axle_imbalance'
    $authority = Read-Number $line 'traction_authority'
    $towLoad = Read-Number $line 'tow_load'

    if ($null -ne $speed) { $maxSpeed = [Math]::Max($maxSpeed, [Math]::Abs($speed)) }
    if ($null -ne $signedSpeed) { $maxSignedSpeed = [Math]::Max($maxSignedSpeed, [Math]::Abs($signedSpeed)) }
    if ($null -ne $gear) {
        [void]$observedGears.Add($gear)
        if ($gear -gt 0) { $maxForwardGear = [Math]::Max($maxForwardGear, $gear) }
    }
    if ($null -ne $forwardGears) { $maxConfiguredForwardGears = [Math]::Max($maxConfiguredForwardGears, $forwardGears) }
    if (Has-Token $line 'automatic_gears' 'YES') { ++$automaticGearSamples }
    if ($null -ne $valid) { $maxValidWheels = [Math]::Max($maxValidWheels, $valid) }
    if ($null -ne $contacts) { $maxContacts = [Math]::Max($maxContacts, $contacts) }
    if ($null -ne $suspensionSamples) { $maxSuspensionSamples = [Math]::Max($maxSuspensionSamples, $suspensionSamples) }
    if ($null -ne $slipMagnitude) { $maxSlipMagnitude = [Math]::Max($maxSlipMagnitude, [Math]::Abs($slipMagnitude)) }
    if ($null -ne $slipAngle) { $maxSlipAngle = [Math]::Max($maxSlipAngle, [Math]::Abs($slipAngle)) }
    if ($null -ne $imbalance) { $maxAxleImbalance = [Math]::Max($maxAxleImbalance, [Math]::Abs($imbalance)) }
    if ($null -ne $authority) { $minTractionAuthority = [Math]::Min($minTractionAuthority, $authority) }
    if (Has-Token $line 'movement' 'ACTIVE') { ++$movementActiveSamples }
    if (Has-Token $line 'suspension_ready' 'YES') { ++$suspensionReadySamples }
    if (($null -ne $throttle -and [Math]::Abs($throttle) -ge 0.05) -or
        ($null -ne $brake -and [Math]::Abs($brake) -ge 0.05) -or
        ($null -ne $steer -and [Math]::Abs($steer) -ge 0.05)) { ++$commandSamples }
    if (Has-Token $line 'trailer' 'ATTACHED') {
        ++$trailerSamples
        if ($null -ne $towLoad -and $towLoad -gt 0.20) { ++$loadedTrailerSamples }
    }
}

$unsafeDirectionShiftCommits = 0
$maxDirectionShiftCommitSpeedKmh = 0.0
foreach ($line in $directionShiftCommits) {
    $shiftSpeed = Read-Number $line 'speed_kmh'
    if ($null -eq $shiftSpeed) { continue }
    $absoluteShiftSpeed = [Math]::Abs($shiftSpeed)
    $maxDirectionShiftCommitSpeedKmh = [Math]::Max($maxDirectionShiftCommitSpeedKmh, $absoluteShiftSpeed)
    if ($absoluteShiftSpeed -gt 3.75) { ++$unsafeDirectionShiftCommits }
}

if ($telemetry.Count -lt 2) { $failures.Add("native telemetry has fewer than 2 samples (found $($telemetry.Count))") }
if (-not $physicsAccepted) { $failures.Add('accepted Native Physics evidence is missing') }
if (-not $wheelSetupObserved) { $failures.Add('Native wheel setup evidence is missing') }
if (-not $nativeReady) { $failures.Add('Fieldmaster Native Chaos smoke-ready marker is missing') }
if (-not $fieldmasterMotion) { $failures.Add('deterministic scenario did not prove FIELDMASTER_MOTION') }
if (-not $fieldmasterControl) { $failures.Add('deterministic scenario did not prove FIELDMASTER_CONTROL') }
if ($physicsFallback) { $failures.Add('Native Physics fallback occurred during runtime evidence') }
if ($movementActiveSamples -lt 2) { $failures.Add('fewer than 2 telemetry samples had active Chaos movement') }
if ($maxValidWheels -lt 4) { $failures.Add("runtime never observed all 4 valid Chaos wheels (max=$maxValidWheels)") }
if ($maxContacts -lt 2) { $failures.Add("runtime never observed at least 2 wheel contacts (max=$maxContacts)") }
if ($maxSuspensionSamples -lt 4 -or $suspensionReadySamples -lt 1) { $failures.Add('runtime never observed complete 4-wheel suspension telemetry') }
if ($maxSpeed -lt 0.35) { $failures.Add("runtime did not prove Fieldmaster motion (max_speed_kmh=$maxSpeed)") }
if ($commandSamples -lt 1) { $failures.Add('runtime did not capture any non-zero throttle/brake/steer command') }
if (@($observedGears | Where-Object { $_ -ne 0 }).Count -lt 1) { $failures.Add('runtime did not observe a non-neutral Chaos gear') }
if ($automaticGearSamples -lt 2) { $failures.Add("runtime did not prove automatic transmission remained enabled (samples=$automaticGearSamples)") }
if ($maxConfiguredForwardGears -lt 2) { $failures.Add("runtime did not expose a multi-gear forward transmission (forward_gears=$maxConfiguredForwardGears)") }
if ($unsafeDirectionShiftCommits -gt 0) { $failures.Add("direction shift committed above safe release speed (unsafe_commits=$unsafeDirectionShiftCommits max_speed_kmh=$maxDirectionShiftCommitSpeedKmh)") }

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.native-chaos-runtime.v1'
    result = $result
    game = 'Grand Theft Tractor'
    version = $build.version
    platform = $build.platform
    git_sha = $build.git_sha
    telemetry_samples = $telemetry.Count
    movement_active_samples = $movementActiveSamples
    command_samples = $commandSamples
    suspension_ready_samples = $suspensionReadySamples
    max_valid_wheels = $maxValidWheels
    max_contacts = $maxContacts
    max_suspension_samples = $maxSuspensionSamples
    max_speed_kmh = [Math]::Round($maxSpeed, 3)
    max_signed_speed_kmh = [Math]::Round($maxSignedSpeed, 3)
    observed_gears = @($observedGears | Sort-Object)
    automatic_gear_samples = $automaticGearSamples
    configured_forward_gears = $maxConfiguredForwardGears
    max_forward_gear_observed = $maxForwardGear
    automatic_gearbox_evidence_samples = $automaticGearboxEvidence.Count
    automatic_shift_events = $automaticShiftEvents.Count
    direction_shift_commits = $directionShiftCommits.Count
    unsafe_direction_shift_commits = $unsafeDirectionShiftCommits
    max_direction_shift_commit_speed_kmh = [Math]::Round($maxDirectionShiftCommitSpeedKmh, 3)
    max_slip_magnitude = [Math]::Round($maxSlipMagnitude, 3)
    max_slip_angle = [Math]::Round($maxSlipAngle, 3)
    max_axle_imbalance = [Math]::Round($maxAxleImbalance, 3)
    min_traction_authority = [Math]::Round($minTractionAuthority, 3)
    trailer_samples = $trailerSamples
    loaded_trailer_samples = $loadedTrailerSamples
    native_physics_accepted = $physicsAccepted
    wheel_setup_observed = $wheelSetupObserved
    native_smoke_ready = $nativeReady
    deterministic_fieldmaster_motion = $fieldmasterMotion
    deterministic_fieldmaster_control = $fieldmasterControl
    physics_fallback_observed = $physicsFallback
    failures = @($failures)
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Error "Native Chaos runtime telemetry gate failed: $($failures -join '; ')"
    exit 3
}

Write-Host "[GTT] Native Chaos runtime telemetry gate: PASS ($($telemetry.Count) samples, speed=$([Math]::Round($maxSpeed,2)) km/h, contacts=$maxContacts/4, suspension=$maxSuspensionSamples/4, commands=$commandSamples, auto-gears=$automaticGearSamples, max-forward-gear=$maxForwardGear)."
Write-Host "[GTT] Evidence: $OutputPath"
