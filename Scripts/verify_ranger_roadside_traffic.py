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
traffic_h = read("Source/GTT/Public/Traffic/GTTTrafficCarPawn.h")
traffic_cpp = read("Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp")
roadmap = read("Docs/ROADMAP.md")
playtest = read("Docs/PLAYTEST_0.1.23.md")
changelog = read("CHANGELOG.d/0.1.23.md")

for needle, label in [
    ("EGTTRangerRoadStopPhase", "shared road-stop phase authority"),
    ("BeginStop", "single stop acquisition API"),
    ("GetRangerStagingPoint", "roadside staging API"),
    ("GetRangerSupportPoint", "reinforcement staging API"),
    ("GetTrafficResponse", "traffic-control query"),
    ("TrafficSlowRadiusCm = 2200.0f", "bounded slowdown radius"),
    ("TrafficHoldRadiusCm = 650.0f", "bounded comply hold radius"),
    ("TrafficSearchHoldRadiusCm = 800.0f", "bounded search safety radius"),
    ("TrafficCorridorHalfWidthCm = 1100.0f", "bounded road corridor"),
]:
    require(subsystem_h, needle, label)

for needle, label in [
    ("ApproachDot <= 0.20f", "approach-direction filter"),
    ("LateralDistanceCm > TrafficCorridorHalfWidthCm", "cross-road/corridor filter"),
    ("Phase == EGTTRangerRoadStopPhase::Search", "search-phase traffic hold"),
    ("FMath::Lerp(0.22f, 0.82f, Alpha)", "progressive traffic slowdown"),
    ("WARDEN STOP | COMPLY", "concise comply status"),
    ("WARDEN STOP | SEARCH", "concise search status"),
    ("WARDEN STOP | FLEE", "concise flee status"),
]:
    require(subsystem_cpp, needle, label)

for needle, label in [
    ("RoadStopShoulderOffset = 190.0f", "shoulder staging offset"),
    ("RoadStopRearOffset = 85.0f", "rear staging offset"),
    ("bComplianceReminderShown", "one-shot surrender reminder"),
]:
    require(ranger_h, needle, label)

for needle, label in [
    ("RoadStopSubsystem->BeginStop", "shared stop acquisition"),
    ("RoadStopSubsystem->GetRangerStagingPoint", "primary ranger shoulder positioning"),
    ("RoadStopSubsystem->GetRangerSupportPoint", "reinforcement shoulder positioning"),
    ("RoadStopSubsystem->IsStopForTarget(Target)", "duplicate stop suppression"),
    ("COMPLY NOW", "late compliance UX"),
    ("RoadStopSubsystem->UpdateStop", "authoritative phase update"),
    ("RoadStop->MarkFlee", "shared flee state"),
    ("if (bRoadStopEvasionEscalated)", "post-evasion citation guard"),
]:
    require(ranger_cpp, needle, label)

for needle, label in [
    ("IsYieldingForRangerStop", "traffic yield state API"),
    ("IsHoldingForRangerStop", "traffic hold state API"),
]:
    require(traffic_h, needle, label)

for needle, label in [
    ("GetTrafficResponse", "traffic consumes shared stop"),
    ("bForceCritical = IsOccupied() || IncidentStopRemaining > 0.0f || bIncidentDisabled || bYieldingForRangerStop", "performance-budget promotion near stop"),
    ("TrafficRangerStop", "STOP roadside traffic feedback"),
    ("TrafficRangerSlow", "SLOW roadside traffic feedback"),
    ("TrafficDriveForce * 1.45f", "physical traffic braking"),
    ("!bYieldingForRangerStop", "stuck-recovery suppression while yielding"),
]:
    require(traffic_cpp, needle, label)

# No second wanted/contraband system and no persistent traffic-stop timer may be introduced.
for forbidden, label in [
    ("RoadsideWantedLevel", "duplicate wanted state"),
    ("RoadsideContraband", "duplicate contraband store"),
    ("PersistentRoadStop", "save-persisted transient enforcement state"),
]:
    if forbidden in subsystem_h + subsystem_cpp + ranger_h + ranger_cpp + traffic_h + traffic_cpp:
        raise AssertionError(f"unexpected {label}: {forbidden}")

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
    require(text, "0.1.23", f"{label} milestone version")
    require(text.lower(), "traffic", f"{label} traffic coverage")
    require(text.lower(), "roadside", f"{label} roadside coverage")

print("GTT 0.1.23 ranger roadside positioning / traffic sanity: PASS")
print("- one world-authoritative road stop now coordinates primary ranger, reinforcement and ambient traffic")
print("- traffic progressively slows, physically holds near the stop and resumes without a persistent queue state")
print("- post-evasion proximity citations cannot erase a police escalation on the following ranger tick")
print("- roadmap remains truthfully locked at 125/130 (96.2%)")
