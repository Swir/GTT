param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)
$OutputPath = Join-Path $PackageDirectory 'NATIVE_TRAILER_RUNTIME.json'
$BuildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$SmokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'
$NativePath = Join-Path $PackageDirectory 'NATIVE_CHAOS_RUNTIME.json'
$DrivetrainPath = Join-Path $PackageDirectory 'NATIVE_DRIVETRAIN_SCENARIO.json'

foreach ($path in @($BuildPath,$SmokePath,$NativePath,$DrivetrainPath,$RuntimeLog)) {
    if (-not (Test-Path $path -PathType Leaf)) {
        throw "Required authored trailer runtime evidence missing: $path"
    }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$native = Get-Content -Raw $NativePath | ConvertFrom-Json
$drivetrain = Get-Content -Raw $DrivetrainPath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

if ($build.platform -ne 'Win64') { $failures.Add('build platform is not Win64') }
if ($smoke.result -ne 'PASS') { $failures.Add('packaged runtime smoke did not PASS') }
if ($native.schema -ne 'gtt.native-chaos-runtime.v1' -or $native.result -ne 'PASS') {
    $failures.Add('Native Chaos runtime evidence did not PASS')
}
if ($drivetrain.schema -ne 'gtt.native-drivetrain-scenario.v1' -or $drivetrain.result -ne 'PASS') {
    $failures.Add('deterministic drivetrain evidence did not PASS')
}
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    $failures.Add("build SHA mismatch package=$($build.git_sha) expected=$ExpectedGitSha")
}
foreach ($evidence in @($native,$drivetrain)) {
    if ($evidence.git_sha -and $evidence.git_sha -ne $build.git_sha) {
        $failures.Add('upstream runtime evidence SHA does not match BUILD_INFO.json')
    }
}

$pattern = 'AUTHORED_TRAILER_RUNTIME_EVIDENCE\s+trailer=(?<trailer>\S+)\s+active=(?<active>[01])\s+nativeTow=(?<nativeTow>[01])\s+contacts=(?<contacts>[0-9.]+)\s+left=(?<left>[01])\s+right=(?<right>[01])\s+clearL=(?<clearL>-?[0-9.]+)\s+clearR=(?<clearR>-?[0-9.]+)\s+axleTilt=(?<axleTilt>-?[0-9.]+)\s+hitchError=(?<hitchError>-?[0-9.]+)\s+articulation=(?<articulation>-?[0-9.]+)\s+stabilization=(?<stabilization>[0-9.]+)\s+warning=(?<warning>[01])'
$samples = [System.Collections.Generic.List[object]]::new()
$invalidRigLines = @($lines | Where-Object { $_ -match 'AUTHORED_TRAILER_RUNTIME_EVIDENCE.*active=0.*reason=NO_VALID_AUTHORED_RIG' })

foreach ($line in $lines) {
    $match = [regex]::Match($line, $pattern)
    if (-not $match.Success) { continue }
    $samples.Add([pscustomobject]@{
        trailer = $match.Groups['trailer'].Value
        active = [int]$match.Groups['active'].Value
        native_tow = [int]$match.Groups['nativeTow'].Value
        contacts = [double]::Parse($match.Groups['contacts'].Value,[Globalization.CultureInfo]::InvariantCulture)
        left = [int]$match.Groups['left'].Value
        right = [int]$match.Groups['right'].Value
        left_clearance_cm = [double]::Parse($match.Groups['clearL'].Value,[Globalization.CultureInfo]::InvariantCulture)
        right_clearance_cm = [double]::Parse($match.Groups['clearR'].Value,[Globalization.CultureInfo]::InvariantCulture)
        axle_tilt_deg = [double]::Parse($match.Groups['axleTilt'].Value,[Globalization.CultureInfo]::InvariantCulture)
        hitch_error_cm = [double]::Parse($match.Groups['hitchError'].Value,[Globalization.CultureInfo]::InvariantCulture)
        articulation_deg = [double]::Parse($match.Groups['articulation'].Value,[Globalization.CultureInfo]::InvariantCulture)
        stabilization = [double]::Parse($match.Groups['stabilization'].Value,[Globalization.CultureInfo]::InvariantCulture)
        warning = [int]$match.Groups['warning'].Value
    })
}

$active = @($samples | Where-Object { $_.active -eq 1 })
$attached = @($active | Where-Object { $_.native_tow -eq 1 })
$grounded = @($attached | Where-Object { $_.contacts -ge 0.999 -and $_.left -eq 1 -and $_.right -eq 1 })
$safe = @($grounded | Where-Object { $_.warning -eq 0 -and $_.hitch_error_cm -le 80.0 })
$trailers = @($active | Select-Object -ExpandProperty trailer -Unique)

if ($samples.Count -eq 0) { $failures.Add('no AUTHORED_TRAILER_RUNTIME_EVIDENCE samples were found') }
if ($active.Count -lt 2) { $failures.Add("fewer than two authored-rig active samples were observed ($($active.Count))") }
if ($attached.Count -lt 2) { $failures.Add("fewer than two authored trailer samples were attached to the Native Fieldmaster ($($attached.Count))") }
if ($grounded.Count -lt 2) { $failures.Add("both authored trailer wheels were not grounded in at least two samples ($($grounded.Count))") }
if ($safe.Count -lt 2) { $failures.Add("hitch alignment/contact safety was not proven in at least two samples ($($safe.Count))") }
if ($trailers.Count -lt 1) { $failures.Add('no valid authored trailer instance was observed') }

$scenarioSamplePattern = 'NATIVE_TRAILER_SCENARIO_SAMPLE\s+speed_kmh=(?<speed>[0-9.]+)\s+distance_cm=(?<distance>[0-9.]+)\s+loaded=(?<loaded>[01])\s+attached=(?<attached>[01])\s+active=(?<active>[01])\s+contacts=(?<contacts>[0-9.]+)\s+left=(?<left>[01])\s+right=(?<right>[01])\s+hitch_error_cm=(?<hitch>-?[0-9.]+)\s+articulation_deg=(?<articulation>-?[0-9.]+)\s+cargo_integrity=(?<cargo>[0-9.]+)\s+trailer_integrity=(?<trailer>[0-9.]+)\s+hitch_load=(?<hitchLoad>[0-9.]+)'
$scenarioSamples = [System.Collections.Generic.List[object]]::new()
foreach ($line in $lines) {
    $match = [regex]::Match($line, $scenarioSamplePattern)
    if (-not $match.Success) { continue }
    $scenarioSamples.Add([pscustomobject]@{
        speed_kmh = [double]::Parse($match.Groups['speed'].Value,[Globalization.CultureInfo]::InvariantCulture)
        distance_cm = [double]::Parse($match.Groups['distance'].Value,[Globalization.CultureInfo]::InvariantCulture)
        loaded = [int]$match.Groups['loaded'].Value
        attached = [int]$match.Groups['attached'].Value
        active = [int]$match.Groups['active'].Value
        contacts = [double]::Parse($match.Groups['contacts'].Value,[Globalization.CultureInfo]::InvariantCulture)
        left = [int]$match.Groups['left'].Value
        right = [int]$match.Groups['right'].Value
        hitch_error_cm = [double]::Parse($match.Groups['hitch'].Value,[Globalization.CultureInfo]::InvariantCulture)
        articulation_deg = [double]::Parse($match.Groups['articulation'].Value,[Globalization.CultureInfo]::InvariantCulture)
        cargo_integrity = [double]::Parse($match.Groups['cargo'].Value,[Globalization.CultureInfo]::InvariantCulture)
        trailer_integrity = [double]::Parse($match.Groups['trailer'].Value,[Globalization.CultureInfo]::InvariantCulture)
        hitch_load = [double]::Parse($match.Groups['hitchLoad'].Value,[Globalization.CultureInfo]::InvariantCulture)
    })
}

$scenarioCompletePattern = 'NATIVE_TRAILER_SCENARIO_COMPLETE\s+result=(?<result>PASS|FAIL)\s+route=loaded-authored-tow\s+attachment=(?<attachment>[01])\s+authored=(?<authored>[01])\s+loaded=(?<loaded>[01])\s+stopped=(?<stopped>[01])\s+max_speed_kmh=(?<speed>[0-9.]+)\s+distance_cm=(?<distance>[0-9.]+)\s+dual_contact_samples=(?<dual>\d+)\s+safe_samples=(?<safe>\d+)\s+max_hitch_error_cm=(?<hitch>[0-9.]+)\s+max_articulation_deg=(?<articulation>[0-9.]+)\s+min_cargo_integrity=(?<cargo>[0-9.]+)\s+final_speed_kmh=(?<finalSpeed>[0-9.]+)'
$scenarioComplete = $null
foreach ($line in $lines) {
    $match = [regex]::Match($line, $scenarioCompletePattern)
    if (-not $match.Success) { continue }
    $scenarioComplete = [pscustomobject]@{
        result = $match.Groups['result'].Value
        attachment = [int]$match.Groups['attachment'].Value
        authored = [int]$match.Groups['authored'].Value
        loaded = [int]$match.Groups['loaded'].Value
        stopped = [int]$match.Groups['stopped'].Value
        max_speed_kmh = [double]::Parse($match.Groups['speed'].Value,[Globalization.CultureInfo]::InvariantCulture)
        distance_cm = [double]::Parse($match.Groups['distance'].Value,[Globalization.CultureInfo]::InvariantCulture)
        dual_contact_samples = [int]$match.Groups['dual'].Value
        safe_samples = [int]$match.Groups['safe'].Value
        max_hitch_error_cm = [double]::Parse($match.Groups['hitch'].Value,[Globalization.CultureInfo]::InvariantCulture)
        max_articulation_deg = [double]::Parse($match.Groups['articulation'].Value,[Globalization.CultureInfo]::InvariantCulture)
        min_cargo_integrity = [double]::Parse($match.Groups['cargo'].Value,[Globalization.CultureInfo]::InvariantCulture)
        final_speed_kmh = [double]::Parse($match.Groups['finalSpeed'].Value,[Globalization.CultureInfo]::InvariantCulture)
    }
}

$scenarioDiagnosticFailures = @($lines | Where-Object { $_ -match 'NATIVE_TRAILER_SCENARIO phase=DIAGNOSTIC result=FAIL' })
$movingLoadedSafe = @($scenarioSamples | Where-Object {
    $_.speed_kmh -ge 4.0 -and $_.loaded -eq 1 -and $_.attached -eq 1 -and $_.active -eq 1 -and
    $_.contacts -ge 0.999 -and $_.left -eq 1 -and $_.right -eq 1 -and $_.hitch_error_cm -le 80.0
})

if (-not $scenarioComplete) {
    $failures.Add('no NATIVE_TRAILER_SCENARIO_COMPLETE marker was found')
} else {
    if ($scenarioComplete.result -ne 'PASS') { $failures.Add('deterministic loaded authored-trailer scenario did not PASS') }
    if ($scenarioComplete.attachment -ne 1 -or $scenarioComplete.authored -ne 1 -or $scenarioComplete.loaded -ne 1 -or $scenarioComplete.stopped -ne 1) {
        $failures.Add('trailer scenario did not prove attachment, authored takeover, loaded tow, and controlled stop')
    }
    if ($scenarioComplete.max_speed_kmh -lt 4.0) { $failures.Add("trailer scenario max speed was below 4 km/h ($($scenarioComplete.max_speed_kmh))") }
    if ($scenarioComplete.distance_cm -lt 900.0) { $failures.Add("trailer scenario distance was below 900 cm ($($scenarioComplete.distance_cm))") }
    if ($scenarioComplete.dual_contact_samples -lt 8 -or $scenarioComplete.safe_samples -lt 8) {
        $failures.Add('trailer scenario did not sustain dual-wheel contact and safe hitch for at least eight moving samples')
    }
    if ($scenarioComplete.max_hitch_error_cm -gt 110.0) { $failures.Add("trailer scenario exceeded hard hitch envelope ($($scenarioComplete.max_hitch_error_cm) cm)") }
}
if ($movingLoadedSafe.Count -lt 8) { $failures.Add("fewer than eight safe moving loaded trailer samples were observed ($($movingLoadedSafe.Count))") }
if ($scenarioDiagnosticFailures.Count -gt 0) { $failures.Add("trailer scenario emitted diagnostic failures ($($scenarioDiagnosticFailures.Count))") }

$maxHitchError = if ($attached.Count) { [double](($attached | Measure-Object hitch_error_cm -Maximum).Maximum) } else { $null }
$maxArticulation = if ($attached.Count) { [double](($attached | Measure-Object articulation_deg -Maximum).Maximum) } else { $null }
$maxStabilization = if ($attached.Count) { [double](($attached | Measure-Object stabilization -Maximum).Maximum) } else { $null }
$maxAxleTilt = if ($attached.Count) { [double](($attached | ForEach-Object {[Math]::Abs($_.axle_tilt_deg)} | Measure-Object -Maximum).Maximum) } else { $null }
$minLeftClearance = if ($grounded.Count) { [double](($grounded | Measure-Object left_clearance_cm -Minimum).Minimum) } else { $null }
$minRightClearance = if ($grounded.Count) { [double](($grounded | Measure-Object right_clearance_cm -Minimum).Minimum) } else { $null }

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.native-trailer-runtime.v1'
    result = $result
    game = 'Grand Theft Tractor'
    version = $build.version
    platform = $build.platform
    git_sha = $build.git_sha
    telemetry_samples = $samples.Count
    authored_active_samples = $active.Count
    native_tow_samples = $attached.Count
    dual_contact_samples = $grounded.Count
    safe_hitch_samples = $safe.Count
    valid_trailer_instances = $trailers
    invalid_rig_observation_count = $invalidRigLines.Count
    max_hitch_error_cm = $maxHitchError
    max_articulation_deg = $maxArticulation
    max_abs_axle_tilt_deg = $maxAxleTilt
    max_stabilization_load = $maxStabilization
    min_left_clearance_cm = $minLeftClearance
    min_right_clearance_cm = $minRightClearance
    deterministic_loaded_tow = if ($scenarioComplete) { $scenarioComplete.result } else { 'MISSING' }
    loaded_motion_samples = $scenarioSamples.Count
    safe_loaded_motion_samples = $movingLoadedSafe.Count
    loaded_tow_max_speed_kmh = if ($scenarioComplete) { $scenarioComplete.max_speed_kmh } else { $null }
    loaded_tow_distance_cm = if ($scenarioComplete) { $scenarioComplete.distance_cm } else { $null }
    loaded_tow_max_hitch_error_cm = if ($scenarioComplete) { $scenarioComplete.max_hitch_error_cm } else { $null }
    loaded_tow_max_articulation_deg = if ($scenarioComplete) { $scenarioComplete.max_articulation_deg } else { $null }
    loaded_tow_min_cargo_integrity = if ($scenarioComplete) { $scenarioComplete.min_cargo_integrity } else { $null }
    controlled_stop_proven = if ($scenarioComplete) { [bool]($scenarioComplete.stopped -eq 1) } else { $false }
    trailer_scenario_diagnostic_failures = $scenarioDiagnosticFailures.Count
    requires_final_authored_rig = $true
    source_native_chaos_runtime = $native.result
    source_drivetrain_scenario = $drivetrain.result
    failures = @($failures)
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Error "Authored trailer runtime acceptance failed: $($failures -join '; ')"
    exit 5
}

Write-Host "[GTT] Authored trailer runtime acceptance: PASS (samples=$($samples.Count), attached=$($attached.Count), grounded=$($grounded.Count), safe=$($safe.Count), loaded-motion=$($movingLoadedSafe.Count))."
Write-Host "[GTT] Evidence: $OutputPath"
