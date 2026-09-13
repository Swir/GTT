from pathlib import Path

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

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "ROADMAP-96.2%25" in roadmap
assert "DONE-125%2F130" in roadmap
assert "| **125** | **5** | **130** | **96.2%** |" in roadmap
assert "███████████████████░ 96.2%" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "Native Fieldmaster Collision & Breakdown Integration" in playtest
assert "Native Fieldmaster Collision & Breakdown Integration" in changelog
assert "125/130 (96.2%)" in changelog
assert "Verify Fieldmaster native collision damage" in workflow

print("Fieldmaster native collision/breakdown sanity passed; roadmap remains 125/130 (96.2%)")
