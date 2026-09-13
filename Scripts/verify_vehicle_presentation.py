from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "Source/GTT/Public/World/GTTVehiclePresentationSubsystem.h").read_text(encoding="utf-8")
CPP = (ROOT / "Source/GTT/Private/World/GTTVehiclePresentationSubsystem.cpp").read_text(encoding="utf-8")
ROADMAP = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")

required_header = [
    "UGTTVehiclePresentationSubsystem",
    "FGTTVehiclePresentationRuntime",
    "HeadlightLeft",
    "RearLightLeft",
    "ReverseLightLeft",
]
required_cpp = [
    "RustyFieldmaster60",
    "Mulebox1200",
    "Rattleback82",
    "DayNightCycle->IsNight()",
    "GetConditionPercent()",
    "GetVelocity()",
    "bElectricalFlicker",
    "bBraking",
    "bReversing",
    "SetInnerConeAngle",
    "SetOuterConeAngle",
    "SetCastShadows(false)",
]

for token in required_header:
    assert token in HEADER, f"missing header integration: {token}"
for token in required_cpp:
    assert token in CPP, f"missing runtime integration: {token}"

assert 'TEXT("Rattleback82")' not in CPP, "Rattleback should use the tuned default layout instead of a redundant special case"
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in ROADMAP
assert "**125** | **5** | **130** | **96.2%**" in ROADMAP
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in ROADMAP
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in ROADMAP
assert "- [ ] Authored skeletal trailer wheel assets and final hitch sockets" in ROADMAP
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in ROADMAP
assert "- [ ] Full Win64 CI/build runner" in ROADMAP

print("Vehicle presentation milestone verified: gameplay-driven lights + honest 125/130 roadmap.")
