from pathlib import Path
import re

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

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
):
    if token not in roadmap:
        raise SystemExit("SWIR roadmap SVG-only presentation missing: " + token)

completed = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
remaining = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = completed + remaining
if (completed, remaining, total) != (125, 5, 130):
    raise SystemExit(
        f"Roadmap progress unexpectedly changed: {completed}/{total} with {remaining} remaining; "
        "runtime-only acceptance must remain honest"
    )
if "DONE-125%2F130" not in roadmap or "ROADMAP-96.2%25" not in roadmap or "| **125** | **5** | **130** | **96.2%** |" not in roadmap:
    raise SystemExit("ROADMAP-PROGRESS numeric dashboard is not synchronized with the 125/130 checklist")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    raise SystemExit("Roadmap must embed exactly one canonical progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit("Legacy text/Unicode roadmap progress meter must not return")

print("Native load-transfer / grade / heavy-haul chassis-response milestone verified with SVG-only progress presentation.")