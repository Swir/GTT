param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = "",
    [string]$OutputPath = "",
    [int]$MinimumFreeGiB = 25
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($ProjectFile)) {
    $ProjectFile = Join-Path $ProjectRoot "GTT.uproject"
}
$ProjectFile = [System.IO.Path]::GetFullPath($ProjectFile)

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $ProjectRoot "Saved\Win64\WIN64_PREFLIGHT.json"
}
$OutputPath = [System.IO.Path]::GetFullPath($OutputPath)
$OutputParent = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $OutputParent | Out-Null

$checks = New-Object System.Collections.Generic.List[object]
$requiredFailure = $false

function Add-Check {
    param(
        [string]$Name,
        [bool]$Passed,
        [string]$Detail,
        [bool]$Required = $true
    )
    $script:checks.Add([ordered]@{
        name = $Name
        required = $Required
        passed = $Passed
        detail = $Detail
    })
    if ($Required -and -not $Passed) {
        $script:requiredFailure = $true
    }
}

function Command-Exists {
    param([string]$Name)
    return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Get-LatestVersionDirectory {
    param([string]$Root)

    if ([string]::IsNullOrWhiteSpace($Root) -or -not (Test-Path $Root -PathType Container)) {
        return $null
    }

    return @(
        Get-ChildItem -Path $Root -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^\d+(?:\.\d+)+$' } |
            Sort-Object { [version]$_.Name } -Descending
    ) | Select-Object -First 1
}

$isWindowsHost = $false
if (Get-Variable IsWindows -ErrorAction SilentlyContinue) {
    $isWindowsHost = [bool]$IsWindows
} else {
    $isWindowsHost = $env:OS -eq "Windows_NT"
}
Add-Check "windows-host" $isWindowsHost "OS=$([System.Environment]::OSVersion.VersionString)"

$engineExists = Test-Path $EngineRoot -PathType Container
Add-Check "engine-root" $engineExists $EngineRoot

$runUat = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$editorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$buildVersionPath = Join-Path $EngineRoot "Engine\Build\Build.version"
$ubtDll = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
$ubtExeLegacy = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool.exe"

Add-Check "run-uat" (Test-Path $runUat -PathType Leaf) $runUat
Add-Check "unreal-editor-cmd" (Test-Path $editorCmd -PathType Leaf) $editorCmd
Add-Check "unreal-build-tool" ((Test-Path $ubtDll -PathType Leaf) -or (Test-Path $ubtExeLegacy -PathType Leaf)) "dll=$ubtDll; legacy=$ubtExeLegacy"
Add-Check "engine-build-version-file" (Test-Path $buildVersionPath -PathType Leaf) $buildVersionPath

$engineVersion = "unknown"
if (Test-Path $buildVersionPath -PathType Leaf) {
    try {
        $buildVersion = Get-Content -Raw $buildVersionPath | ConvertFrom-Json
        $engineVersion = "$($buildVersion.MajorVersion).$($buildVersion.MinorVersion).$($buildVersion.PatchVersion)"
        Add-Check "engine-version-5.8" (($buildVersion.MajorVersion -eq 5) -and ($buildVersion.MinorVersion -eq 8)) $engineVersion
    } catch {
        Add-Check "engine-version-5.8" $false "Unable to parse Build.version: $($_.Exception.Message)"
    }
} else {
    Add-Check "engine-version-5.8" $false "Build.version unavailable"
}

$projectExists = Test-Path $ProjectFile -PathType Leaf
Add-Check "project-file" $projectExists $ProjectFile

$projectEngineAssociation = "unknown"
$chaosEnabled = $false
$runtimeModuleFound = $false
if ($projectExists) {
    try {
        $project = Get-Content -Raw $ProjectFile | ConvertFrom-Json
        $projectEngineAssociation = [string]$project.EngineAssociation
        $chaosPlugin = @($project.Plugins | Where-Object { $_.Name -eq "ChaosVehiclesPlugin" -and $_.Enabled -eq $true })
        $runtimeModule = @($project.Modules | Where-Object { $_.Name -eq "GTT" -and $_.Type -eq "Runtime" })
        $chaosEnabled = $chaosPlugin.Count -gt 0
        $runtimeModuleFound = $runtimeModule.Count -gt 0
        Add-Check "project-engine-association" ($projectEngineAssociation -eq "5.8") "EngineAssociation=$projectEngineAssociation"
        Add-Check "chaos-vehicles-plugin" $chaosEnabled "ChaosVehiclesPlugin enabled=$chaosEnabled"
        Add-Check "gtt-runtime-module" $runtimeModuleFound "GTT Runtime module found=$runtimeModuleFound"
    } catch {
        Add-Check "project-json" $false "Unable to parse GTT.uproject: $($_.Exception.Message)"
    }
}

$buildCs = Join-Path $ProjectRoot "Source\GTT\GTT.Build.cs"
Add-Check "gtt-build-cs" (Test-Path $buildCs -PathType Leaf) $buildCs

$gitAvailable = Command-Exists "git"
Add-Check "git" $gitAvailable ($(if ($gitAvailable) { (& git --version 2>$null) -join " " } else { "git not found" }))

$gitLfsAvailable = $false
$gitLfsVersion = "git-lfs not found"
if ($gitAvailable) {
    try {
        $gitLfsVersion = (& git lfs version 2>$null) -join " "
        $gitLfsAvailable = ($LASTEXITCODE -eq 0) -and -not [string]::IsNullOrWhiteSpace($gitLfsVersion)
    } catch { }
}
Add-Check "git-lfs" $gitLfsAvailable $gitLfsVersion

$programFilesX86 = ${env:ProgramFiles(x86)}
$vswhere = if ([string]::IsNullOrWhiteSpace($programFilesX86)) { "" } else { Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe" }
$vsInstall = ""
if (-not [string]::IsNullOrWhiteSpace($vswhere) -and (Test-Path $vswhere -PathType Leaf)) {
    try {
        $vsInstall = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null | Select-Object -First 1)
    } catch { }
}

$msvcToolsetVersion = "missing"
$clExe = ""
$linkExe = ""
if (-not [string]::IsNullOrWhiteSpace($vsInstall)) {
    $msvcRoot = Join-Path $vsInstall "VC\Tools\MSVC"
    $msvcToolset = Get-LatestVersionDirectory -Root $msvcRoot
    if ($null -ne $msvcToolset) {
        $msvcToolsetVersion = $msvcToolset.Name
        $clExe = Join-Path $msvcToolset.FullName "bin\Hostx64\x64\cl.exe"
        $linkExe = Join-Path $msvcToolset.FullName "bin\Hostx64\x64\link.exe"
    }
}
$clFound = -not [string]::IsNullOrWhiteSpace($clExe) -and (Test-Path $clExe -PathType Leaf)
$linkFound = -not [string]::IsNullOrWhiteSpace($linkExe) -and (Test-Path $linkExe -PathType Leaf)
$msvcReady = -not [string]::IsNullOrWhiteSpace($vsInstall) -and $clFound -and $linkFound
Add-Check "msvc-toolchain" $msvcReady "install=$vsInstall; toolset=$msvcToolsetVersion; cl=$clExe; link=$linkExe"

$windowsKits = if ([string]::IsNullOrWhiteSpace($programFilesX86)) { "" } else { Join-Path $programFilesX86 "Windows Kits\10" }
$windowsSdkVersion = "missing"
$rcExe = ""
$mtExe = ""
if (-not [string]::IsNullOrWhiteSpace($windowsKits) -and (Test-Path $windowsKits -PathType Container)) {
    $sdkBinRoot = Join-Path $windowsKits "bin"
    $sdkVersionDir = Get-LatestVersionDirectory -Root $sdkBinRoot
    if ($null -ne $sdkVersionDir) {
        $windowsSdkVersion = $sdkVersionDir.Name
        $rcExe = Join-Path $sdkVersionDir.FullName "x64\rc.exe"
        $mtExe = Join-Path $sdkVersionDir.FullName "x64\mt.exe"
    }
}
$rcFound = -not [string]::IsNullOrWhiteSpace($rcExe) -and (Test-Path $rcExe -PathType Leaf)
$mtFound = -not [string]::IsNullOrWhiteSpace($mtExe) -and (Test-Path $mtExe -PathType Leaf)
$windowsSdkFound = $rcFound -and $mtFound
Add-Check "windows-sdk" $windowsSdkFound "root=$windowsKits; version=$windowsSdkVersion; rc=$rcExe; mt=$mtExe"

$netFxSdkRoot = ""
$netFxCandidates = New-Object System.Collections.Generic.List[string]
if (-not [string]::IsNullOrWhiteSpace($env:UE_SDKS_ROOT)) {
    foreach ($version in @("4.6.2", "4.6.1", "4.6")) {
        $netFxCandidates.Add((Join-Path $env:UE_SDKS_ROOT "HostWin64\Win64\Windows Kits\NETFXSDK\$version"))
    }
}
foreach ($registryRoot in @(
    "HKCU:\SOFTWARE\Microsoft\Microsoft SDKs\NETFXSDK",
    "HKCU:\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\NETFXSDK",
    "HKLM:\SOFTWARE\Microsoft\Microsoft SDKs\NETFXSDK",
    "HKLM:\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\NETFXSDK"
)) {
    if (Test-Path $registryRoot) {
        foreach ($key in @(Get-ChildItem $registryRoot -ErrorAction SilentlyContinue | Sort-Object { try { [version]$_.PSChildName } catch { [version]"0.0" } } -Descending)) {
            $candidate = [string](Get-ItemPropertyValue -Path $key.PSPath -Name KitsInstallationFolder -ErrorAction SilentlyContinue)
            if (-not [string]::IsNullOrWhiteSpace($candidate)) { $netFxCandidates.Add($candidate) }
        }
    }
}
foreach ($candidate in $netFxCandidates) {
    if ((Test-Path (Join-Path $candidate "Include\um\mscoree.h") -PathType Leaf) -and
        (Test-Path (Join-Path $candidate "Lib\um\x64\mscoree.lib") -PathType Leaf)) {
        $netFxSdkRoot = $candidate
        break
    }
}
Add-Check "netfx-sdk" (-not [string]::IsNullOrWhiteSpace($netFxSdkRoot)) "root=$netFxSdkRoot; required=Include\um\mscoree.h + Lib\um\x64\mscoree.lib"

$freeGiB = -1.0
try {
    $rootPath = [System.IO.Path]::GetPathRoot($OutputPath)
    $drive = New-Object System.IO.DriveInfo($rootPath)
    $freeGiB = [Math]::Round($drive.AvailableFreeSpace / 1GB, 2)
    Add-Check "free-disk" ($freeGiB -ge $MinimumFreeGiB) "free=$freeGiB GiB; required=$MinimumFreeGiB GiB; drive=$rootPath"
} catch {
    Add-Check "free-disk" $false "Unable to inspect output drive: $($_.Exception.Message)"
}

$gitSha = "unknown"
if ($gitAvailable) {
    try {
        $candidate = (& git -C $ProjectRoot rev-parse HEAD 2>$null).Trim()
        if ($candidate) { $gitSha = $candidate }
    } catch { }
}

$passedCount = @($checks | Where-Object { $_.passed }).Count
$requiredCount = @($checks | Where-Object { $_.required }).Count
$requiredPassed = @($checks | Where-Object { $_.required -and $_.passed }).Count
$result = if ($requiredFailure) { "FAIL" } else { "PASS" }

$report = [ordered]@{
    evidence_schema = 2
    gate = "GTT_WIN64_UNREAL_PREFLIGHT"
    result = $result
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    git_sha = $gitSha
    host = [ordered]@{
        machine = $env:COMPUTERNAME
        os = [System.Environment]::OSVersion.VersionString
        powershell = $PSVersionTable.PSVersion.ToString()
    }
    unreal = [ordered]@{
        requested_root = $EngineRoot
        detected_version = $engineVersion
        project_engine_association = $projectEngineAssociation
    }
    capacity = [ordered]@{
        minimum_free_gib = $MinimumFreeGiB
        detected_free_gib = $freeGiB
    }
    checks = $checks
    summary = [ordered]@{
        passed = $passedCount
        total = $checks.Count
        required_passed = $requiredPassed
        required_total = $requiredCount
    }
}

$report | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $OutputPath

Write-Host "[GTT] Win64 Unreal preflight: $result ($requiredPassed/$requiredCount required checks)"
Write-Host "[GTT] Evidence: $OutputPath"
foreach ($check in $checks) {
    $mark = if ($check.passed) { "PASS" } elseif ($check.required) { "FAIL" } else { "WARN" }
    Write-Host "[GTT][$mark] $($check.name): $($check.detail)"
}

if ($requiredFailure) {
    exit 2
}
exit 0
