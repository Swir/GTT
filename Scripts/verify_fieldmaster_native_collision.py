from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
environment = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativeEnvironment.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.48.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.48.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

for token in [
    "virtual void NotifyHit",
    "LastImpactDamageTimeSeconds",
    "ApplyNativeImpactDamage",
]:
    assert token in header, f"native collision API missing {token}"

for token in [
    "AGTTFieldmasterNativePawn::NotifyHit",
    "Super::NotifyHit",
    "NormalImpulse.Size()",
    "GetMesh()->GetMass()",
    "ImpactDamageCooldownSeconds",
    "MinimumImpactSpeedKmh",
    "ApplyNativeImpactDamage(ImpactSpeedKmh",
    "SyncLegacyMirror()",
    "Movement->SetBrakeInput(1.0f)",
    "MigrationSnapshot.ConditionPercent",
    "MigrationSnapshot.TireIntegrity",
]:
    assert token in environment, f"native collision behavior missing {token}"

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
    "ROADMAP-96.2%25",
    "DONE-125%2F130",
    "| **125** | **5** | **130** | **96.2%** |",
):
    assert token in roadmap, f"roadmap SVG-only presentation missing {token}"
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE), "legacy text/Unicode roadmap progress meter must not return"
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "Native Fieldmaster Collision & Breakdown Integration" in playtest
assert "Native Fieldmaster Collision & Breakdown Integration" in changelog
assert "125/130 (96.2%)" in changelog
assert "Verify Fieldmaster native collision damage" in workflow

print("Fieldmaster native collision/breakdown sanity passed; roadmap remains 125/130 (96.2%) with SVG-only progress")
