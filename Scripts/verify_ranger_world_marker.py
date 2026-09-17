from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


subsystem_h = read("Source/GTT/Public/Ranger/GTTRangerRoadStopSubsystem.h")
director_h = read("Source/GTT/Public/Ranger/GTTRangerDirector.h")
director_cpp = read("Source/GTT/Private/Ranger/GTTRangerDirector.cpp")
marker_h = read("Source/GTT/Public/Ranger/GTTRangerPullOverMarker.h")
marker_cpp = read("Source/GTT/Private/Ranger/GTTRangerPullOverMarker.cpp")
patrol_h = read("Source/GTT/Public/Ranger/GTTRangerPatrolVehicle.h")
patrol_cpp = read("Source/GTT/Private/Ranger/GTTRangerPatrolVehicle.cpp")
roadmap = read("Docs/ROADMAP.md")
playtest = read("Docs/PLAYTEST_0.1.26.md")
changelog = read("CHANGELOG.d/0.1.26.md")
workflow = read(".github/workflows/ranger-world-marker-sanity.yml")

# Marker consumes the exact fixed pull-over target and road heading. It must not
# invent a second compliance position or keep presentation alive during FLEE.
for needle, label in [
    ("GetPullOverMarkerTransform", "shared marker transform API"),
    ("if (!HasTrafficControl())", "COMPLY/SEARCH-only marker lifetime"),
    ("FTransform(RoadForward.Rotation(), PullOverTargetLocation, FVector::OneVector)", "authoritative fixed target transform"),
]:
    require(subsystem_h, needle, label)

for needle, label in [
    ("class GTT_API AGTTRangerPullOverMarker : public AActor", "presentation-only marker actor"),
    ("IsMarkerDeployed", "marker deployment state"),
    ("GroundDisc", "ground target geometry"),
    ("DirectionChevron", "direction cue"),
    ("MarkerLabel", "world label"),
    ("MarkerLight", "night-readable marker light"),
]:
    require(marker_h, needle, label)

for needle, label in [
    ("/Engine/BasicShapes/Cylinder.Cylinder", "engine primitive ground disc"),
    ("/Engine/BasicShapes/Cone.Cone", "engine primitive chevron"),
    ("RangerPullOverMarkerLabel", "original pull-over label"),
    ("GetPullOverMarkerTransform", "marker follows shared authority"),
    ("LineTraceSingleByObjectType", "grounding trace"),
    ("ECC_WorldStatic", "dynamic vehicles excluded from grounding"),
    ("Hit.ImpactPoint.Z + 4.0f", "surface offset avoiding z-fighting"),
    ("RoadStop->GetPhase() == EGTTRangerRoadStopPhase::Search", "phase-aware declutter"),
    ("MarkerLabel->SetVisibility(!bSearchPhase", "SEARCH floating-label declutter"),
    ("DirectionChevron->SetVisibility(!bSearchPhase", "SEARCH chevron declutter"),
    ("SetActorEnableCollision(false)", "non-blocking presentation"),
]:
    require(marker_cpp, needle, label)

for forbidden, label in [
    ("UGTTWantedComponent", "duplicate Wanted authority"),
    ("ConfiscateContraband", "duplicate economy authority"),
    ("SaveGame", "persistent transient marker state"),
    ("TryRangerCitation", "duplicate citation authority"),
]:
    if forbidden in marker_h + marker_cpp:
        raise AssertionError(f"unexpected {label} in marker actor: {forbidden}")

# Director owns one reusable marker just as it owns one patrol scene actor.
for needle, label in [
    ("class AGTTRangerPullOverMarker", "marker forward declaration"),
    ("PullOverMarkerClass", "configurable marker class"),
    ("PullOverMarker", "single director-owned marker"),
    ("IsPullOverMarkerAvailable", "marker availability diagnostic"),
]:
    require(director_h, needle, label)
for needle, label in [
    ("PullOverMarkerClass = AGTTRangerPullOverMarker::StaticClass()", "default marker actor"),
    ("SpawnActor<AGTTRangerPullOverMarker>", "single marker spawn"),
    ("SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn", "deterministic hidden presentation spawn"),
]:
    require(director_cpp, needle, label)

# Patrol scene has real light components, not only toggled geometry. SEARCH is
# intentionally distinct from the alternating roadside warning beacons.
for needle, label in [
    ("BeaconLightLeft", "left beacon point light"),
    ("BeaconLightRight", "right beacon point light"),
    ("SearchLamp", "search scene lamp"),
    ("UpdateBeacons(float DeltaSeconds, bool bSearchPhase)", "phase-aware patrol presentation"),
]:
    require(patrol_h, needle, label)
for needle, label in [
    ("CreateDefaultSubobject<UPointLightComponent>(TEXT(\"BeaconLightLeft\"))", "left real light creation"),
    ("CreateDefaultSubobject<UPointLightComponent>(TEXT(\"BeaconLightRight\"))", "right real light creation"),
    ("CreateDefaultSubobject<USpotLightComponent>(TEXT(\"SearchLamp\"))", "search lamp creation"),
    ("RoadStop->GetPhase() == EGTTRangerRoadStopPhase::Search", "search phase integration"),
    ("BeaconLightLeft->SetVisibility(bLeft, true)", "left light synchronization"),
    ("BeaconLightRight->SetVisibility(!bLeft, true)", "right light synchronization"),
    ("SearchLamp->SetVisibility(bSearchPhase, true)", "search-only scene lighting"),
]:
    require(patrol_cpp, needle, label)

# Critical transient-state guard: both presentation actors must explicitly hide
# themselves at BeginPlay and clear visible lights when not deployed.
require(marker_cpp, "SetActorHiddenInGame(true)", "marker hidden at rest")
require(patrol_cpp, "SetActorHiddenInGame(true)", "patrol hidden at rest")
for light in ["BeaconLightLeft", "BeaconLightRight", "SearchLamp"]:
    require(patrol_cpp, f"{light}->SetVisibility(false, true)", f"{light} cleared outside incident")

# Source work must preserve the locked Roadmap truth until real Win64/runtime
# acceptance closes one of the remaining hardware-dependent items.
require(roadmap, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "SWIR roadmap style lock")
require(roadmap, "📊 Overall progress", "roadmap dashboard heading")
checked = len(re.findall(r"^\s*- \[x\]", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \]", roadmap, flags=re.MULTILINE))
if (checked, open_items, checked + open_items) != (125, 5, 130):
    raise AssertionError(
        f"roadmap truth changed unexpectedly: checked={checked}, open={open_items}, total={checked + open_items}"
    )
require(roadmap, "███████████████████░ 96.2%", "roadmap progress bar")

for text, label in [(playtest, "playtest"), (changelog, "changelog")]:
    require(text, "0.1.26", f"{label} milestone version")
    require(text.lower(), "world-space", f"{label} world marker coverage")
    require(text.lower(), "search", f"{label} search presentation coverage")
    require(text.lower(), "win64", f"{label} truthful Win64 status")

require(workflow, "python Scripts/verify_ranger_roadside_scene.py", "0.1.25 regression chain")
require(workflow, "python Scripts/verify_ranger_world_marker.py", "0.1.26 milestone verifier")

print("GTT 0.1.26 world-space pull-over marker / patrol lighting sanity: PASS")
print("- COMPLY/SEARCH presentation consumes the fixed road-stop target and cannot create compliance independently")
print("- marker grounds against WorldStatic, stays non-colliding, declutters during SEARCH and disappears outside traffic control")
print("- patrol support now synchronizes real beacon lights and enables a dedicated SEARCH scene lamp")
print("- 0.1.25 physical roadside behavior remains the regression baseline")
print("- roadmap remains truthfully locked at 125/130 (96.2%); no Win64/demo claim is inferred")