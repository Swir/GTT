from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")

presentation_h = read("Source/GTT/Public/Combat/GTTCombatPresentationComponent.h")
presentation_cpp = read("Source/GTT/Private/Combat/GTTCombatPresentationComponent.cpp")
character_h = read("Source/GTT/Public/Characters/GTTCharacter.h")
character_cpp = read("Source/GTT/Private/Characters/GTTCharacter.cpp")
citizen_h = read("Source/GTT/Public/NPC/GTTCitizenPawn.h")
citizen_cpp = read("Source/GTT/Private/NPC/GTTCitizenPawn.cpp")
types_h = read("Source/GTT/Public/Combat/GTTCombatTypes.h")
playtest = read("Docs/PLAYTEST_0.0.32.md")
roadmap = read("Docs/ROADMAP.md")
changelog = read("CHANGELOG.md")
workflow = read(".github/workflows/project-sanity.yml")

for token in [
    "UGTTCombatPresentationComponent",
    "PlayAttack",
    "RefreshEquippedWeapon",
    "PrimaryPart",
    "SecondaryPart",
    "DetailPart",
]:
    assert token in presentation_h, f"presentation header missing {token}"

for token in [
    "CombatWeaponPrimary",
    "CombatWeaponSecondary",
    "CombatWeaponDetail",
    "FarmShotgun",
    "Pitchfork",
    "WorkshopWrench",
    "FMath::Sin",
    "LoadObject<UStaticMesh>",
]:
    assert token in presentation_cpp, f"presentation runtime missing {token}"

weapon_names = [
    "Pitchfork", "Axe", "Branch", "Rake", "CowChain", "Shovel", "WorkshopWrench", "FarmShotgun"
]
for weapon in weapon_names:
    assert weapon in types_h, f"weapon profile missing {weapon}"
    assert weapon in presentation_cpp, f"weapon visual missing {weapon}"

assert "CombatPresentationComponent" in character_h
assert "CreateDefaultSubobject<UGTTCombatPresentationComponent>" in character_cpp
assert "PlayAttack(CombatComponent->GetEquippedWeapon())" in character_cpp
assert "RefreshEquippedWeapon" in character_cpp

for token in [
    "CombatProp",
    "CombatPropDetail",
    "HitReactionTimeRemaining",
    "AttackPresentationTimeRemaining",
    "UpdateCombatPresentation",
    "ConfigureCombatProp",
]:
    assert token in citizen_h or token in citizen_cpp, f"NPC presentation missing {token}"

assert "SetActorHiddenInGame(true)" not in citizen_cpp, "knockout should remain visually readable"
assert "HitReactionTimeRemaining = 0.34f" in citizen_cpp
assert "AttackPresentationTimeRemaining=0.30f" in citizen_cpp
assert "bUrgentSimulation" in citizen_cpp and "HitReactionTimeRemaining" in citizen_cpp

assert "0.0.32" in playtest
assert "[0.0.32]" in changelog and "Combat Presentation" in changelog
assert "Verify combat presentation milestone" in workflow
assert "verify_combat_presentation.py" in workflow

asset_exts = {".fbx", ".obj", ".blend", ".glb", ".gltf"}
imported_combat_assets = [
    p for p in ROOT.rglob("*")
    if p.is_file() and p.suffix.lower() in asset_exts and "combat" in str(p).lower()
]
assert not imported_combat_assets, f"unexpected imported combat assets: {imported_combat_assets}"

assert "- [x] Authored combat animations / weapon models / hit reactions" in roadmap
for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
):
    assert token in roadmap, f"roadmap SVG-only presentation missing {token}"
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
uncheked = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + uncheked
assert total == 130, f"roadmap total drifted: {total}"
percent = round(checked / total * 100, 1)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{uncheked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(
    r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}",
    roadmap,
    flags=re.MULTILINE,
), "legacy text/Unicode roadmap progress meter must not return"
assert checked >= 125, f"combat presentation milestone did not advance roadmap: {checked}/{total}"

# Native Chaos and Unreal runner tasks must remain honest/open in this source-only milestone.
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "- [ ] Full Win64 CI/build runner" in roadmap

print(f"Combat presentation sanity OK: 8 weapon visuals + player attack motion + NPC reactions; roadmap {checked}/{total} ({percent:.1f}%), SVG-only progress verified")
