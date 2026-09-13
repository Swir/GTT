from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")


terminal_h = read("Source/GTT/Public/World/GTTVehicleStyleTerminal.h")
terminal_cpp = read("Source/GTT/Private/World/GTTVehicleStyleTerminal.cpp")
subsystem_h = read("Source/GTT/Public/World/GTTVehicleStyleWorldSubsystem.h")
subsystem_cpp = read("Source/GTT/Private/World/GTTVehicleStyleWorldSubsystem.cpp")
roadmap = read("Docs/ROADMAP.md")
changelog = read("CHANGELOG.md")
playtest = read("Docs/PLAYTEST_0.0.29.md")
workflow = read(".github/workflows/project-sanity.yml")

assert "EGTTVehicleStyleService" in terminal_h
for service in ["TractorVisual", "OldCarVariant", "BodyPanels"]:
    assert service in terminal_h and service in terminal_cpp, f"missing customization service {service}"

assert "Cast<AGTTTractorPawn>" in terminal_cpp, "tractor package must target the real tractor class"
assert "Cast<AGTTOldCarPawn>" in terminal_cpp, "Rattleback package must target the real old-car class"
assert "InstallEngineUpgrade" in terminal_cpp and "InstallTireUpgrade" in terminal_cpp, "variants must reuse persistent tuning"
assert "SpendCash" in terminal_cpp, "customization must consume the real economy"
assert "GetDetachedPartCount" in terminal_cpp and "RepairVehicle" in terminal_cpp, "panel economy must consume the real breakable-part/repair loop"
assert "GTT_CustomVisual" in terminal_cpp and "NewObject<UStaticMeshComponent>" in terminal_cpp, "visual packages must create actual vehicle-attached geometry"
assert "UGTTVehicleStyleWorldSubsystem" in subsystem_h and "OnWorldBeginPlay" in subsystem_h
for token in ["TractorVisual", "OldCarVariant", "BodyPanels", "2350.0f"]:
    assert token in subsystem_cpp, f"workshop world wiring missing {token}"

assert "[0.0.29]" in changelog and "Workshop Customization" in changelog
assert "Fieldmaster visual upgrades" in playtest and "Rattleback street/performance variant" in playtest
assert "Replacement body-panel economy" in playtest
assert "Verify vehicle customization milestone" in workflow and "verify_vehicle_customization.py" in workflow

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "<!-- ROADMAP-PROGRESS:START -->" in roadmap and "<!-- ROADMAP-PROGRESS:END -->" in roadmap
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
unchecked = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + unchecked
assert total == 130, f"roadmap total drifted: {total}"
assert checked >= 123, f"roadmap regressed below Workshop Customization baseline: {checked}/{total}"
percent = round(checked / total * 100, 1)
segments = round(checked / total * 20)
bar = "█" * segments + "░" * (20 - segments)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{unchecked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert f"{bar} {percent:.1f}%" in roadmap
for task in ["Tractor visual upgrades", "Old-car visual/performance variants", "Replacement body-panel economy"]:
    assert f"- [x] {task}" in roadmap, f"roadmap task not completed: {task}"

print(f"Vehicle customization sanity OK: {checked}/{total} ({percent:.1f}%), bar {segments}/20")
