param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [ValidateSet("Development", "Shipping")]
    [string]$Configuration = "Shipping",
    [string]$ArchiveDirectory = "",
    [string]$Version = "",
    [switch]$SkipZip
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "GTT.uproject"
$GameConfig = Join-Path $ProjectRoot "Config\DefaultGame.ini"
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$ZenTool = Join-Path $EngineRoot "Engine\Binaries\Win64\zen.exe"
$ZenServer = Join-Path $EngineRoot "Engine\Binaries\Win64\zenserver.exe"
$Validator = Join-Path $PSScriptRoot "validate_windows_package.ps1"
$Preflight = Join-Path $PSScriptRoot "preflight_win64_unreal.ps1"

if (-not (Test-Path $ProjectFile)) { throw "GTT.uproject was not found at $ProjectFile" }
if (-not (Test-Path $GameConfig)) { throw "Config/DefaultGame.ini was not found at $GameConfig" }
if (-not (Test-Path $Validator)) { throw "Package validator was not found at $Validator" }
if (-not (Test-Path $Preflight)) { throw "Win64 Unreal preflight was not found at $Preflight" }

$gameIni = Get-Content -Raw $GameConfig
$versionMatch = [regex]::Match($gameIni, '(?m)^ProjectVersion=(.+)$')
if (-not $versionMatch.Success) { throw "ProjectVersion is missing from Config/DefaultGame.ini." }
$ProjectVersion = $versionMatch.Groups[1].Value.Trim()
if ([string]::IsNullOrWhiteSpace($ProjectVersion)) { throw "ProjectVersion in Config/DefaultGame.ini must not be empty." }
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = $ProjectVersion
} elseif ($Version -ne $ProjectVersion) {
    throw "Version '$Version' does not match ProjectVersion '$ProjectVersion'."
}

if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) { $ArchiveDirectory = Join-Path $ProjectRoot "Releases\GTT-$Version-Windows-$Configuration" }

$ArchiveDirectory = [System.IO.Path]::GetFullPath($ArchiveDirectory)
$ArchiveParent = Split-Path -Parent $ArchiveDirectory
New-Item -ItemType Directory -Force -Path $ArchiveParent | Out-Null
$PreflightReport = "$ArchiveDirectory.preflight.json"
$AttemptReport = "$ArchiveDirectory.attempt.json"

Write-Host "[GTT] Running Win64/UE 5.8 build preflight before touching the archive directory..."
& $Preflight -EngineRoot $EngineRoot -ProjectFile $ProjectFile -OutputPath $PreflightReport
$preflightExit = $LASTEXITCODE
if ($preflightExit -ne 0) { throw "Win64 Unreal preflight failed with exit code $preflightExit. Evidence: $PreflightReport" }
if (-not (Test-Path $RunUAT)) { throw "RunUAT.bat was not found after preflight. Expected: $RunUAT" }
if (-not (Test-Path $ZenTool)) { throw "zen.exe was not found after preflight. Expected: $ZenTool" }
if (-not (Test-Path $ZenServer)) { throw "zenserver.exe was not found after preflight. Expected: $ZenServer" }

if (Test-Path $ArchiveDirectory) { Remove-Item -Recurse -Force $ArchiveDirectory }
New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null

$gitSha = "unknown"
try {
    $candidate = (& git -C $ProjectRoot rev-parse HEAD 2>$null).Trim()
    if ($candidate) { $gitSha = $candidate }
} catch { }

$attempt = [ordered]@{
    evidence_schema = 2
    gate = "GTT_WIN64_BUILD_ATTEMPT"
    result = "RUNNING"
    game = "Grand Theft Tractor"
    version = $Version
    configuration = $Configuration
    platform = "Win64"
    engine_root = $EngineRoot
    git_sha = $gitSha
    started_utc = (Get-Date).ToUniversalTime().ToString("o")
    completed_utc = $null
    uat_exit_code = $null
    error = $null
}
$attempt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $AttemptReport

Write-Host "[GTT] Packaging Windows build..."
Write-Host "[GTT] Engine: $EngineRoot"
Write-Host "[GTT] Config: $Configuration"
Write-Host "[GTT] Version: $Version"
Write-Host "[GTT] Output: $ArchiveDirectory"

$uatExit = -1
$startedZen = $false
$zenProcess = $null
try {
    # Editor import commandlets auto-launch a sponsored Zen instance which exits
    # as soon as its sponsor process ends. A health probe can race that shutdown:
    # the cooker sees a ready server and then loses it while deleting its oplog.
    # Always replace any inherited instance with an explicitly managed server.
    # Launch the engine-bundled server directly. The zen.exe launcher uses a
    # per-user installed copy and ProgramData state, which is unavailable to a
    # locked-down self-hosted runner even though the project workspace is
    # writable. Project-local state also keeps the exact-candidate run isolated.
    $ZenRoot = Join-Path $ProjectRoot "Saved\Zen\Package"
    $ZenDataRoot = Join-Path $ZenRoot "Data"
    $ZenSystemRoot = Join-Path $ZenRoot "System"
    $ZenLog = Join-Path $ZenRoot "zenserver.log"
    New-Item -ItemType Directory -Force -Path $ZenDataRoot, $ZenSystemRoot | Out-Null
    Write-Host "[GTT] Restarting a dedicated local UE 5.8 Zen server for cook/stage..."
    & $ZenTool down 2>&1 | ForEach-Object { Write-Host "[Zen] $_" }
    Start-Sleep -Seconds 2
    $ZenArgs = @(
        "--data-dir=`"$ZenDataRoot`""
        "--system-dir=`"$ZenSystemRoot`""
        "--port=8558"
        "--http=asio"
        "--http-forceloopback"
        "--detach=false"
        "--no-sentry"
        "--quiet"
        "--abslog=`"$ZenLog`""
    )
    $zenProcess = Start-Process -FilePath $ZenServer -ArgumentList $ZenArgs -PassThru -WindowStyle Hidden
    $startedZen = $true
    $zenReady = $false
    for ($probe = 0; $probe -lt 20; $probe++) {
        Start-Sleep -Milliseconds 500
        if ($zenProcess.HasExited) { break }
        try {
            $zenHealth = Invoke-WebRequest -UseBasicParsing -Uri "http://127.0.0.1:8558/health/ready" -TimeoutSec 2
            if ($zenHealth.StatusCode -eq 200) {
                $zenReady = $true
                break
            }
        } catch { }
    }
    if (-not $zenReady) { throw "The dedicated Zen server did not become ready on 127.0.0.1:8558. See $ZenLog" }

    $UATLog = Join-Path $ProjectRoot "Saved\Logs\GTT-Package-UAT.log"
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $UATLog) | Out-Null
    $UATArgs = @(
        "BuildCookRun"
        "-project=$ProjectFile"
        "-noP4"
        "-platform=Win64"
        "-target=GTT"
        "-clientconfig=$Configuration"
        "-build"
        "-cook"
        "-stage"
        "-pak"
        "-iostore"
        "-NoZenAutoLaunch=127.0.0.1:8558"
        "-prereqs"
        "-nodebuginfo"
        "-archive"
        "-archivedirectory=$ArchiveDirectory"
        "-utf8output"
    )
    $UATOutput = @(& $RunUAT @UATArgs 2>&1 | Tee-Object -FilePath $UATLog)

    $uatExit = $LASTEXITCODE
    # Treat only the Unreal log severity field as an error. Warning messages can
    # legitimately contain transport diagnostics such as "ErrorCode" or
    # "Error:" in their body while UAT still completes successfully.
    $ErrorSeverityPattern = '(?i)(?:^|\]\s*)(?:Log[^:\r\n]+:\s+)?(?:Error|Fatal):'
    $ErrorLines = [string[]]@($UATOutput | ForEach-Object { [string]$_ } | Where-Object { $_ -match $ErrorSeverityPattern })
    $ErrorLineCount = ($ErrorLines | Measure-Object).Count
    if ($ErrorLineCount -gt 0) {
        throw "UAT emitted Error/Fatal log lines. First error: $($ErrorLines[0])"
    }
    if ($uatExit -ne 0) { throw "Unreal Automation Tool failed with exit code $uatExit" }
    $attempt.result = "PASS"
} catch {
    $attempt.result = "FAIL"
    $attempt.error = $_.Exception.Message
    if ($uatExit -ge 0) { $attempt.uat_exit_code = $uatExit }
    $attempt.completed_utc = (Get-Date).ToUniversalTime().ToString("o")
    $attempt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $AttemptReport
    throw
} finally {
    if ($startedZen) {
        Write-Host "[GTT] Stopping the local Zen server started for this package run..."
        & $ZenTool down
        if ($null -ne $zenProcess -and -not $zenProcess.HasExited) {
            Wait-Process -Id $zenProcess.Id -Timeout 15 -ErrorAction SilentlyContinue
        }
        if ($null -ne $zenProcess -and -not $zenProcess.HasExited) {
            Stop-Process -Id $zenProcess.Id -Force
        }
    }
}

$attempt.uat_exit_code = $uatExit
$attempt.completed_utc = (Get-Date).ToUniversalTime().ToString("o")
$attempt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $AttemptReport

Copy-Item -Force $PreflightReport (Join-Path $ArchiveDirectory "WIN64_PREFLIGHT.json")
Copy-Item -Force $AttemptReport (Join-Path $ArchiveDirectory "BUILD_ATTEMPT.json")

$buildInfo = [ordered]@{
    game = "Grand Theft Tractor"
    version = $Version
    configuration = $Configuration
    platform = "Win64"
    engine = Split-Path -Leaf $EngineRoot
    git_sha = $gitSha
    built_utc = (Get-Date).ToUniversalTime().ToString("o")
    evidence_schema = 2
    preflight = "WIN64_PREFLIGHT.json"
    build_attempt = "BUILD_ATTEMPT.json"
}
$buildInfoPath = Join-Path $ArchiveDirectory "BUILD_INFO.json"
$buildInfo | ConvertTo-Json | Set-Content -Encoding UTF8 $buildInfoPath

if ($Configuration -eq "Shipping") {
    $debugArtifacts = @(Get-ChildItem -Path $ArchiveDirectory -Recurse -File | Where-Object { $_.Extension -in '.pdb', '.exp', '.lib' })
    $debugArtifactCount = ($debugArtifacts | Measure-Object).Count
    if ($debugArtifactCount -gt 0) {
        $debugArtifacts | Remove-Item -Force
        Write-Host "[GTT] Removed $debugArtifactCount debug/linker artifact(s) from the Shipping archive."
    }
}

& $Validator -PackageDirectory $ArchiveDirectory -Configuration $Configuration -Version $Version
if ($LASTEXITCODE -ne 0) { throw "Package validation failed with exit code $LASTEXITCODE" }

$hashFile = Join-Path $ArchiveDirectory "SHA256SUMS.txt"
$filesToHash = Get-ChildItem -Path $ArchiveDirectory -Recurse -File | Where-Object { $_.FullName -ne $hashFile } | Sort-Object FullName
$hashLines = foreach ($file in $filesToHash) {
    $relative = [System.IO.Path]::GetRelativePath($ArchiveDirectory, $file.FullName).Replace('\','/')
    $hash = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
    "$hash  $relative"
}
$hashLines | Set-Content -Encoding ASCII $hashFile

if (-not $SkipZip) {
    $zipPath = "$ArchiveDirectory.zip"
    if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
    Compress-Archive -Path (Join-Path $ArchiveDirectory '*') -DestinationPath $zipPath -CompressionLevel Optimal
    $zipHash = (Get-FileHash -Algorithm SHA256 -Path $zipPath).Hash.ToLowerInvariant()
    Set-Content -Encoding ASCII -Path "$zipPath.sha256" -Value "$zipHash  $([IO.Path]::GetFileName($zipPath))"
    Write-Host "[GTT] Release ZIP: $zipPath"
}

Write-Host "[GTT] Windows package finished and validated successfully."
Write-Host "[GTT] Build: $ArchiveDirectory"
Write-Host "[GTT] Manifest: $hashFile"
Write-Host "[GTT] Preflight evidence: $(Join-Path $ArchiveDirectory 'WIN64_PREFLIGHT.json')"
Write-Host "[GTT] Build attempt evidence: $(Join-Path $ArchiveDirectory 'BUILD_ATTEMPT.json')"
