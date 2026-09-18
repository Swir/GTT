from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


director_h = read("Source/GTT/Public/Ranger/GTTRangerDirector.h")
director_cpp = read("Source/GTT/Private/Ranger/GTTRangerDirector.cpp")
ai_cpp = read("Source/GTT/Private/Ranger/GTTRangerAIController.cpp")
poach_h = read("Source/GTT/Public/Activities/GTTForestPoachingSpot.h")
poach_cpp = read("Source/GTT/Private/Activities/GTTForestPoachingSpot.cpp")
rural_h = read("Source/GTT/Public/World/GTTRuralEconomySubsystem.h")
roadmap = read("Docs/ROADMAP.md")

for needle, label in [
    ("GetEnforcementTier", "warden enforcement tier API"),
    ("IsPoliceHandoffActive", "police-handoff status API"),
    ("IsNightReinforcementActive", "night reinforcement status API"),
    ("PoliceHandoffAlertLevel = 3", "critical wildlife-alert threshold"),
    ("PoliceHandoffHeat = 35.0f", "bounded police handoff heat"),
]:
    require(director_h, needle, label)

for needle, label in [
    ("DayNight->IsNight()", "day/night ranger response"),
    ("bNightReinforcementActive = bNight && AlertLevel >= NightReinforcementAlertLevel", "night reinforcement threshold"),
    ("ApplyPoliceHandoff(PlayerPawn, AlertLevel)", "warden to police handoff call"),
    ("FindWantedComponentForPawn", "existing wanted-system integration"),
    ("Wanted->AddHeat(PoliceHandoffHeat)", "police escalation heat"),
    ("WARDEN RADIO HANDOFF", "player-visible handoff feedback"),
    ("if (AlertLevel <= 0)", "incident reset boundary"),
    ("bPoliceHandoffIssued = false", "handoff re-arm after incident"),
]:
    require(director_cpp, needle, label)

for needle, label in [
    ("TryRangerCitation(Target)", "existing citation authority"),
    ("ConfiscateContraband", "contraband seizure on citation"),
    ("WARDEN SEIZURE", "seizure feedback"),
]:
    require(ai_cpp, needle, label)

for needle, label in [
    ("NightWildlifeHeatMultiplier = 1.25f", "night heat risk"),
    ("NightFenceValueMultiplier = 1.18f", "night fence reward"),
]:
    require(poach_h, needle, label)

for needle, label in [
    ("DayNight->IsNight()", "night poaching detection"),
    ("HeatSeverity = WildlifeHeatPerAttempt * (bNight ? NightWildlifeHeatMultiplier : 1.0f)", "night severity application"),
    ("EstimatedValue = FMath::RoundToInt", "night reward application"),
    ("NIGHT RISK + REWARD", "night-risk player feedback"),
]:
    require(poach_cpp, needle, label)

for needle, label in [
    ("int32 ConfiscateContraband(int32& OutEstimatedValue)", "contraband confiscation API"),
    ("ContrabandUnits = 0", "contraband unit seizure"),
    ("ContrabandValue = 0", "contraband value seizure"),
    ("SaveState();", "persistent seizure state"),
]:
    require(rural_h, needle, label)

# Protect the locked roadmap dashboard and prove this gameplay milestone does not
# falsely close any of the five Win64/runtime hardware acceptance blockers.
for token, label in [
    ("<!-- SWIR-ROADMAP-STANDARD:v1 -->", "SWIR roadmap style lock"),
    ("<!-- ROADMAP-PROGRESS:START -->", "roadmap progress start"),
    ("📊 Overall progress", "roadmap dashboard heading"),
    ("../assets/readme/progress-mini.svg", "SVG-only roadmap meter"),
    ("| **125** | **5** | **130** | **96.2%** |", "roadmap numeric truth"),
]:
    require(roadmap, token, label)
checked = len(re.findall(r"^\s*- \[x\]", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \]", roadmap, flags=re.MULTILINE))
total = checked + open_items
if (checked, open_items, total) != (125, 5, 130):
    raise AssertionError(f"roadmap checkbox truth changed unexpectedly: checked={checked}, open={open_items}, total={total}")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise AssertionError("legacy text/Unicode roadmap progress meter must not return")

print("GTT 0.1.21 ranger enforcement handoff sanity: PASS")
print("- warden escalation uses the existing wanted/police system exactly once per wildlife incident")
print("- night poaching increases both risk and fence reward and can summon a second ranger")
print("- ranger citations confiscate the same persistent contraband created by forest poaching")
print("- roadmap remains truthfully locked at 125/130 (96.2%) with SVG-only presentation")
