param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$Version,
    [ValidateSet("Development", "Shipping")]
    [Parameter(Mandatory=$true)][string]$Configuration,
    [Parameter(Mandatory=$true)][string]$ExpectedGitSha,
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$ExpectedGitSha = $ExpectedGitSha.ToLowerInvariant()
if ($ExpectedGitSha -notmatch '^[0-9a-f]{40}$') { throw "ExpectedGitSha must be an exact 40-character Git SHA." }
if ([string]::IsNullOrWhiteSpace($Version)) { throw "Version must not be empty." }
if (-not (Test-Path $PackageDirectory -PathType Container)) { throw "Package directory does not exist: $PackageDirectory" }

$zipPath = "$PackageDirectory.zip"
$sidecarPath = "$zipPath.sha256"
if (-not (Test-Path $zipPath -PathType Leaf)) { throw "Sealed candidate ZIP missing: $zipPath" }
if (-not (Test-Path $sidecarPath -PathType Leaf)) { throw "Sealed candidate SHA256 sidecar missing: $sidecarPath" }

# Reuse the canonical archive verifier first. A human-review packet must never be
# prepared from a merely source-qualified or unsealed candidate.
$archiveVerifier = Join-Path $PSScriptRoot "verify_win64_candidate_archive.ps1"
if (-not (Test-Path $archiveVerifier -PathType Leaf)) { throw "Canonical archive verifier missing: $archiveVerifier" }
& $archiveVerifier -PackageDirectory $PackageDirectory -Version $Version -Configuration $Configuration -ExpectedGitSha $ExpectedGitSha
if ($LASTEXITCODE -ne 0) { throw "Canonical sealed candidate archive verification failed." }

$archiveHash = (Get-FileHash -Algorithm SHA256 -Path $zipPath).Hash.ToLowerInvariant()
$sidecar = (Get-Content -Raw $sidecarPath).Trim()
$sidecarMatch = [regex]::Match($sidecar, '^(?<hash>[0-9a-f]{64})  (?<name>[^/\\]+\.zip)$')
if (-not $sidecarMatch.Success) { throw "Candidate ZIP SHA256 sidecar has invalid canonical format." }
if ($sidecarMatch.Groups['hash'].Value -ne $archiveHash) { throw "Candidate ZIP SHA256 sidecar does not match the sealed archive." }
if ($sidecarMatch.Groups['name'].Value -ne [IO.Path]::GetFileName($zipPath)) { throw "Candidate ZIP SHA256 sidecar filename mismatch." }

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("gtt-human-review-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
try {
    Expand-Archive -Path $zipPath -DestinationPath $tempRoot -Force

    function Read-JsonRequired {
        param([Parameter(Mandatory=$true)][string]$RelativePath)
        $path = Join-Path $tempRoot $RelativePath
        if (-not (Test-Path $path -PathType Leaf)) { throw "Required sealed-candidate evidence missing: $RelativePath" }
        try { return (Get-Content -Raw $path | ConvertFrom-Json) }
        catch { throw "Invalid JSON in sealed-candidate evidence '$RelativePath': $($_.Exception.Message)" }
    }

    function Assert-Identity {
        param([Parameter(Mandatory=$true)][object]$Evidence,[Parameter(Mandatory=$true)][string]$Scope)
        if (-not ($Evidence.PSObject.Properties.Name -contains 'git_sha') -or [string]$Evidence.git_sha -ne $ExpectedGitSha) {
            throw "$Scope is not bound to exact candidate $ExpectedGitSha."
        }
        if (($Evidence.PSObject.Properties.Name -contains 'version') -and [string]$Evidence.version -ne $Version) {
            throw "$Scope version does not match '$Version'."
        }
    }

    $summary = Read-JsonRequired "WIN64_ACCEPTANCE_SUMMARY.json"
    Assert-Identity $summary "WIN64_ACCEPTANCE_SUMMARY.json"
    if ([string]$summary.result -ne "PASS") { throw "WIN64_ACCEPTANCE_SUMMARY.json is not PASS." }
    if ([string]$summary.configuration -ne $Configuration) { throw "Candidate configuration mismatch." }
    if ([string]$summary.human_visual_review -ne "REQUIRED") { throw "Candidate does not preserve the REQUIRED human-review boundary." }
    if ([bool]$summary.demo_release_authorized) { throw "Technical candidate illegally authorizes Demo Release." }

    $technical = Read-JsonRequired "DEMO_TECHNICAL_GATE.json"
    Assert-Identity $technical "DEMO_TECHNICAL_GATE.json"
    if ([string]$technical.result -ne "PASS") { throw "DEMO_TECHNICAL_GATE.json is not PASS." }

    $visual = Read-JsonRequired "DEMO_VISUAL_EVIDENCE.json"
    Assert-Identity $visual "DEMO_VISUAL_EVIDENCE.json"
    if ([string]$visual.result -ne "PASS" -or -not [bool]$visual.human_review_required) {
        throw "Rendered visual evidence is not a PASS awaiting human review."
    }
    if ([string]$visual.human_visual_acceptance -ne "NOT_PERFORMED") {
        throw "Rendered visual evidence unexpectedly claims human acceptance."
    }

    $chaos = Read-JsonRequired "NATIVE_CHAOS_RUNTIME.json"
    Assert-Identity $chaos "NATIVE_CHAOS_RUNTIME.json"
    if ([string]$chaos.result -ne "PASS") { throw "NATIVE_CHAOS_RUNTIME.json is not PASS." }

    $trailer = Read-JsonRequired "NATIVE_TRAILER_RUNTIME.json"
    Assert-Identity $trailer "NATIVE_TRAILER_RUNTIME.json"
    if ([string]$trailer.result -ne "PASS") { throw "NATIVE_TRAILER_RUNTIME.json is not PASS." }

    $expectedScenes = @('world_gameplay','law_pressure','native_vehicle','loaded_trailer','hud_overview')
    $sceneRecords = @()
    foreach ($scene in $expectedScenes) {
        $relative = "DemoVisualEvidence/GTT_visual_$scene.png"
        $path = Join-Path $tempRoot ($relative.Replace('/','\'))
        if (-not (Test-Path $path -PathType Leaf)) { throw "Human-review screenshot missing: $relative" }
        $item = Get-Item $path
        if ($item.Length -lt 20000) { throw "Human-review screenshot is suspiciously small: $relative" }
        $sceneRecords += [ordered]@{
            scene = $scene
            file = $relative
            bytes = [int64]$item.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -Path $path).Hash.ToLowerInvariant()
            human_decision = "PENDING"
        }
    }

    $criteria = @(
        [ordered]@{ id='coherent_gameplay_presentation'; status='PENDING'; prompt='Gameplay presentation is coherent and release-demo appropriate.' },
        [ordered]@{ id='readable_hud_ui'; status='PENDING'; prompt='HUD/UI is readable at the captured 1280x720 review resolution.' },
        [ordered]@{ id='no_placeholder_debug_clutter'; status='PENDING'; prompt='No obvious placeholder assets, debug clutter or accidental development overlays are visible.' },
        [ordered]@{ id='native_vehicle_visual_integrity'; status='PENDING'; prompt='Native tractor presentation appears visually intact in the rendered candidate.' },
        [ordered]@{ id='authored_trailer_wheels_hitch_visual_integrity'; status='PENDING'; prompt='Authored trailer wheel/hitch presentation appears visually intact in the loaded-trailer scene.' }
    )

    if ([string]::IsNullOrWhiteSpace($OutputPath)) { $OutputPath = "$zipPath.human-review-packet.json" }
    $OutputPath = [IO.Path]::GetFullPath($OutputPath)
    $packet = [ordered]@{
        schema = "gtt.demo-human-review-packet.v1"
        status = "READY_FOR_HUMAN_REVIEW"
        game = "Grand Theft Tractor"
        git_sha = $ExpectedGitSha
        version = $Version
        configuration = $Configuration
        platform = "Win64"
        engine = "Unreal Engine 5.8"
        archive_file = [IO.Path]::GetFileName($zipPath)
        archive_sha256 = $archiveHash
        canonical_archive_verification = "PASS"
        technical_gate = "PASS"
        rendered_visual_evidence = "PASS"
        native_chaos_runtime = "PASS"
        authored_trailer_runtime = "PASS"
        human_visual_review = "PENDING"
        demo_release_authorized = $false
        screenshots = $sceneRecords
        criteria = $criteria
        generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    }
    $packet | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath
    Write-Host "[GTT] Human review packet READY: $OutputPath"
    Write-Host "[GTT] This packet does NOT perform human acceptance and does NOT authorize Demo Release."
}
finally {
    if (Test-Path $tempRoot) { Remove-Item -Recurse -Force $tempRoot -ErrorAction SilentlyContinue }
}
