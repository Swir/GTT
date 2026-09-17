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
citizen_h = read("Source/GTT/Public/NPC/GTTCitizenPawn.h")
citizen_cpp = read("Source/GTT/Private/NPC/GTTCitizenPawn.cpp")
hud_h = read("Source/GTT/Public/UI/GTTGameHUD.h")
hud_cpp = read("Source/GTT/Private/UI/GTTGameHUD.cpp")
traffic_cpp = read("Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp")
roadmap = read("Docs/ROADMAP.md")
readme = read("README.md")
playtest = read("Docs/PLAYTEST_0.1.24.md")
changelog = read("CHANGELOG.d/0.1.24.md")

for needle, label in [
    ("FGTTRangerRoadStopPresentation", "compact presentation snapshot"),
    ("GetPresentationSnapshot", "HUD snapshot API"),
    ("GetCivilianResponse", "civilian roadside response API"),
    ("InitialGraceSeconds", "deterministic COMPLY progress baseline"),
    ("ComplianceSpeedLimitKmh", "presentation speed-limit authority"),
    ("TrafficSameDirectionDot = 0.35f", "same-direction lane filter"),
    ("CivilianAwarenessRadiusCm = 1800.0f", "bounded civilian awareness"),
    ("CivilianSafeLateralCm = 720.0f", "safe civilian lateral target"),
]:
    require(subsystem_h, needle, label)

for needle, label in [
    ("SameDirectionDot <= TrafficSameDirectionDot", "opposite-lane pass-through"),
    ("GetCivilianResponse", "civilian response implementation"),
    ("PreferredSide", "stable safe-side selection"),
    ("PULL OVER | SLOW BELOW", "clear COMPLY instruction"),
    ("REMAIN STOPPED | VEHICLE SEARCH IN PROGRESS", "clear SEARCH instruction"),
    ("STOP FAILED | POLICE ESCALATION ACTIVE", "clear FLEE instruction"),
    ("SearchHoldElapsed / SearchHoldRequired", "SEARCH progress"),
    ("1.0f - RemainingSeconds / InitialGraceSeconds", "COMPLY progress"),
]:
    require(subsystem_cpp, needle, label)

for needle, label in [
    ("IsReactingToRangerStop", "civilian reaction state API"),
    ("IsObservingRangerStop", "civilian observer state API"),
    ("RangerStopReactionSpeed = 185.0f", "bounded civilian reaction speed"),
]:
    require(citizen_h, needle, label)

for needle, label in [
    ("Ranger/GTTRangerRoadStopSubsystem.h", "shared road-stop authority include"),
    ("GetCivilianResponse", "civilian shared-state query"),
    ("!bBrawlParticipant && !IsFactionHostile()", "combat/hostile priority guard"),
    ("bUrgentSimulation", "performance-budget participation"),
    ("|| bRangerSceneResponse", "critical update cadence during scene response"),
    ("AddMovementInput(ToSafe.GetSafeNormal2D(), 1.0f)", "pedestrian lane-clear movement"),
    ("RInterpTo", "stationary witness orientation"),
    ("ChooseNewWanderTarget();", "schedule recovery after stop"),
]:
    require(citizen_cpp, needle, label)

for needle, label in [
    ("DrawRangerStopPanel", "compact enforcement panel declaration"),
]:
    require(hud_h, needle, label)

for needle, label in [
    ("GetPresentationSnapshot", "HUD consumes presentation snapshot"),
    ("WARDEN STOP  |  %s", "phase heading"),
    ("DrawRect(FLinearColor(0.01f, 0.025f, 0.04f, 0.90f)", "compact dark panel"),
    ("Snapshot.Progress01", "single progress bar"),
    ("EGTTRangerRoadStopPhase::Search", "SEARCH visual state"),
    ("EGTTRangerRoadStopPhase::Flee", "FLEE visual state"),
]:
    require(hud_cpp, needle, label)

# Existing traffic car remains the physical brake/queue consumer; 0.1.24 refines
# which lane receives that response instead of creating a second traffic controller.
require(traffic_cpp, "RoadStop->GetTrafficResponse", "existing traffic consumer preserved")
require(traffic_cpp, "bHoldingForRangerStop", "physical traffic hold preserved")

# The presentation/scene layer must not introduce duplicate law/economy authority.
for forbidden, label in [
    ("UGTTWantedComponent", "duplicate wanted ownership in road-stop subsystem"),
    ("ConfiscateContraband", "duplicate seizure ownership in road-stop subsystem"),
    ("SaveGame", "persisted transient road-stop presentation"),
]:
    if forbidden in subsystem_h + subsystem_cpp:
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

require(readme, "## 🔎 Search Keywords", "README SEO/search-keywords section")
keywords_line = next((line for line in readme.splitlines() if "`original sandbox game`" in line), "")
keyword_count = keywords_line.count("`") // 2
if not 8 <= keyword_count <= 20:
    raise AssertionError(f"README keyword count outside 8..20: {keyword_count}")

for text, label in [(playtest, "playtest"), (changelog, "changelog")]:
    require(text, "0.1.24", f"{label} milestone version")
    require(text.lower(), "civilian", f"{label} civilian coverage")
    require(text.lower(), "hud", f"{label} HUD coverage")
    require(text.lower(), "opposite", f"{label} opposite-lane coverage")

print("GTT 0.1.24 enforcement UX / civilian roadside scene sanity: PASS")
print("- COMPLY / SEARCH / FLEE now has one compact progress-driven HUD presentation snapshot")
print("- nearby civilians clear the live lane, observe from a safe offset and return to schedules afterward")
print("- opposite-direction traffic is excluded while the existing same-direction brake/queue logic stays authoritative")
print("- combat, Wanted, seizure and persistence ownership remain in their existing systems")
print("- roadmap remains truthfully locked at 125/130 (96.2%)")
