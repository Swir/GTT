param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)
$BuildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$SmokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'
$AuthorityPath = Join-Path $PackageDirectory 'NATIVE_AUTHORITY_RUNTIME.json'
$OutputPath = Join-Path $PackageDirectory 'FIELDMASTER_HILL_HAUL_RUNTIME.json'

foreach ($path in @($BuildPath, $SmokePath, $AuthorityPath, $RuntimeLog)) {
    if (-not (Test-Path $path -PathType Leaf)) { throw "Required hill-haul evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$authority = Get-Content -Raw $AuthorityPath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

if ($build.platform -ne 'Win64') { $failures.Add('build platform is not Win64') }
if ($smoke.result -ne 'PASS') { $failures.Add('packaged runtime smoke did not PASS') }
if ($authority.result -ne 'PASS' -or $authority.authority -ne 'NATIVE_CHAOS' -or [int]$authority.authority_faults -ne 0) {
    $failures.Add('Native Chaos authority evidence did not PASS cleanly')
}
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    $failures.Add("build SHA mismatch package=$($build.git_sha) expected=$ExpectedGitSha")
}
if ($authority.git_sha -ne $build.git_sha) {
    $failures.Add("native authority SHA mismatch authority=$($authority.git_sha) build=$($build.git_sha)")
}

function Get-Token {
    param([Parameter(Mandatory=$true)][string]$Line, [Parameter(Mandatory=$true)][string]$Name)
    $pattern = '(?:^|\s)' + [regex]::Escape($Name) + '=(?<value>[^\s]+)'
    $match = [regex]::Match($Line, $pattern)
    if (-not $match.Success) { return $null }
    return $match.Groups['value'].Value
}

function Get-NumberToken {
    param([Parameter(Mandatory=$true)][string]$Line, [Parameter(Mandatory=$true)][string]$Name)
    $raw = Get-Token -Line $Line -Name $Name
    if ($null -eq $raw) { throw "missing numeric token '$Name'" }
    try { return [double]::Parse($raw, [Globalization.CultureInfo]::InvariantCulture) }
    catch { throw "invalid numeric token '$Name=$raw'" }
}

$samples = @($lines | Where-Object {
    $_ -match 'NATIVE_FIELDMASTER_RUNTIME_TELEMETRY\s+vehicle=RustyFieldmaster60' -and
    $_ -match '(?:^|\s)authority=NATIVE_CHAOS(?:\s|$)' -and
    $_ -match '(?:^|\s)takeover_integrity=PASS(?:\s|$)' -and
    $_ -match '(?:^|\s)trailer=ATTACHED(?:\s|$)'
})

$loadedSamples = 0
$assistSamples = 0
$thermalSamples = 0
$runawaySamples = 0
$maxHeat = 0.0
$minBrakeAuthority = 1.0
$maxAbsoluteGrade = 0.0
$maxSpeedKmh = 0.0
$sampleFailures = [System.Collections.Generic.List[string]]::new()

foreach ($line in $samples) {
    try {
        $towLoad = Get-NumberToken -Line $line -Name 'tow_load'
        $terrainAuthority = Get-NumberToken -Line $line -Name 'terrain_throttle_authority'
        $grade = Get-NumberToken -Line $line -Name 'travel_grade_deg'
        $speed = Get-NumberToken -Line $line -Name 'speed_kmh'
        $hillBrake = Get-NumberToken -Line $line -Name 'hill_haul_brake'
        $heat = Get-NumberToken -Line $line -Name 'trailer_brake_heat'
        $brakeAuthority = Get-NumberToken -Line $line -Name 'trailer_brake_authority'
        $runawayBrake = Get-NumberToken -Line $line -Name 'runaway_brake'
        $hillHold = Get-Token -Line $line -Name 'hill_hold'
        $downhillBrake = Get-Token -Line $line -Name 'downhill_tow_brake'
        $thermalState = Get-Token -Line $line -Name 'trailer_brake_state'
        $fade = Get-Token -Line $line -Name 'trailer_brake_fade'
        $cooling = Get-Token -Line $line -Name 'trailer_brake_cooling'
        $runaway = Get-Token -Line $line -Name 'runaway_mitigation'

        foreach ($flag in @($hillHold, $downhillBrake, $fade, $cooling, $runaway)) {
            if ($flag -notin @('YES', 'NO')) { throw "invalid YES/NO telemetry token '$flag'" }
        }
        if ($thermalState -notin @('NORMAL', 'HOT', 'FADING', 'CRITICAL')) {
            throw "invalid trailer_brake_state '$thermalState'"
        }

        if ($towLoad -lt 0.0 -or $towLoad -gt 1.001) { throw "tow_load out of range: $towLoad" }
        if ($terrainAuthority -lt 0.0 -or $terrainAuthority -gt 1.001) { throw "terrain_throttle_authority out of range: $terrainAuthority" }
        if ($hillBrake -lt 0.0 -or $hillBrake -gt 1.001) { throw "hill_haul_brake out of range: $hillBrake" }
        if ($heat -lt 0.0 -or $heat -gt 1.001) { throw "trailer_brake_heat out of range: $heat" }
        if ($brakeAuthority -lt 0.549 -or $brakeAuthority -gt 1.001) { throw "trailer_brake_authority out of range: $brakeAuthority" }
        if ($runawayBrake -lt 0.0 -or $runawayBrake -gt 0.201) { throw "runaway_brake out of range: $runawayBrake" }

        if ($towLoad -ge 0.15) { $loadedSamples++ }
        if ($hillHold -eq 'YES' -or $downhillBrake -eq 'YES') { $assistSamples++ }
        if ($heat -gt 0.01 -or $thermalState -ne 'NORMAL' -or $fade -eq 'YES' -or $cooling -eq 'YES') { $thermalSamples++ }

        if ($fade -eq 'YES' -and $heat -lt 0.60) {
            throw "fade reported below the production 0.62 threshold (tolerance floor 0.60): heat=$heat"
        }
        if ($runaway -eq 'YES') {
            $runawaySamples++
            if ($thermalState -ne 'CRITICAL') { throw "runaway mitigation without CRITICAL thermal state" }
            if ($towLoad -lt 0.50) { throw "runaway mitigation below 0.50 tow load" }
            if ($grade -gt -8.0) { throw "runaway mitigation outside an >=8 degree descent: grade=$grade" }
            if ($speed -lt 24.0) { throw "runaway mitigation below 24 km/h: speed=$speed" }
            if ($runawayBrake -le 0.0) { throw "runaway mitigation reported without tractor-side safety brake" }
        }

        $maxHeat = [Math]::Max($maxHeat, $heat)
        $minBrakeAuthority = [Math]::Min($minBrakeAuthority, $brakeAuthority)
        $maxAbsoluteGrade = [Math]::Max($maxAbsoluteGrade, [Math]::Abs($grade))
        $maxSpeedKmh = [Math]::Max($maxSpeedKmh, $speed)
    }
    catch {
        $sampleFailures.Add($_.Exception.Message)
    }
}

if ($samples.Count -lt 2) {
    $failures.Add("fewer than two attached-trailer telemetry samples were captured (found $($samples.Count))")
}
if ($loadedSamples -lt 2) {
    $failures.Add("fewer than two loaded-trailer telemetry samples were captured (found $loadedSamples)")
}
if ($assistSamples -lt 1) {
    $failures.Add('no hill-hold/downhill-assist sample was captured')
}
if ($thermalSamples -lt 1) {
    $failures.Add('no trailer-brake thermal behavior sample was captured')
}
foreach ($failure in $sampleFailures) { $failures.Add("telemetry invariant: $failure") }

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.fieldmaster-hill-haul-runtime.v1'
    result = $result
    game = 'Grand Theft Tractor'
    version = $build.version
    platform = $build.platform
    git_sha = $build.git_sha
    packaged_smoke = $smoke.result
    native_authority = $authority.authority
    native_authority_faults = [int]$authority.authority_faults
    attached_trailer_samples = $samples.Count
    loaded_trailer_samples = $loadedSamples
    assist_samples = $assistSamples
    thermal_samples = $thermalSamples
    runaway_samples = $runawaySamples
    max_trailer_brake_heat = [Math]::Round($maxHeat, 4)
    min_trailer_brake_authority = [Math]::Round($minBrakeAuthority, 4)
    max_absolute_grade_deg = [Math]::Round($maxAbsoluteGrade, 3)
    max_speed_kmh = [Math]::Round($maxSpeedKmh, 3)
    failures = @($failures)
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Error "Fieldmaster hill-haul runtime gate failed: $($failures -join '; ')"
    exit 3
}

Write-Host "[GTT] Fieldmaster hill-haul runtime gate: PASS ($loadedSamples loaded samples, $assistSamples assist, $thermalSamples thermal, $runawaySamples runaway)."
Write-Host "[GTT] Evidence: $OutputPath"
