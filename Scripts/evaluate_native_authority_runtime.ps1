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
$OutputPath = Join-Path $PackageDirectory 'NATIVE_AUTHORITY_RUNTIME.json'

foreach ($path in @($BuildPath, $SmokePath, $RuntimeLog)) {
    if (-not (Test-Path $path)) { throw "Required native-authority evidence missing: $path" }
}

$build = Get-Content -Raw $BuildPath | ConvertFrom-Json
$smoke = Get-Content -Raw $SmokePath | ConvertFrom-Json
$lines = @(Get-Content $RuntimeLog)
$failures = [System.Collections.Generic.List[string]]::new()

if ($build.platform -ne 'Win64') { $failures.Add('build platform is not Win64') }
if ($smoke.result -ne 'PASS') { $failures.Add('packaged runtime smoke did not PASS') }
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    $failures.Add("build SHA mismatch package=$($build.git_sha) expected=$ExpectedGitSha")
}

$authoritySamples = @($lines | Where-Object {
    $_ -match 'NATIVE_FIELDMASTER_RUNTIME_TELEMETRY\s+vehicle=RustyFieldmaster60' -and
    $_ -match '(?:^|\s)authority=NATIVE_CHAOS(?:\s|$)' -and
    $_ -match '(?:^|\s)takeover_integrity=PASS(?:\s|$)' -and
    $_ -match '(?:^|\s)movement_class=GTTFieldmasterChaosMovementComponent(?:\s|$)' -and
    $_ -match '(?:^|\s)legacy_mirror=QUIESCENT(?:\s|$)' -and
    $_ -match '(?:^|\s)legacy_collision=NO(?:\s|$)' -and
    $_ -match '(?:^|\s)legacy_tick=NO(?:\s|$)' -and
    $_ -match '(?:^|\s)native_collision=YES(?:\s|$)' -and
    $_ -match '(?:^|\s)physics_asset=YES(?:\s|$)' -and
    $_ -match '(?:^|\s)movement=ACTIVE(?:\s|$)'
})
$authorityFaults = @($lines | Where-Object { $_ -match 'NATIVE_FIELDMASTER_AUTHORITY_FAULT\s+vehicle=RustyFieldmaster60' })

if ($authoritySamples.Count -lt 2) {
    $failures.Add("fewer than two authoritative Native Chaos samples were captured (found $($authoritySamples.Count))")
}
if ($authorityFaults.Count -gt 0) {
    $failures.Add("native authority watchdog observed $($authorityFaults.Count) split-authority fault(s)")
}

$result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
$evidence = [ordered]@{
    schema = 'gtt.native-authority-runtime.v1'
    result = $result
    game = 'Grand Theft Tractor'
    version = $build.version
    platform = $build.platform
    git_sha = $build.git_sha
    authority = 'NATIVE_CHAOS'
    authority_samples = $authoritySamples.Count
    authority_faults = $authorityFaults.Count
    movement_class = 'GTTFieldmasterChaosMovementComponent'
    legacy_mirror_required_state = 'QUIESCENT'
    packaged_smoke = $smoke.result
    failures = @($failures)
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$evidence | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Error "Native authority runtime gate failed: $($failures -join '; ')"
    exit 3
}

Write-Host "[GTT] Native Fieldmaster authority runtime gate: PASS ($($authoritySamples.Count) authoritative samples, zero split-authority faults)."
Write-Host "[GTT] Evidence: $OutputPath"
