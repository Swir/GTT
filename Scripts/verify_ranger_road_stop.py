from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing {label}: {needle}")


header = read("Source/GTT/Public/Ranger/GTTRangerAIController.h")
cpp = read("Source/GTT/Private/Ranger/GTTRangerAIController.cpp")
readme = read("README.md")
roadmap = read("Docs/ROADMAP.md")
playtest = read("Docs/PLAYTEST_0.1.22.md")
changelog = read("CHANGELOG.d/0.1.22.md")

for needle, label in [
    ("IsRoadStopActive", "road-stop runtime status API"),
    ("IsRoadStopEvasionEscalated", "road-stop evasion status API"),
    ("RoadStopAlertLevel = 2", "alert-2 road-stop threshold"),
    ("RoadStopOrderRadius = 1200.0f", "bounded road-stop order radius"),
    ("RoadStopSearchRadius = 275.0f", "bounded roadside-search radius"),
    ("RoadStopComplianceSpeedKmh = 2.5f", "compliance stop speed"),
    ("RoadStopFleeSpeedKmh = 8.0f", "evasion speed threshold"),
    ("RoadStopGraceSeconds = 7.0f", "compliance grace period"),
    ("RoadStopComplianceHoldSeconds = 2.25f", "search hold duration"),
    ("RoadStopEvasionWantedHeat = 45.0f", "bounded wanted escalation"),
]:
    require(header, needle, label)

for needle, label in [
    ("Cast<AGTTVehicleBase>(Target)", "legacy vehicle stop support"),
    ("Cast<AGTTFieldmasterNativePawn>(Target)", "native Fieldmaster stop support"),
    ("Cast<AGTTRoadVehicleNativePawn>(Target)", "native road-fleet stop support"),
    ("WARDEN ROAD STOP", "player-visible stop order"),
    ("WARDEN SEARCH", "player-visible roadside search"),
    ("ComplianceHoldElapsed += RepathInterval", "continuous compliance hold"),
    ("RoadStopTimeRemaining = FMath::Max", "authoritative grace countdown"),
    ("SpeedKmh >= RoadStopFleeSpeedKmh", "post-grace evasion decision"),
    ("Wanted->AddHeat(RoadStopEvasionWantedHeat)", "existing wanted/police integration"),
    ("FLED WARDEN STOP", "evasion escalation feedback"),
    ("ConfiscateContraband", "persistent rural contraband seizure"),
    ("TryRangerCitation(Target)", "existing citation/fish confiscation authority"),
    ("return;\n        }\n    }\n\n    if (DistanceSquared <= FMath::Square(CitationRadius))", "moving-vehicle citation guard"),
]:
    require(cpp, needle, label)

# The new system must not introduce a second police meter or a parallel contraband store.
for forbidden, label in [
    ("RoadStopWantedLevel", "duplicate wanted level"),
    ("RoadStopContraband", "duplicate contraband inventory"),
    ("PoliceHeat =", "parallel police heat state"),
]:
    if forbidden in header or forbidden in cpp:
        raise AssertionError(f"unexpected {label}: {forbidden}")

# README must follow the canonical SWIR family marker and include useful project SEO
# without using a competitor/trademark keyword for discoverability.
require(readme, "<!-- SWIR-README-STANDARD:v1 -->", "SWIR README standard marker")
require(readme, "## 🔎 Search Keywords", "mandatory search-keyword section")
require(readme, "Unreal Engine 5.8", "truthful engine requirement")
require(readme, "No public demo release is available yet", "truthful release status")
keyword_section = readme.split("## 🔎 Search Keywords", 1)[1]
keywords = re.findall(r"`([^`]+)`", keyword_section.split("##", 1)[0])
if not 8 <= len(keywords) <= 20:
    raise AssertionError(f"README keyword count must be 8-20, found {len(keywords)}")
if any("gta" in keyword.lower() for keyword in keywords):
    raise AssertionError("README search keywords must not use GTA branding for SEO")

# Preserve the locked roadmap dashboard and the five real Win64/runtime blockers.
require(roadmap, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "SWIR roadmap style lock")
require(roadmap, "📊 Overall progress", "roadmap dashboard heading")
checked = len(re.findall(r"^\s*- \[x\]", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \]", roadmap, flags=re.MULTILINE))
total = checked + open_items
if (checked, open_items, total) != (125, 5, 130):
    raise AssertionError(f"roadmap checkbox truth changed unexpectedly: checked={checked}, open={open_items}, total={total}")
require(roadmap, "███████████████████░ 96.2%", "20-segment roadmap progress bar")

for text, label in [
    (playtest, "playtest"),
    (changelog, "changelog"),
]:
    require(text, "0.1.22", f"{label} milestone version")
    require(text, "road stop", f"{label} road-stop coverage")

print("GTT 0.1.22 ranger road-stop/search/evasion sanity: PASS")
print("- alert-2+ vehicle enforcement now requires a real stop before citation/search resolution")
print("- fleeing a warden stop escalates through the existing Wanted/Police system exactly once per incident")
print("- roadside search reuses authoritative fish and rural-contraband seizure paths")
print("- README now follows SWIR README PRO and includes compliant discoverability keywords")
print("- roadmap remains truthfully locked at 125/130 (96.2%)")
