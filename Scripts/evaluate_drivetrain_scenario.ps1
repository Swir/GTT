param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)
$OutputPath = Join-Path $PackageDirectory 'NATIVE_DRIVETRAIN_SCENARIO.json'
$BuildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$SmokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'
$NativePath = Join-Path $PackageDirectory 'NATIVE_CHAOS_RUNTIME.json'

foreach ($path in @($BuildPath, $SmokePath, $NativePath, $RuntimeLog)) {
    if (-not (Test-Path $path)) { throw "Required deterministic drivetrain evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$native = Get-Content -Raw $NativePath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

function Find-Phase([string]$Phase, [string]$Result = 'PASS') {
    return @($lines | Where-Object { $_ -match "NATIVE_DRIVETRAIN_SCENARIO\s+phase=$([regex]::Escape($Phase))\s+result=$([regex]::Escape($Result))(?:\s|$)" }) | Select-Object -Last 1
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
if ($native.result -ne 'PASS') { $failures.Add('Native Chaos runtime gate did not PASS before drivetrain scenario evaluation') }
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    $failures.Add("build SHA mismatch package=$($build.git_sha) expected=$ExpectedGitSha")
}
if ($native.git_sha -ne $build.git_sha) { $failures.Add('Native Chaos runtime evidence SHA does not match BUILD_INFO.json') }

$begin = @($lines | Where-Object { $_ -match 'NATIVE_DRIVETRAIN_SCENARIO_BEGIN\s+version=1' }) | Select-Object -Last 1
$automatic = Find-Phase 'AUTOMATIC_UPSHIFT'
$reverseInterlock = Find-Phase 'REVERSE_INTERLOCK'
$reverseCommit = Find-Phase 'REVERSE_COMMIT'
$reverseMotion = Find-Phase 'REVERSE_MOTION'
$forwardCommit = Find-Phase 'FORWARD_COMMIT'
$forwardMotion = Find-Phase 'FORWARD_MOTION'
$complete = @($lines | Where-Object { $_ -match 'NATIVE_DRIVETRAIN_SCENARIO_COMPLETE\s+result=PASS\s+route=forward-auto-reverse-forward' }) | Select-Object -Last 1
$diagnosticFailures = @($lines | Where-Object { $_ -match 'NATIVE_DRIVETRAIN_SCENARIO\s+phase=DIAGNOSTIC\s+result=FAIL' })

if (-not $begin) { $failures.Add('deterministic drivetrain scenario begin marker is missing') }
if (-not $automatic) { $failures.Add('automatic forward upshift was not proven') }
if (-not $reverseInterlock) { $failures.Add('high-speed reverse interlock/braking phase was not proven') }
if (-not $reverseCommit) { $failures.Add('safe reverse commit was not proven') }
if (-not $reverseMotion) { $failures.Add('measurable reverse motion was not proven') }
if (-not $forwardCommit) { $failures.Add('safe return-to-forward commit was not proven') }
if (-not $forwardMotion) { $failures.Add('measurable forward motion after reverse was not proven') }
if (-not $complete) { $failures.Add('forward-auto-reverse-forward sequence did not complete with PASS') }
if ($diagnosticFailures.Count -gt 0) { $failures.Add("scenario logged $($diagnosticFailures.Count) diagnostic failure(s)") }

$automaticGear = Read-Integer $automatic 'gear'
$automaticSpeed = Read-Number $automatic 'speed_kmh'
$interlockSpeed = Read-Number $reverseInterlock 'speed_abs_kmh'
$reverseCommitSpeed = Read-Number $reverseCommit 'speed_abs_kmh'
$reverseSignedSpeed = Read-Number $reverseMotion 'signed_speed_kmh'
$reverseGear = Read-Integer $reverseMotion 'gear'
$forwardCommitSpeed = Read-Number $forwardCommit 'speed_abs_kmh'
$forwardSignedSpeed = Read-Number $forwardMotion 'signed_speed_kmh'
$forwardGear = Read-Integer $forwardMotion 'gear'
$maxForwardGear = Read-Integer $complete 'max_forward_gear'

if ($null -ne $automaticGear -and $automaticGear -lt 2) { $failures.Add("automatic shift evidence stayed below gear 2 (gear=$automaticGear)") }
if ($null -ne $automaticSpeed -and $automaticSpeed -lt 6.0) { $failures.Add("automatic shift evidence occurred below the deterministic forward-speed floor ($automaticSpeed km/h)") }
if ($null -ne $interlockSpeed -and $interlockSpeed -le 3.5) { $failures.Add("reverse interlock was not exercised above the 3.5 km/h release threshold ($interlockSpeed km/h)") }
if ($null -ne $reverseCommitSpeed -and $reverseCommitSpeed -gt 3.75) { $failures.Add("reverse was committed above the safe release window ($reverseCommitSpeed km/h)") }
if ($null -ne $reverseSignedSpeed -and $reverseSignedSpeed -gt -4.5) { $failures.Add("reverse motion was too small to prove direction change ($reverseSignedSpeed km/h)") }
if ($null -ne $reverseGear -and $reverseGear -ge 0) { $failures.Add("reverse-motion evidence did not use a reverse gear (gear=$reverseGear)") }
if ($null -ne $forwardCommitSpeed -and $forwardCommitSpeed -gt 3.75) { $failures.Add("forward return was committed above the safe release window ($forwardCommitSpeed km/h)") }
if ($null -ne $forwardSignedSpeed -and $forwardSignedSpeed -lt 1.5) { $failures.Add("forward return motion was too small ($forwardSignedSpeed km/h)") }
if ($null -ne $forwardGear -and $forwardGear -le 0) { $failures.Add("forward-return evidence did not use a forward gear (gear=$forwardGear)") }
if ($null -ne $maxForwardGear -and $maxForwardGear -lt 2) { $failures.Add("scenario never observed a higher automatic forward gear (max=$maxForwardGear)") }

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.native-drivetrain-scenario.v1'
    result = $result
    game = 'Grand Theft Tractor'
    version = $build.version
    platform = $build.platform
    git_sha = $build.git_sha
    automatic_upshift_gear = $automaticGear
    automatic_upshift_speed_kmh = $automaticSpeed
    reverse_interlock_speed_kmh = $interlockSpeed
    reverse_commit_speed_kmh = $reverseCommitSpeed
    reverse_motion_signed_speed_kmh = $reverseSignedSpeed
    reverse_motion_gear = $reverseGear
    forward_commit_speed_kmh = $forwardCommitSpeed
    forward_motion_signed_speed_kmh = $forwardSignedSpeed
    forward_motion_gear = $forwardGear
    max_forward_gear_observed = $maxForwardGear
    safe_shift_release_kmh = 3.5
    diagnostic_failure_count = $diagnosticFailures.Count
    failures = @($failures)
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Error "Deterministic Native drivetrain scenario failed: $($failures -join '; ')"
    exit 4
}

Write-Host "[GTT] Deterministic Native drivetrain scenario: PASS (auto gear=$automaticGear, reverse=$reverseSignedSpeed km/h, return=$forwardSignedSpeed km/h)."
Write-Host "[GTT] Evidence: $OutputPath"
