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
$HillHaulPath = Join-Path $PackageDirectory 'FIELDMASTER_HILL_HAUL_RUNTIME.json'
$OutputPath = Join-Path $PackageDirectory 'FIELDMASTER_HUD_RUNTIME.json'

foreach ($path in @($BuildPath, $SmokePath, $AuthorityPath, $HillHaulPath, $RuntimeLog)) {
    if (-not (Test-Path $path -PathType Leaf)) { throw "Required Fieldmaster HUD evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$authority = Get-Content -Raw $AuthorityPath | ConvertFrom-Json
$hillHaul = Get-Content -Raw $HillHaulPath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

if ($build.platform -ne 'Win64') { $failures.Add('build platform is not Win64') }
if ($smoke.result -ne 'PASS') { $failures.Add('packaged runtime smoke did not PASS') }
if ($authority.result -ne 'PASS' -or $authority.authority -ne 'NATIVE_CHAOS' -or [int]$authority.authority_faults -ne 0) {
    $failures.Add('Native Chaos authority evidence did not PASS cleanly')
}
if ($hillHaul.result -ne 'PASS') { $failures.Add('Fieldmaster hill-haul runtime evidence did not PASS') }
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    $failures.Add("build SHA mismatch package=$($build.git_sha) expected=$ExpectedGitSha")
}
foreach ($doc in @($authority, $hillHaul)) {
    if ($doc.git_sha -ne $build.git_sha) { $failures.Add('runtime evidence SHA does not match BUILD_INFO.json') }
    if (($doc.PSObject.Properties.Name -contains 'version') -and $doc.version -ne $build.version) { $failures.Add('runtime evidence version does not match BUILD_INFO.json') }
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

$visibleAlerts = 0
$assistAlerts = 0
$thermalAlerts = 0
$coolingAlerts = 0
$criticalAlerts = 0
$runawayAlerts = 0
$labels = [ordered]@{
    TRAILER_RUNAWAY_ASSIST = 0
    TRAILER_BRAKES_CRITICAL = 0
    TRAILER_BRAKE_FADE = 0
    TRAILER_BRAKES_HOT = 0
    TRAILER_BRAKES_COOLING = 0
    DESCENT_ASSIST = 0
    HILL_HOLD_ACTIVE = 0
    NONE = 0
}
$sampleFailures = [System.Collections.Generic.List[string]]::new()

foreach ($line in $samples) {
    try {
        $heat = Get-NumberToken -Line $line -Name 'trailer_brake_heat'
        $authorityValue = Get-NumberToken -Line $line -Name 'trailer_brake_authority'
        $thermalState = Get-Token -Line $line -Name 'trailer_brake_state'
        $fade = Get-Token -Line $line -Name 'trailer_brake_fade'
        $cooling = Get-Token -Line $line -Name 'trailer_brake_cooling'
        $runaway = Get-Token -Line $line -Name 'runaway_mitigation'
        $downhill = Get-Token -Line $line -Name 'downhill_tow_brake'
        $hillHold = Get-Token -Line $line -Name 'hill_hold'

        foreach ($flag in @($fade, $cooling, $runaway, $downhill, $hillHold)) {
            if ($flag -notin @('YES', 'NO')) { throw "invalid YES/NO telemetry token '$flag'" }
        }
        if ($thermalState -notin @('NORMAL', 'HOT', 'FADING', 'CRITICAL')) { throw "invalid trailer_brake_state '$thermalState'" }
        if ($heat -lt 0.0 -or $heat -gt 1.001) { throw "trailer_brake_heat out of range: $heat" }
        if ($authorityValue -lt 0.549 -or $authorityValue -gt 1.001) { throw "trailer_brake_authority out of range: $authorityValue" }
        if ($fade -eq 'YES' -and $thermalState -notin @('FADING', 'CRITICAL')) { throw 'fade flag disagrees with HUD thermal state' }
        if ($runaway -eq 'YES' -and $thermalState -ne 'CRITICAL') { throw 'runaway mitigation must override HUD only in CRITICAL thermal state' }

        # Mirrors BuildNativeFieldmasterAlert precedence exactly. The packaged runtime
        # proves the authoritative telemetry state; the source verifier pins this mapping
        # to the actual HUD labels so CI cannot silently drift from runtime evidence.
        $label = 'NONE'
        if ($runaway -eq 'YES') { $label = 'TRAILER_RUNAWAY_ASSIST' }
        elseif ($thermalState -eq 'CRITICAL') { $label = 'TRAILER_BRAKES_CRITICAL' }
        elseif ($thermalState -eq 'FADING') { $label = 'TRAILER_BRAKE_FADE' }
        elseif ($thermalState -eq 'HOT') { $label = 'TRAILER_BRAKES_HOT' }
        elseif ($cooling -eq 'YES' -and $heat -ge 0.20) { $label = 'TRAILER_BRAKES_COOLING' }
        elseif ($downhill -eq 'YES') { $label = 'DESCENT_ASSIST' }
        elseif ($hillHold -eq 'YES') { $label = 'HILL_HOLD_ACTIVE' }

        $labels[$label] = [int]$labels[$label] + 1
        if ($label -ne 'NONE') { $visibleAlerts++ }
        if ($label -in @('DESCENT_ASSIST', 'HILL_HOLD_ACTIVE')) { $assistAlerts++ }
        if ($label -in @('TRAILER_BRAKES_HOT', 'TRAILER_BRAKE_FADE', 'TRAILER_BRAKES_CRITICAL', 'TRAILER_RUNAWAY_ASSIST')) { $thermalAlerts++ }
        if ($label -eq 'TRAILER_BRAKES_COOLING') { $coolingAlerts++ }
        if ($label -eq 'TRAILER_BRAKES_CRITICAL') { $criticalAlerts++ }
        if ($label -eq 'TRAILER_RUNAWAY_ASSIST') { $runawayAlerts++ }
    }
    catch {
        $sampleFailures.Add($_.Exception.Message)
    }
}

if ($samples.Count -lt 2) { $failures.Add("fewer than two Fieldmaster HUD telemetry samples were captured (found $($samples.Count))") }
if ($visibleAlerts -lt 1) { $failures.Add('no driver-visible Fieldmaster safety alert state was exercised') }
if (($assistAlerts + $thermalAlerts + $coolingAlerts) -lt 1) { $failures.Add('no hill-haul/thermal/cooling HUD state was exercised') }
if ([int]$hillHaul.assist_samples -gt 0 -and ($assistAlerts + $thermalAlerts + $coolingAlerts) -lt 1) { $failures.Add('hill-haul evidence exists but no corresponding HUD state was derived') }
if ([int]$hillHaul.thermal_samples -gt 0 -and ($thermalAlerts + $coolingAlerts) -lt 1) { $failures.Add('thermal evidence exists but no thermal/cooling HUD state was derived') }
foreach ($failure in $sampleFailures) { $failures.Add("HUD telemetry invariant: $failure") }

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.fieldmaster-hud-runtime.v1'
    result = $result
    game = 'Grand Theft Tractor'
    version = $build.version
    platform = $build.platform
    git_sha = $build.git_sha
    packaged_smoke = $smoke.result
    native_authority = $authority.authority
    native_authority_faults = [int]$authority.authority_faults
    telemetry_samples = $samples.Count
    visible_alert_samples = $visibleAlerts
    assist_alert_samples = $assistAlerts
    thermal_alert_samples = $thermalAlerts
    cooling_alert_samples = $coolingAlerts
    critical_alert_samples = $criticalAlerts
    runaway_alert_samples = $runawayAlerts
    hud_label_counts = $labels
    failures = @($failures)
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 7 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Error "Fieldmaster HUD runtime evidence failed: $($failures -join '; ')"
    exit 4
}

Write-Host "[GTT] Fieldmaster HUD runtime evidence: PASS ($visibleAlerts visible alerts from $($samples.Count) samples)."
Write-Host "[GTT] Evidence: $OutputPath"
