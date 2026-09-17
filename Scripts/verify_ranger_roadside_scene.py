from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


subsystem_h = read("Source/GTT/Public/Ranger/GTTRangerRoadStopSubsystem.h")
subsystem_cpp = read("Source/GTT/Private/Ranger/GTTRangerRoadStopSubsystem.cpp")
ranger_h = read("Source/GTT/Public/Ranger/GTTRangerAIController.h")
ranger_cpp = read("Source/GTT/Private/Ranger/GTTRangerAIController.cpp")
director_h = read("Source/GTT/Public/Ranger/GTTRangerDirector.h")
director_cpp = read("Source/GTT/Private/Ranger/GTTRangerDirector.cpp")
patrol_h = read("Source/GTT/Public/Ranger/GTTRangerPatrolVehicle.h")
patrol_cpp = read("Source/GTT/Private/Ranger/GTTRangerPatrolVehicle.cpp")
traffic_cpp = read("Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp")
hud_cpp = read("Source/GTT/Private/UI/GTTGameHUD.cpp")
roadmap = read("Docs/ROADMAP.md")
readme = read("README.md")
playtest = read("Docs/PLAYTEST_0.1.25.md")
changelog = read("CHANGELOG.d/0.1.25.md")
workflow = read(".github/workflows/ranger-roadside-scene-sanity.yml")

# One fixed incident frame owns lane control, the actual shoulder destination and
# patrol parking. These values are intentionally bounded and source-verifiable.
for needle, label in [
    ("PullOverAheadCm = 700.0f", "bounded forward pull-over target"),
    ("PullOverLateralCm = 420.0f", "bounded shoulder offset"),
    ("PullOverAcceptanceRadiusCm = 275.0f", "bounded pull-over acceptance zone"),
    ("PatrolVehicleRearOffsetCm = 520.0f", "patrol parking distance"),
    ("GetPullOverTargetLocation", "pull-over target API"),
    ("GetPullOverDistanceCm", "pull-over distance API"),
    ("IsTargetInPullOverZone", "spatial compliance API"),
    ("GetPatrolVehicleTransform", "shared patrol transform API"),
    ("bInPullOverZone", "presentation spatial-compliance state"),
    ("PullOverDistanceMeters", "presentation marker distance"),
]:
    require(subsystem_h, needle, label)

for needle, label in [
    ("StopLocation += RoadForward * PullOverAheadCm", "frozen lane anchor ahead of the stop order"),
    ("PullOverTargetLocation = StopLocation + RoadRight * ShoulderSide * PullOverLateralCm", "physical shoulder target"),
    ("FVector::Dist2D(Target->GetActorLocation(), PullOverTargetLocation)", "real spatial target distance"),
    ("PatrolLocation = PullOverTargetLocation - RoadForward * PatrolVehicleRearOffsetCm", "patrol unit behind player stop"),
    ("PULL OVER | MOVE TO SHOULDER MARKER", "distance-aware COMPLY HUD instruction"),
    ("PULL OVER | SLOW BELOW", "existing speed-compliance instruction preserved"),
    ("REMAIN STOPPED | VEHICLE SEARCH IN PROGRESS", "SEARCH instruction preserved"),
    ("STOP FAILED | POLICE ESCALATION ACTIVE", "FLEE instruction preserved"),
    ("SameDirectionDot <= TrafficSameDirectionDot", "same-direction traffic filter preserved"),
]:
    require(subsystem_cpp, needle, label)

# Critical regression guard: the road frame is captured when the stop begins and
# must not be refreshed every UpdateStop tick, otherwise the marker chases the car.
update_body = subsystem_cpp.split("void UGTTRangerRoadStopSubsystem::UpdateStop", 1)[1].split(
    "void UGTTRangerRoadStopSubsystem::MarkFlee", 1
)[0]
if "RefreshRoadFrame(Target)" in update_body:
    raise AssertionError("UpdateStop must not move the fixed roadside frame with the target")

# Compliance now requires both the existing speed requirement and the real
# shoulder zone; legacy vehicle-family coverage and old escalation contract stay.
for needle, label in [
    ("RoadStopShoulderOffset = 190.0f", "existing ranger shoulder staging contract"),
    ("RoadStopSearchRadius = 275.0f", "existing search/pull-over tolerance"),
    ("RoadStopComplianceSpeedKmh = 2.5f", "existing compliance speed"),
    ("RoadStopFleeSpeedKmh = 8.0f", "existing flee speed"),
]:
    require(ranger_h, needle, label)

for needle, label in [
    ("RoadStopSubsystem->IsTargetInPullOverZone(Target, RoadStopSearchRadius)", "spatial compliance query"),
    ("RoadStopSubsystem->GetPullOverDistanceCm(Target)", "late-compliance distance feedback"),
    ("const bool bSearching = bCompliantSpeed && bInsideSearchRadius", "dual speed/spatial search gate"),
    ("RoadStopComplianceSpeedKmh, bInsideSearchRadius", "shared presentation receives spatial state"),
    ("COMPLY NOW - move into the shoulder marker", "late physical pull-over reminder"),
    ("Wanted->AddHeat(RoadStopEvasionWantedHeat)", "existing Wanted escalation retained"),
    ("ConfiscateContraband", "existing authoritative seizure retained"),
    ("TryRangerCitation(Target)", "existing citation authority retained"),
]:
    require(ranger_cpp, needle, label)

# The patrol unit is presentation/support, not a second garage/persistence vehicle.
for needle, label in [
    ("class GTT_API AGTTRangerPatrolVehicle : public AActor", "non-drivable patrol support actor"),
    ("IsRoadsideDeployed", "patrol deployment state"),
    ("BeaconLeft", "left warning beacon"),
    ("BeaconRight", "right warning beacon"),
]:
    require(patrol_h, needle, label)

for needle, label in [
    ("/Engine/BasicShapes/Cube.Cube", "engine-owned original code-built body"),
    ("/Engine/BasicShapes/Cylinder.Cylinder", "engine-owned code-built wheels"),
    ("/Engine/BasicShapes/Sphere.Sphere", "engine-owned code-built beacons"),
    ("GetPatrolVehicleTransform", "patrol consumes shared incident transform"),
    ("SetActorHiddenInGame(true)", "patrol hidden outside incident"),
    ("SetActorEnableCollision(false)", "patrol non-blocking outside incident"),
    ("SetActorLocationAndRotation", "stable roadside deployment"),
    ("RangerPatrolVehicleLabel", "original warden label"),
]:
    require(patrol_cpp, needle, label)

for forbidden, label in [
    ("GTTVehicleBase", "player-drivable vehicle inheritance"),
    ("SaveGame", "persistent transient patrol state"),
    ("UGTTWantedComponent", "duplicate wanted authority"),
    ("ConfiscateContraband", "duplicate seizure authority"),
]:
    if forbidden in patrol_h + patrol_cpp:
        raise AssertionError(f"unexpected {label} in patrol scene actor: {forbidden}")

for needle, label in [
    ("class AGTTRangerPatrolVehicle", "director patrol forward declaration"),
    ("PatrolVehicleClass", "director configurable patrol class"),
    ("PatrolVehicle", "single director-owned patrol instance"),
]:
    require(director_h, needle, label)

for needle, label in [
    ("PatrolVehicleClass = AGTTRangerPatrolVehicle::StaticClass()", "default original patrol actor"),
    ("SpawnActor<AGTTRangerPatrolVehicle>", "single patrol support spawn"),
    ("SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn", "deterministic hidden support spawn"),
]:
    require(director_cpp, needle, label)

# Existing consumers remain authoritative; 0.1.25 extends their common scene
# instead of replacing traffic or HUD systems with parallel implementations.
require(traffic_cpp, "RoadStop->GetTrafficResponse", "existing physical traffic controller")
require(hud_cpp, "GetPresentationSnapshot", "existing compact road-stop HUD consumer")
require(hud_cpp, "Snapshot.Progress01", "single compact progress bar preserved")

# No source-only milestone may fake the remaining runtime/hardware acceptance.
require(roadmap, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "SWIR roadmap style lock")
require(roadmap, "📊 Overall progress", "roadmap dashboard heading")
checked = len(re.findall(r"^\s*- \[x\]", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \]", roadmap, flags=re.MULTILINE))
if (checked, open_items, checked + open_items) != (125, 5, 130):
    raise AssertionError(
        f"roadmap truth changed unexpectedly: checked={checked}, open={open_items}, total={checked + open_items}"
    )
require(roadmap, "███████████████████░ 96.2%", "roadmap progress bar")

# README is intentionally not rewritten by this gameplay milestone; preserve the
# currently enforced family marker/search section and truthful no-demo statement.
require(readme, "<!-- SWIR-README-STANDARD:v1 -->", "current SWIR README contract")
require(readme, "## 🔎 Search Keywords", "README discoverability section")
require(readme, "No public demo release is available yet", "truthful demo status")
keyword_section = readme.split("## 🔎 Search Keywords", 1)[1]
keywords = re.findall(r"`([^`]+)`", keyword_section.split("##", 1)[0])
if not 8 <= len(keywords) <= 20:
    raise AssertionError(f"README keyword count must remain 8-20, found {len(keywords)}")
if any("gta" in keyword.lower() for keyword in keywords):
    raise AssertionError("README search keywords must not use GTA branding for SEO")

for text, label in [(playtest, "playtest"), (changelog, "changelog")]:
    require(text, "0.1.25", f"{label} milestone version")
    require(text.lower(), "shoulder", f"{label} shoulder pull-over coverage")
    require(text.lower(), "patrol", f"{label} patrol scene coverage")
    require(text.lower(), "win64", f"{label} truthful Win64 status")

require(workflow, "python Scripts/verify_ranger_enforcement_ux.py", "0.1.24 regression chain")
require(workflow, "python Scripts/verify_ranger_roadside_scene.py", "0.1.25 milestone verifier")

print("GTT 0.1.25 physical shoulder pull-over / warden patrol scene sanity: PASS")
print("- compliance requires a stable physical shoulder target plus the existing speed/hold requirements")
print("- ranger, traffic, civilians and the patrol support unit share one frozen roadside incident frame")
print("- the original code-built patrol actor is hidden/non-colliding outside the scene and owns no save/Wanted/economy state")
print("- 0.1.24 enforcement UX, same-direction traffic control and compact HUD remain authoritative")
print("- roadmap remains truthfully locked at 125/130 (96.2%); no Win64/demo claim is inferred")