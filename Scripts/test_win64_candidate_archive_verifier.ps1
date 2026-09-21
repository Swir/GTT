param()

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Verifier = Join-Path $PSScriptRoot "verify_win64_candidate_archive.ps1"
if (-not (Test-Path $Verifier -PathType Leaf)) { throw "Archive verifier missing: $Verifier" }

$Sha = "0123456789abcdef0123456789abcdef01234567"
$Version = "0.1.68"
$Configuration = "Shipping"
$TestRoot = Join-Path ([IO.Path]::GetTempPath()) ("gtt-archive-contract-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $TestRoot | Out-Null

function Get-RelativeSlashPath {
    param([string]$Root, [string]$Path)
    return [IO.Path]::GetRelativePath($Root, $Path).Replace('\','/')
}

function Write-Fixture {
    param([Parameter(Mandatory=$true)][string]$Name)
    $package = Join-Path $TestRoot $Name
    New-Item -ItemType Directory -Path $package | Out-Null

    $exe = Join-Path $package "GTT.exe"
    [IO.File]::WriteAllBytes($exe, [Text.Encoding]::UTF8.GetBytes("fake-win64-exe-$Name"))
    $runtime = Join-Path $package "RUNTIME_SMOKE.json"
    [ordered]@{
        result = "PASS"
        git_sha = $Sha
        version = $Version
    } | ConvertTo-Json | Set-Content -Encoding UTF8 $runtime

    $evidence = @()
    foreach ($path in @($exe, $runtime)) {
        $file = Get-Item $path
        $evidence += [ordered]@{
            path = Get-RelativeSlashPath -Root $package -Path $file.FullName
            bytes = [int64]$file.Length
            sha256 = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
        }
    }

    $attestation = [ordered]@{
        schema = "gtt.win64-candidate-attestation.v1"
        result = "PASS"
        git_sha = $Sha
        version = $Version
        configuration = $Configuration
        platform = "Win64"
        engine = "Unreal Engine 5.8"
        packaged_exe = "GTT.exe"
        packaged_exe_sha256 = (Get-FileHash -Algorithm SHA256 -Path $exe).Hash.ToLowerInvariant()
        evidence_hash_algorithm = "SHA256"
        evidence_file_count = $evidence.Count
        evidence_files = $evidence
        native_authority_runtime = "PASS"
        native_authority_faults = 0
        fieldmaster_hill_haul_runtime = "PASS"
        fieldmaster_hud_runtime = "PASS"
        technical_gate_schema = 17
        technical_gate = "PASS"
        rendered_visual_evidence = "PASS"
        rendered_screenshot_count = 5
        human_visual_review = "REQUIRED"
        demo_release_authorized = $false
    }
    $attestation | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 (Join-Path $package "WIN64_CANDIDATE_ATTESTATION.json")
    return $package
}

function Seal-Fixture {
    param([Parameter(Mandatory=$true)][string]$Package)
    $manifest = Join-Path $Package "FINAL_SHA256SUMS.txt"
    $lines = foreach ($file in (Get-ChildItem -Path $Package -Recurse -File | Where-Object { $_.FullName -ne $manifest } | Sort-Object FullName)) {
        $relative = Get-RelativeSlashPath -Root $Package -Path $file.FullName
        $hash = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
        "$hash  $relative"
    }
    $lines | Set-Content -Encoding ASCII $manifest
    $zip = "$Package.zip"
    if (Test-Path $zip) { Remove-Item -Force $zip }
    Compress-Archive -Path (Join-Path $Package "*") -DestinationPath $zip -CompressionLevel Optimal
    $hash = (Get-FileHash -Algorithm SHA256 -Path $zip).Hash.ToLowerInvariant()
    Set-Content -Encoding ASCII -Path "$zip.sha256" -Value "$hash  $([IO.Path]::GetFileName($zip))"
    return $zip
}

function Invoke-Pass {
    param([string]$Package)
    & $Verifier -PackageDirectory $Package -Version $Version -Configuration $Configuration -ExpectedGitSha $Sha
    $verificationPath = "$Package.zip.verify.json"
    if (-not (Test-Path $verificationPath -PathType Leaf)) { throw "PASS fixture did not write verification evidence." }
    $verification = Get-Content -Raw $verificationPath | ConvertFrom-Json
    if ($verification.result -ne "PASS" -or $verification.git_sha -ne $Sha -or $verification.human_visual_review -ne "REQUIRED" -or [bool]$verification.demo_release_authorized) {
        throw "PASS fixture verification output crossed an identity/release boundary."
    }
}

function Invoke-ExpectedFailure {
    param([Parameter(Mandatory=$true)][scriptblock]$Action, [Parameter(Mandatory=$true)][string]$Needle)
    $failed = $false
    try { & $Action }
    catch {
        $failed = $true
        if ($_.Exception.Message -notlike "*$Needle*") {
            throw "Expected failure containing '$Needle', got: $($_.Exception.Message)"
        }
    }
    if (-not $failed) { throw "Expected archive verification failure containing '$Needle'." }
}

try {
    $passPackage = Write-Fixture -Name "pass"
    [void](Seal-Fixture -Package $passPackage)
    Invoke-Pass -Package $passPackage

    $hashPackage = Write-Fixture -Name "bad-sidecar"
    $hashZip = Seal-Fixture -Package $hashPackage
    Set-Content -Encoding ASCII -Path "$hashZip.sha256" -Value ("0" * 64 + "  " + [IO.Path]::GetFileName($hashZip))
    Invoke-ExpectedFailure -Needle "sidecar hash does not match" -Action {
        & $Verifier -PackageDirectory $hashPackage -Version $Version -Configuration $Configuration -ExpectedGitSha $Sha
    }

    $extraPackage = Write-Fixture -Name "unmanifested"
    $extraManifest = Join-Path $extraPackage "FINAL_SHA256SUMS.txt"
    [void](Seal-Fixture -Package $extraPackage)
    # Add a file after the manifest was generated, then rebuild and re-hash the ZIP.
    Set-Content -Encoding ASCII -Path (Join-Path $extraPackage "UNSEALED.bin") -Value "unsealed"
    $extraZip = "$extraPackage.zip"
    Remove-Item -Force $extraZip
    Compress-Archive -Path (Join-Path $extraPackage "*") -DestinationPath $extraZip -CompressionLevel Optimal
    $extraHash = (Get-FileHash -Algorithm SHA256 -Path $extraZip).Hash.ToLowerInvariant()
    Set-Content -Encoding ASCII -Path "$extraZip.sha256" -Value "$extraHash  $([IO.Path]::GetFileName($extraZip))"
    Invoke-ExpectedFailure -Needle "does not exactly match" -Action {
        & $Verifier -PackageDirectory $extraPackage -Version $Version -Configuration $Configuration -ExpectedGitSha $Sha
    }

    $boundaryPackage = Write-Fixture -Name "bad-boundary"
    $attPath = Join-Path $boundaryPackage "WIN64_CANDIDATE_ATTESTATION.json"
    $att = Get-Content -Raw $attPath | ConvertFrom-Json
    $att.human_visual_review = "PASS"
    $att.demo_release_authorized = $true
    $att | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $attPath
    [void](Seal-Fixture -Package $boundaryPackage)
    Invoke-ExpectedFailure -Needle "human visual review / Demo Release boundary" -Action {
        & $Verifier -PackageDirectory $boundaryPackage -Version $Version -Configuration $Configuration -ExpectedGitSha $Sha
    }

    Write-Host "[GTT][ARCHIVE-TEST] PASS: positive round-trip plus sidecar/full-tree/human-boundary negative cases."
}
finally {
    if (Test-Path $TestRoot) { Remove-Item -Recurse -Force $TestRoot }
}
