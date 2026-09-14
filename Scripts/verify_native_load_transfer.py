from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/GTT/Private/Vehicles/GTTNativeStabilitySubsystem.cpp").read_text(encoding="utf-8")
hdr = (root / "Source/GTT/Public/Vehicles/GTTNativeStabilitySubsystem.h").read_text(encoding="utf-8")
roadmap = (root / "Docs/ROADMAP.md").read_text(encoding="utf-8")

required_cpp = [
    "LoadTransferGraceSeconds",
    "ComputeLoadTransferRisk",
    "LoadTransferSeconds",
    "front_rear_bias",
    "side_bias",
    "load_transfer=",
    "LoadTransferBrake",
    "FMath::Max3(GroundRisk, HeavyHaulRisk, ChassisRisk)",
]
required_hdr = [
    "LastLoadTransferRisk",
    "LastFrontRearBias",
    "LastSideBias",
    "ComputeLoadTransferRisk",
]

missing = [token for token in required_cpp if token not in cpp]
missing += [token for token in required_hdr if token not in hdr]
if missing:
    raise SystemExit("Native load-transfer contract missing: " + ", ".join(missing))

if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    raise SystemExit("SWIR roadmap style lock marker missing")
if "125 / 130" not in roadmap and "125/130" not in roadmap:
    raise SystemExit("Roadmap progress unexpectedly changed; runtime-only acceptance must remain honest")

print("Native load-transfer / grade / heavy-haul chassis-response milestone verified.")