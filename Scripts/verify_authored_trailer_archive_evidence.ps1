param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$Version,
    [ValidateSet("Development", "Shipping")]
    [Parameter(Mandatory=$true)][string]$Configuration,
    [Parameter(Mandatory=$true)][string]$ExpectedGitSha
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
if ($ExpectedGitSha -notmatch '^[0-9a-fA-F]{40}$') { throw "ExpectedGitSha must be an exact 40-character Git SHA." }
$ExpectedGitSha = $ExpectedGitSha.ToLowerInvariant()

$ZipPath = "$PackageDirectory.zip"
$GenericVerificationPath = "$ZipPath.verify.json"
$VerificationPath = "$ZipPath.trailer-editor-verify.json"

foreach ($path in @($ZipPath, $GenericVerificationPath)) {
    if (-not (Test-Path $path -PathType Leaf)) { throw "Required sealed candidate evidence missing: $path" }
}
if (Test-Path $VerificationPath) { Remove-Item -Force $VerificationPath }

$generic = Get-Content -Raw $GenericVerificationPath | ConvertFrom-Json
if ([string]$generic.schema -ne "gtt.win64-candidate-archive-verification.v1" -or
    [string]$generic.result -ne "PASS" -or
    [string]$generic.git_sha -ne $ExpectedGitSha -or
    [string]$generic.version -ne $Version -or
    [string]$generic.configuration -ne $Configuration -or
    [string]$generic.platform -ne "Win64" -or
    [string]$generic.engine -ne "Unreal Engine 5.8") {
    throw "Generic sealed-archive verification does not match this exact UE 5.8 Win64 candidate."
}
$zipHash = (Get-FileHash -Algorithm SHA256 -Path $ZipPath).Hash.ToLowerInvariant()
if ([string]$generic.archive_sha256 -ne $zipHash) {
    throw "Generic sealed-archive verification is not bound to the current archive bytes."
}

function Read-ZipJsonRequired {
    param(
        [Parameter(Mandatory=$true)][System.IO.Compression.ZipArchive]$Archive,
        [Parameter(Mandatory=$true)][string]$Name
    )
    $matches = @($Archive.Entries | Where-Object { $_.FullName.Replace('\','/') -eq $Name })
    if ($matches.Count -ne 1) { throw "Expected exactly one '$Name' in sealed archive, found $($matches.Count)." }
    $stream = $matches[0].Open()
    $reader = New-Object System.IO.StreamReader($stream, [Text.Encoding]::UTF8, $true)
    try {
        $text = $reader.ReadToEnd()
    }
    finally {
        $reader.Dispose()
        $stream.Dispose()
    }
    try { return ($text | ConvertFrom-Json) }
    catch { throw "Invalid JSON in sealed archive '$Name': $($_.Exception.Message)" }
}

function Assert-ExactStringSet {
    param(
        [Parameter(Mandatory=$true)][object[]]$Actual,
        [Parameter(Mandatory=$true)][string[]]$Expected,
        [Parameter(Mandatory=$true)][string]$Scope
    )
    $actualStrings = @($Actual | ForEach-Object { [string]$_ } | Sort-Object -Unique)
    $expectedStrings = @($Expected | Sort-Object -Unique)
    if ($actualStrings.Count -ne $expectedStrings.Count) {
        throw "$Scope count mismatch: expected $($expectedStrings.Count), got $($actualStrings.Count)."
    }
    for ($i = 0; $i -lt $expectedStrings.Count; $i++) {
        if ($actualStrings[$i] -ne $expectedStrings[$i]) {
            throw "$Scope mismatch: expected '$($expectedStrings -join ',')', got '$($actualStrings -join ',')'."
        }
    }
}

function Assert-EditorEvidence {
    param(
        [Parameter(Mandatory=$true)][object]$Evidence,
        [Parameter(Mandatory=$true)][string]$Scope
    )
    if ([string]$Evidence.schema -ne "gtt.authored-trailer-editor-acceptance.v1" -or [string]$Evidence.result -ne "PASS") {
        throw "$Scope is not PASS schema gtt.authored-trailer-editor-acceptance.v1."
    }
    if ([string]$Evidence.git_sha -ne $ExpectedGitSha) { throw "$Scope git_sha does not match exact candidate." }
    if ([string]$Evidence.source_gltf_sha256 -notmatch '^[0-9a-f]{64}$') { throw "$Scope source_gltf_sha256 is invalid." }
    if ([string]::IsNullOrWhiteSpace([string]$Evidence.skeletal_mesh_object_path) -or
        -not ([string]$Evidence.skeletal_mesh_object_path).StartsWith('/Game/GTT/Vehicles/Trailer/')) {
        throw "$Scope skeletal mesh object path is outside the final authored trailer lane."
    }
    if ([string]::IsNullOrWhiteSpace([string]$Evidence.skeleton_object_path) -or
        -not ([string]$Evidence.skeleton_object_path).StartsWith('/Game/GTT/Vehicles/Trailer/')) {
        throw "$Scope skeleton object path is outside the final authored trailer lane."
    }
    if ([string]::IsNullOrWhiteSpace([string]$Evidence.physics_asset_object_path) -or
        -not ([string]$Evidence.physics_asset_object_path).StartsWith('/Game/GTT/Vehicles/Trailer/')) {
        throw "$Scope PhysicsAsset object path is outside the final authored trailer lane."
    }
    Assert-ExactStringSet @($Evidence.required_bones) @('body','wheel_l','wheel_r') "$Scope required_bones"
    Assert-ExactStringSet @($Evidence.verified_bones) @('body','wheel_l','wheel_r') "$Scope verified_bones"
    Assert-ExactStringSet @($Evidence.required_sockets) @('socket_hitch','socket_cargo','socket_axle_l','socket_axle_r') "$Scope required_sockets"
    Assert-ExactStringSet @($Evidence.verified_sockets) @('socket_hitch','socket_cargo','socket_axle_l','socket_axle_r') "$Scope verified_sockets"
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($ZipPath)
try {
    $editor = Read-ZipJsonRequired $archive "AUTHORED_TRAILER_EDITOR_ACCEPTANCE.json"
    $import = Read-ZipJsonRequired $archive "AUTHORED_TRAILER_IMPORT.json"

    Assert-EditorEvidence $editor "AUTHORED_TRAILER_EDITOR_ACCEPTANCE.json"

    if ([string]$import.schema -ne "gtt.authored-trailer-import.v1" -or [string]$import.result -ne "PASS") {
        throw "AUTHORED_TRAILER_IMPORT.json is not PASS schema gtt.authored-trailer-import.v1."
    }
    if ([string]$import.git_sha -ne $ExpectedGitSha -or [string]$import.version -ne $Version) {
        throw "AUTHORED_TRAILER_IMPORT.json does not match exact candidate identity."
    }
    if (-not ($import.PSObject.Properties.Name -contains 'editor_acceptance') -or $null -eq $import.editor_acceptance) {
        throw "AUTHORED_TRAILER_IMPORT.json does not embed editor_acceptance evidence."
    }
    Assert-EditorEvidence $import.editor_acceptance "AUTHORED_TRAILER_IMPORT.json.editor_acceptance"

    foreach ($field in @(
        'git_sha',
        'source_gltf_sha256',
        'skeletal_mesh_object_path',
        'skeleton_object_path',
        'physics_asset_object_path'
    )) {
        if ([string]$editor.$field -ne [string]$import.editor_acceptance.$field) {
            throw "Standalone/nested authored trailer editor evidence mismatch for '$field'."
        }
    }

    foreach ($field in @('required_bones','verified_bones','required_sockets','verified_sockets')) {
        $standalone = @($editor.$field | ForEach-Object { [string]$_ } | Sort-Object -Unique) -join ','
        $nested = @($import.editor_acceptance.$field | ForEach-Object { [string]$_ } | Sort-Object -Unique) -join ','
        if ($standalone -ne $nested) { throw "Standalone/nested authored trailer editor evidence mismatch for '$field'." }
    }

    $verification = [ordered]@{
        schema = "gtt.authored-trailer-archive-evidence.v1"
        result = "PASS"
        git_sha = $ExpectedGitSha
        version = $Version
        configuration = $Configuration
        platform = "Win64"
        engine = "Unreal Engine 5.8"
        archive_sha256 = $zipHash
        authored_skeletal_mesh = "PASS"
        authored_skeleton = "PASS"
        authored_physics_asset = "PASS"
        final_hitch_socket = "PASS"
        final_axle_sockets = "PASS"
        skeletal_wheel_bones = "PASS"
        source_gltf_sha256 = [string]$editor.source_gltf_sha256
        human_visual_review = "REQUIRED"
        demo_release_authorized = $false
        verified_utc = (Get-Date).ToUniversalTime().ToString("o")
    }
    $verification | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 $VerificationPath
}
finally {
    $archive.Dispose()
}

Write-Host "[GTT][TRAILER-ARCHIVE] PASS: sealed candidate preserves exact authored skeletal trailer, wheel-bone, PhysicsAsset and final socket editor evidence."
Write-Host "[GTT][TRAILER-ARCHIVE] Human/rendered acceptance remains separate and REQUIRED."
Write-Host "[GTT][TRAILER-ARCHIVE] Verification: $VerificationPath"
