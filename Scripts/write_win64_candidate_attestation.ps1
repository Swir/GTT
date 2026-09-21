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
if (-not (Test-Path $PackageDirectory -PathType Container)) { throw "Package directory does not exist: $PackageDirectory" }
if ([string]::IsNullOrWhiteSpace($Version)) { throw "Version must not be empty." }
if ($ExpectedGitSha -notmatch '^[0-9a-fA-F]{40}$') { throw "ExpectedGitSha must be an exact 40-character Git SHA." }
$ExpectedGitSha = $ExpectedGitSha.ToLowerInvariant()

function Read-JsonRequired {
    param([Parameter(Mandatory=$true)][string]$Name)
    $path = Join-Path $PackageDirectory $Name
    if (-not (Test-Path $path -PathType Leaf)) { throw "Required candidate evidence missing: $Name" }
    try { return (Get-Content -Raw $path | ConvertFrom-Json) }
    catch { throw "Invalid JSON in required candidate evidence '$Name': $($_.Exception.Message)" }
}

function Assert-ExactIdentity {
    param([Parameter(Mandatory=$true)][object]$Object, [Parameter(Mandatory=$true)][string]$Scope)
    if (-not ($Object.PSObject.Properties.Name -contains 'git_sha') -or [string]$Object.git_sha -ne $ExpectedGitSha) {
        throw "$Scope git_sha does not match exact candidate $ExpectedGitSha."
    }
    if (($Object.PSObject.Properties.Name -contains 'version') -and [string]$Object.version -ne $Version) {
        throw "$Scope version '$($Object.version)' does not match candidate '$Version'."
    }
}

$buildInfo = Read-JsonRequired "BUILD_INFO.json"
Assert-ExactIdentity $buildInfo "BUILD_INFO.json"
if ([string]$buildInfo.configuration -ne $Configuration) { throw "BUILD_INFO.json configuration does not match '$Configuration'." }
if ([string]$buildInfo.platform -ne "Win64") { throw "BUILD_INFO.json platform must be Win64." }

$attempt = Read-JsonRequired "BUILD_ATTEMPT.json"
Assert-ExactIdentity $attempt "BUILD_ATTEMPT.json"
if ([string]$attempt.result -ne "PASS") { throw "BUILD_ATTEMPT.json is not PASS." }

$import = Read-JsonRequired "AUTHORED_TRAILER_IMPORT.json"
Assert-ExactIdentity $import "AUTHORED_TRAILER_IMPORT.json"
if ([string]$import.result -ne "PASS") { throw "AUTHORED_TRAILER_IMPORT.json is not PASS." }

$runtime = Read-JsonRequired "RUNTIME_SMOKE.json"
Assert-ExactIdentity $runtime "RUNTIME_SMOKE.json"
if ([string]$runtime.result -ne "PASS") { throw "RUNTIME_SMOKE.json is not PASS." }

$scenario = Read-JsonRequired "DEMO_SCENARIO.json"
Assert-ExactIdentity $scenario "DEMO_SCENARIO.json"
if ([string]$scenario.result -ne "PASS") { throw "DEMO_SCENARIO.json is not PASS." }

# FINISH-FIRST current-target evidence: do not accept a generic PASS token for the
# Native Chaos roadmap gates. Bind the final attestation to the concrete movement,
# four-wheel/suspension and no-fallback observations produced by the packaged EXE.
$chaos = Read-JsonRequired "NATIVE_CHAOS_RUNTIME.json"
Assert-ExactIdentity $chaos "NATIVE_CHAOS_RUNTIME.json"
if ([string]$chaos.schema -ne "gtt.native-chaos-runtime.v1" -or [string]$chaos.result -ne "PASS") {
    throw "NATIVE_CHAOS_RUNTIME.json is not PASS schema v1."
}
if (-not [bool]$chaos.native_physics_accepted) { throw "Native Chaos physics was not accepted for the Fieldmaster." }
if (-not [bool]$chaos.wheel_setup_observed) { throw "Native Chaos wheel setup evidence is missing." }
if (-not [bool]$chaos.native_smoke_ready) { throw "Native Chaos smoke-ready evidence is missing." }
if (-not [bool]$chaos.deterministic_fieldmaster_motion -or -not [bool]$chaos.deterministic_fieldmaster_control) {
    throw "Native Chaos Fieldmaster movement/control was not deterministically proven."
}
if ([bool]$chaos.physics_fallback_observed) { throw "Native physics fallback was observed in the exact candidate." }
if ([int]$chaos.movement_active_samples -lt 2) { throw "Native Chaos movement lacks two active packaged-runtime samples." }
if ([int]$chaos.command_samples -lt 1) { throw "Native Chaos movement lacks a non-zero driver command sample." }
if ([int]$chaos.max_valid_wheels -lt 4) { throw "Native Chaos did not expose all four valid Fieldmaster wheels." }
if ([int]$chaos.max_contacts -lt 2) { throw "Native Chaos did not prove sufficient wheel contact." }
if ([int]$chaos.max_suspension_samples -lt 4 -or [int]$chaos.suspension_ready_samples -lt 1) {
    throw "Native Chaos did not prove complete four-wheel suspension telemetry."
}
if ([double]$chaos.max_speed_kmh -lt 0.35) { throw "Native Chaos Fieldmaster movement stayed below the acceptance speed floor." }
if ([int]$chaos.configured_forward_gears -lt 2 -or [int]$chaos.unsafe_direction_shift_commits -ne 0) {
    throw "Native Chaos drivetrain configuration/safe-shift evidence is insufficient."
}

$authority = Read-JsonRequired "NATIVE_AUTHORITY_RUNTIME.json"
Assert-ExactIdentity $authority "NATIVE_AUTHORITY_RUNTIME.json"
if ([string]$authority.result -ne "PASS") { throw "NATIVE_AUTHORITY_RUNTIME.json is not PASS." }
if ([string]$authority.authority -ne "NATIVE_CHAOS") { throw "NATIVE_AUTHORITY_RUNTIME.json authority must be NATIVE_CHAOS." }
if ([int]$authority.authority_faults -ne 0) { throw "NATIVE_AUTHORITY_RUNTIME.json contains split-authority faults." }

# The drivetrain acceptance is a separate canonical roadmap gate. Seal the exact
# packaged forward/automatic/reverse/forward scenario instead of merely hashing it.
$drivetrain = Read-JsonRequired "NATIVE_DRIVETRAIN_SCENARIO.json"
Assert-ExactIdentity $drivetrain "NATIVE_DRIVETRAIN_SCENARIO.json"
if ([string]$drivetrain.schema -ne "gtt.native-drivetrain-scenario.v1" -or [string]$drivetrain.result -ne "PASS") {
    throw "NATIVE_DRIVETRAIN_SCENARIO.json is not PASS schema v1."
}
if ([int]$drivetrain.diagnostic_failure_count -ne 0) { throw "Native drivetrain scenario contains diagnostic failures." }
if ([int]$drivetrain.max_forward_gear_observed -lt 2 -or [int]$drivetrain.automatic_upshift_gear -lt 2) {
    throw "Native drivetrain scenario did not prove a real automatic upshift."
}
if ([double]$drivetrain.reverse_interlock_speed_kmh -le 3.5) { throw "Native drivetrain reverse interlock was not exercised above release speed." }
if ([double]$drivetrain.reverse_commit_speed_kmh -gt 3.75 -or [double]$drivetrain.forward_commit_speed_kmh -gt 3.75) {
    throw "Native drivetrain direction change committed outside the safe release window."
}
if ([double]$drivetrain.reverse_motion_signed_speed_kmh -gt -4.5 -or [int]$drivetrain.reverse_motion_gear -ge 0) {
    throw "Native drivetrain scenario did not prove measurable reverse motion in reverse gear."
}
if ([double]$drivetrain.forward_motion_signed_speed_kmh -lt 1.5 -or [int]$drivetrain.forward_motion_gear -le 0) {
    throw "Native drivetrain scenario did not prove a safe return to forward motion."
}

$hillHaul = Read-JsonRequired "FIELDMASTER_HILL_HAUL_RUNTIME.json"
Assert-ExactIdentity $hillHaul "FIELDMASTER_HILL_HAUL_RUNTIME.json"
if ([string]$hillHaul.result -ne "PASS") { throw "FIELDMASTER_HILL_HAUL_RUNTIME.json is not PASS." }
if ([int]$hillHaul.loaded_trailer_samples -lt 2) { throw "FIELDMASTER_HILL_HAUL_RUNTIME.json lacks loaded trailer evidence." }
if ([int]$hillHaul.assist_samples -lt 1) { throw "FIELDMASTER_HILL_HAUL_RUNTIME.json lacks hill-assist evidence." }
if ([int]$hillHaul.thermal_samples -lt 1) { throw "FIELDMASTER_HILL_HAUL_RUNTIME.json lacks thermal behavior evidence." }

$hud = Read-JsonRequired "FIELDMASTER_HUD_RUNTIME.json"
Assert-ExactIdentity $hud "FIELDMASTER_HUD_RUNTIME.json"
if ([string]$hud.schema -ne "gtt.fieldmaster-hud-runtime.v1" -or [string]$hud.result -ne "PASS") { throw "FIELDMASTER_HUD_RUNTIME.json is not PASS schema v1." }
if ([int]$hud.telemetry_samples -lt 2) { throw "FIELDMASTER_HUD_RUNTIME.json lacks telemetry coverage." }
if ([int]$hud.visible_alert_samples -lt 1) { throw "FIELDMASTER_HUD_RUNTIME.json lacks driver-visible safety alert coverage." }
if (([int]$hud.assist_alert_samples + [int]$hud.thermal_alert_samples + [int]$hud.cooling_alert_samples) -lt 1) { throw "FIELDMASTER_HUD_RUNTIME.json lacks hill-haul/thermal/cooling alert coverage." }

$trailer = Read-JsonRequired "NATIVE_TRAILER_RUNTIME.json"
Assert-ExactIdentity $trailer "NATIVE_TRAILER_RUNTIME.json"
if ([string]$trailer.schema -ne "gtt.native-trailer-runtime.v1" -or [string]$trailer.result -ne "PASS") {
    throw "NATIVE_TRAILER_RUNTIME.json is not PASS schema v1."
}
if ([int]$trailer.authored_active_samples -lt 2 -or [int]$trailer.native_tow_samples -lt 2) {
    throw "Authored trailer runtime lacks active Native tow coverage."
}
if ([int]$trailer.dual_contact_samples -lt 2 -or [int]$trailer.safe_hitch_samples -lt 2) {
    throw "Authored trailer runtime lacks dual-wheel contact / safe hitch coverage."
}
if ([string]$trailer.deterministic_loaded_tow -ne "PASS" -or [int]$trailer.safe_loaded_motion_samples -lt 8) {
    throw "Authored trailer runtime did not prove the deterministic loaded tow route."
}
if (-not [bool]$trailer.controlled_stop_proven -or [int]$trailer.trailer_scenario_diagnostic_failures -ne 0) {
    throw "Authored trailer runtime did not prove a clean controlled stop."
}
if ([int]$trailer.invalid_rig_observation_count -ne 0) {
    throw "Exact candidate observed an invalid authored trailer rig state."
}

$gate = Read-JsonRequired "DEMO_TECHNICAL_GATE.json"
Assert-ExactIdentity $gate "DEMO_TECHNICAL_GATE.json"
if ([int]$gate.schema -ne 17 -or [string]$gate.result -ne "PASS") { throw "DEMO_TECHNICAL_GATE.json must be PASS schema 17." }

$visual = Read-JsonRequired "DEMO_VISUAL_EVIDENCE.json"
Assert-ExactIdentity $visual "DEMO_VISUAL_EVIDENCE.json"
if ([string]$visual.result -ne "PASS") { throw "DEMO_VISUAL_EVIDENCE.json is not PASS." }

$summary = Read-JsonRequired "WIN64_ACCEPTANCE_SUMMARY.json"
Assert-ExactIdentity $summary "WIN64_ACCEPTANCE_SUMMARY.json"
if ([string]$summary.result -ne "PASS") { throw "WIN64_ACCEPTANCE_SUMMARY.json is not PASS." }
if ([string]$summary.configuration -ne $Configuration) { throw "WIN64_ACCEPTANCE_SUMMARY.json configuration does not match '$Configuration'." }
if ([string]$summary.native_authority_runtime -ne "PASS") { throw "WIN64_ACCEPTANCE_SUMMARY.json native authority runtime is not PASS." }
if ([string]$summary.fieldmaster_hill_haul_runtime -ne "PASS") { throw "WIN64_ACCEPTANCE_SUMMARY.json hill-haul runtime is not PASS." }
if ([string]$summary.fieldmaster_hud_runtime -ne "PASS") { throw "WIN64_ACCEPTANCE_SUMMARY.json Fieldmaster HUD runtime is not PASS." }
if ([int]$summary.fieldmaster_hud_visible_alert_samples -lt 1) { throw "WIN64_ACCEPTANCE_SUMMARY.json lacks Fieldmaster HUD visible-alert coverage." }
if ([string]$summary.human_visual_review -ne "REQUIRED") { throw "Human visual review boundary must remain REQUIRED." }
if ([bool]$summary.demo_release_authorized) { throw "Technical attestation must never authorize a Demo Release." }

$exeCandidates = @(Get-ChildItem -Path $PackageDirectory -Recurse -File -Filter "GTT.exe")
if ($exeCandidates.Count -ne 1) { throw "Expected exactly one packaged GTT.exe, found $($exeCandidates.Count)." }
$exe = $exeCandidates[0]

$shots = @(Get-ChildItem -Path (Join-Path $PackageDirectory "DemoVisualEvidence") -File -Filter "GTT_visual_*.png" | Sort-Object Name)
if ($shots.Count -ne 5) { throw "Expected exactly five rendered screenshots, found $($shots.Count)." }

$criticalNames = @(
    "WIN64_PREFLIGHT.json",
    "BUILD_ATTEMPT.json",
    "BUILD_INFO.json",
    "PACKAGE_VALIDATION.json",
    "AUTHORED_TRAILER_IMPORT.json",
    "RUNTIME_SMOKE.json",
    "DEMO_SCENARIO.json",
    "GAMEPLAY_SMOKE.json",
    "NATIVE_CHAOS_RUNTIME.json",
    "NATIVE_AUTHORITY_RUNTIME.json",
    "FIELDMASTER_HILL_HAUL_RUNTIME.json",
    "FIELDMASTER_HUD_RUNTIME.json",
    "NATIVE_DRIVETRAIN_SCENARIO.json",
    "NATIVE_TRAILER_RUNTIME.json",
    "FARM_CARGO_RUNTIME.json",
    "FARM_CARGO_RECOVERY_RUNTIME.json",
    "FARM_CARGO_BREAKDOWN_RUNTIME.json",
    "FARM_CARGO_DISPATCH_RUNTIME.json",
    "FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME.json",
    "FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json",
    "WORKSHOP_HOURS_RUNTIME.json",
    "WORKSHOP_QUEUE_RUNTIME.json",
    "WORKSHOP_CAPACITY_RUNTIME.json",
    "WORKSHOP_PRIORITY_PICKUP_RUNTIME.json",
    "DEMO_TECHNICAL_GATE.json",
    "VISUAL_RUNTIME_SMOKE.json",
    "DEMO_VISUAL_EVIDENCE.json",
    "WIN64_ACCEPTANCE_SUMMARY.json",
    "GTT_RUNTIME.log",
    "GTT_VISUAL_RUNTIME.log"
)

$criticalFiles = @()
foreach ($name in $criticalNames) {
    $path = Join-Path $PackageDirectory $name
    if (-not (Test-Path $path -PathType Leaf)) { throw "Critical candidate file missing: $name" }
    $criticalFiles += Get-Item $path
}
$criticalFiles += $exe
$criticalFiles += $shots

$hashed = foreach ($file in ($criticalFiles | Sort-Object FullName -Unique)) {
    $relative = [IO.Path]::GetRelativePath($PackageDirectory, $file.FullName).Replace('\','/')
    [ordered]@{
        path = $relative
        bytes = [int64]$file.Length
        sha256 = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
    }
}

$attestation = [ordered]@{
    schema = "gtt.win64-candidate-attestation.v1"
    result = "PASS"
    game = "Grand Theft Tractor"
    git_sha = $ExpectedGitSha
    version = $Version
    configuration = $Configuration
    platform = "Win64"
    engine = "Unreal Engine 5.8"
    native_chaos_tractor_movement = "PASS"
    native_chaos_movement_samples = [int]$chaos.movement_active_samples
    native_chaos_max_speed_kmh = [double]$chaos.max_speed_kmh
    native_chaos_drivetrain_suspension_wheels = "PASS"
    native_chaos_valid_wheels = [int]$chaos.max_valid_wheels
    native_chaos_suspension_samples = [int]$chaos.max_suspension_samples
    native_chaos_contact_samples = [int]$chaos.max_contacts
    native_drivetrain_scenario = "PASS"
    native_drivetrain_max_forward_gear = [int]$drivetrain.max_forward_gear_observed
    native_drivetrain_diagnostic_failures = [int]$drivetrain.diagnostic_failure_count
    native_authority_runtime = "PASS"
    native_authority_faults = 0
    authored_trailer_runtime = "PASS"
    authored_trailer_dual_contact_samples = [int]$trailer.dual_contact_samples
    authored_trailer_safe_hitch_samples = [int]$trailer.safe_hitch_samples
    authored_trailer_safe_loaded_motion_samples = [int]$trailer.safe_loaded_motion_samples
    authored_trailer_controlled_stop = [bool]$trailer.controlled_stop_proven
    authored_trailer_invalid_rig_observations = [int]$trailer.invalid_rig_observation_count
    fieldmaster_hill_haul_runtime = "PASS"
    hill_haul_loaded_samples = [int]$hillHaul.loaded_trailer_samples
    hill_haul_assist_samples = [int]$hillHaul.assist_samples
    hill_haul_thermal_samples = [int]$hillHaul.thermal_samples
    fieldmaster_hud_runtime = "PASS"
    fieldmaster_hud_telemetry_samples = [int]$hud.telemetry_samples
    fieldmaster_hud_visible_alert_samples = [int]$hud.visible_alert_samples
    fieldmaster_hud_assist_alert_samples = [int]$hud.assist_alert_samples
    fieldmaster_hud_thermal_alert_samples = [int]$hud.thermal_alert_samples
    fieldmaster_hud_cooling_alert_samples = [int]$hud.cooling_alert_samples
    technical_gate_schema = 17
    technical_gate = "PASS"
    rendered_visual_evidence = "PASS"
    rendered_screenshot_count = $shots.Count
    packaged_exe = [IO.Path]::GetRelativePath($PackageDirectory, $exe.FullName).Replace('\','/')
    packaged_exe_sha256 = (Get-FileHash -Algorithm SHA256 -Path $exe.FullName).Hash.ToLowerInvariant()
    evidence_hash_algorithm = "SHA256"
    evidence_file_count = @($hashed).Count
    evidence_files = @($hashed)
    human_visual_review = "REQUIRED"
    demo_release_authorized = $false
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
}

$attestationPath = Join-Path $PackageDirectory "WIN64_CANDIDATE_ATTESTATION.json"
$attestation | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $attestationPath

$finalManifestPath = Join-Path $PackageDirectory "FINAL_SHA256SUMS.txt"
$finalFiles = Get-ChildItem -Path $PackageDirectory -Recurse -File |
    Where-Object { $_.FullName -ne $finalManifestPath } |
    Sort-Object FullName
$manifestLines = foreach ($file in $finalFiles) {
    $relative = [IO.Path]::GetRelativePath($PackageDirectory, $file.FullName).Replace('\','/')
    $hash = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
    "$hash  $relative"
}
$manifestLines | Set-Content -Encoding ASCII $finalManifestPath

$zipPath = "$PackageDirectory.zip"
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
Compress-Archive -Path (Join-Path $PackageDirectory "*") -DestinationPath $zipPath -CompressionLevel Optimal
$zipHash = (Get-FileHash -Algorithm SHA256 -Path $zipPath).Hash.ToLowerInvariant()
Set-Content -Encoding ASCII -Path "$zipPath.sha256" -Value "$zipHash  $([IO.Path]::GetFileName($zipPath))"

$roundTrip = Get-Content -Raw $attestationPath | ConvertFrom-Json
if ($roundTrip.schema -ne "gtt.win64-candidate-attestation.v1" -or $roundTrip.result -ne "PASS" -or
    $roundTrip.git_sha -ne $ExpectedGitSha -or $roundTrip.version -ne $Version -or
    $roundTrip.configuration -ne $Configuration -or $roundTrip.native_chaos_tractor_movement -ne "PASS" -or
    [int]$roundTrip.native_chaos_movement_samples -lt 2 -or [double]$roundTrip.native_chaos_max_speed_kmh -lt 0.35 -or
    $roundTrip.native_chaos_drivetrain_suspension_wheels -ne "PASS" -or [int]$roundTrip.native_chaos_valid_wheels -lt 4 -or
    [int]$roundTrip.native_chaos_suspension_samples -lt 4 -or $roundTrip.native_drivetrain_scenario -ne "PASS" -or
    [int]$roundTrip.native_drivetrain_max_forward_gear -lt 2 -or [int]$roundTrip.native_drivetrain_diagnostic_failures -ne 0 -or
    $roundTrip.native_authority_runtime -ne "PASS" -or [int]$roundTrip.native_authority_faults -ne 0 -or
    $roundTrip.authored_trailer_runtime -ne "PASS" -or [int]$roundTrip.authored_trailer_dual_contact_samples -lt 2 -or
    [int]$roundTrip.authored_trailer_safe_loaded_motion_samples -lt 8 -or -not [bool]$roundTrip.authored_trailer_controlled_stop -or
    [int]$roundTrip.authored_trailer_invalid_rig_observations -ne 0 -or $roundTrip.fieldmaster_hill_haul_runtime -ne "PASS" -or
    [int]$roundTrip.hill_haul_loaded_samples -lt 2 -or [int]$roundTrip.hill_haul_assist_samples -lt 1 -or
    [int]$roundTrip.hill_haul_thermal_samples -lt 1 -or $roundTrip.fieldmaster_hud_runtime -ne "PASS" -or
    [int]$roundTrip.fieldmaster_hud_telemetry_samples -lt 2 -or [int]$roundTrip.fieldmaster_hud_visible_alert_samples -lt 1 -or
    $roundTrip.human_visual_review -ne "REQUIRED" -or [bool]$roundTrip.demo_release_authorized) {
    throw "WIN64_CANDIDATE_ATTESTATION.json failed round-trip identity/current-gate/boundary validation."
}

Write-Host "[GTT][ATTEST] PASS: exact candidate identity, current-target Native Chaos/trailer gates and final evidence hashes are sealed."
Write-Host "[GTT][ATTEST] Attestation: $attestationPath"
Write-Host "[GTT][ATTEST] Final manifest: $finalManifestPath"
