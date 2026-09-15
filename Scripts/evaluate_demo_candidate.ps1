param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha = $env:GITHUB_SHA,
    [switch]$RequireVisual
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog = [IO.Path]::GetFullPath($RuntimeLog)

$buildPath = Join-Path $PackageDirectory 'BUILD_INFO.json'
$smokePath = Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'
if (-not (Test-Path $buildPath)) { throw 'BUILD_INFO.json missing.' }
if (-not (Test-Path $smokePath)) { throw 'RUNTIME_SMOKE.json missing.' }
if (-not (Test-Path $RuntimeLog)) { throw "Runtime log missing: $RuntimeLog" }

$build = Get-Content $buildPath -Raw | ConvertFrom-Json
$smoke = Get-Content $smokePath -Raw | ConvertFrom-Json
$log = Get-Content $RuntimeLog -Raw
if ($smoke.result -ne 'PASS') { throw 'Packaged runtime smoke did not PASS.' }
if ($build.platform -ne 'Win64') { throw 'Build evidence is not Win64.' }
if ($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha) {
    throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"
}

$vehicles = @('Fieldmaster','Rattleback82','Mulebox1200')
$missing = @()
foreach ($vehicle in $vehicles) {
    $pattern = "NATIVE_CHAOS_SMOKE_READY.*vehicle=$vehicle"
    if ($log -notmatch $pattern) { $missing += $vehicle }
}
if ($missing.Count -gt 0) {
    throw "Native Chaos packaged evidence missing for: $($missing -join ', ')"
}

$visualPath = Join-Path $PackageDirectory 'DEMO_VISUAL_ACCEPTANCE.json'
$visualStatus = 'NOT_REQUIRED_FOR_TECHNICAL_GATE'
if ($RequireVisual) {
    if (-not (Test-Path $visualPath)) { throw 'DEMO_VISUAL_ACCEPTANCE.json missing.' }
    $visual = Get-Content $visualPath -Raw | ConvertFrom-Json
    if ($visual.result -ne 'PASS') { throw 'Visual acceptance is not PASS.' }
    if (-not $visual.reviewer -or -not $visual.reviewed_utc) { throw 'Visual acceptance reviewer/timestamp missing.' }
    $visualStatus = 'PASS'
}

$evidence = [ordered]@{
    schema = 1
    game = 'Grand Theft Tractor'
    result = 'PASS'
    git_sha = $build.git_sha
    version = $build.version
    platform = $build.platform
    runtime_smoke = 'PASS'
    native_chaos_smoke_ready = $vehicles
    visual_acceptance = $visualStatus
    evaluated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$out = Join-Path $PackageDirectory 'DEMO_TECHNICAL_GATE.json'
$evidence | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 $out
Write-Host '[GTT] Demo technical evidence gate: PASS'
Write-Host "[GTT] Evidence: $out"
