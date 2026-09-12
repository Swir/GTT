from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")


door_h = read("Source/GTT/Public/World/GTTSocialVenueDoor.h")
door_cpp = read("Source/GTT/Private/World/GTTSocialVenueDoor.cpp")
npc_h = read("Source/GTT/Public/NPC/GTTSocialNPC.h")
npc_cpp = read("Source/GTT/Private/NPC/GTTSocialNPC.cpp")
world_h = read("Source/GTT/Public/World/GTTSocialWorldSubsystem.h")
world_cpp = read("Source/GTT/Private/World/GTTSocialWorldSubsystem.cpp")
roadmap = read("Docs/ROADMAP.md")
changelog = read("CHANGELOG.md")
playtest = read("Docs/PLAYTEST_0.0.28.md")
workflow = read(".github/workflows/project-sanity.yml")

assert "IGTTInteractable" in door_h and "SetActorLocation(Destination" in door_cpp
assert "bInteriorExit" in door_h and "Leave %s" in door_cpp
assert "EGTTSocialRole" in npc_h
for role in ["Bartender", "HallOrganizer", "MechanicLocal", "FarmerLocal"]:
    assert role in npc_h or role in npc_cpp, f"missing social role {role}"
assert "GetTimeOfDayHours" in npc_cpp, "dialogue must consume real day/night state"
assert "GetConditionPercent" in npc_cpp and "GetFuelPercent" in npc_cpp and "GetTireIntegrity" in npc_cpp, "mechanic dialogue must inspect actual vehicles"
assert "UGTTSocialWorldSubsystem" in world_h and "OnWorldBeginPlay" in world_h
for token in ["Bent Axle", "Community Hall", "TavernInterior", "HallInterior", "GTTSocialNPC", "GTTSocialVenueDoor"]:
    assert token in world_cpp, f"world social venue wiring missing {token}"
assert "[0.0.28]" in changelog and "Village Social Life" in changelog
assert "Tavern/community-hall interiors" in playtest and "time-aware" in playtest.lower()
assert "Verify social venues milestone" in workflow and "verify_social_venues.py" in workflow

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
unchecked = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + unchecked
assert (checked, total) == (120, 130), f"roadmap checklist is {checked}/{total}, expected 120/130"
percent = round(checked / total * 100, 1)
segments = round(checked / total * 20)
bar = "█" * segments + "░" * (20 - segments)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{unchecked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert f"{bar} {percent:.1f}%" in roadmap
assert "- [x] Tavern/community-hall interiors" in roadmap
assert "- [x] NPC dialogue/social encounters" in roadmap

print(f"Social venues sanity OK: {checked}/{total} ({percent:.1f}%), bar {segments}/20")
