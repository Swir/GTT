#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

header = read("Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h")
pawn = read("Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp")
environment = read("Source/GTT/Private/Vehicles/GTTFieldmasterNativeEnvironment.cpp")
heavy = read("Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp")
garage = read("Source/GTT/Private/World/GTTGarageFleetSubsystem.cpp")
vehicle = read("Source/GTT/Private/Vehicles/GTTVehicleBase.cpp")
changelog = read("CHANGELOG.d/0.1.1.md")
roadmap = read("Docs/ROADMAP.md")
workflow = read(".github/workflows/project-sanity.yml")

checks = {
    "snapshot default is normalized healthy": "float ConditionPercent = 1.0f;" in header,
    "snapshot editor range documents normalized ratio": 'meta=(ClampMin="0.0", ClampMax="1.0")' in header,
    "legacy source is normalized": "return FMath::Clamp(Condition / MaxCondition, 0.0f, 1.0f);" in vehicle,
    "native import consumes legacy ratio directly": "Snapshot.ConditionPercent = LegacyVehicle->GetConditionPercent();" in pawn,
    "legacy mirror receives normalized ratio directly": "MigrationSnapshot.ConditionPercent,\n        MigrationSnapshot.FuelLiters" in pawn,
    "native collision damage uses ratio magnitude": "const float BodyDamageRatio = Severity * 0.13f" in environment,
    "native collision condition clamps to ratio": "MigrationSnapshot.ConditionPercent - BodyDamageRatio, 0.0f, 1.0f" in environment,
    "native collision log converts only for presentation": "MigrationSnapshot.ConditionPercent * 100.0f" in environment,
    "heavy haul native eligibility uses ratio threshold": "GetMigrationSnapshot().ConditionPercent < 0.40f" in heavy,
    "heavy haul reward factor uses ratio directly": "GetMigrationSnapshot().ConditionPercent, 0.40f, 1.0f" in heavy,
    "heavy haul no stale percent divisor": "GetMigrationSnapshot().ConditionPercent / 100.0f" not in heavy,
    "garage consumes native Fieldmaster ratio": "Snapshot.ConditionPercent = FMath::Clamp(State.ConditionPercent, 0.0f, 1.0f);" in garage,
    "old 0..100 impact damage removed": "Severity * 13.0f" not in environment and "0.0f, 100.0f" not in environment,
    "changelog records unit hardening": "Native Fieldmaster condition-unit hardening" in changelog,
    "sanity workflow wires unit verifier": "Verify Native Fieldmaster condition units" in workflow and "verify_fieldmaster_condition_units.py" in workflow,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Fieldmaster condition-unit verification failed: " + ", ".join(failed))

required_style = [
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->',
    'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"',
    '## 📊 Overall progress', '<!-- ROADMAP-PROGRESS:END -->'
]
for token in required_style:
    if token not in roadmap:
        raise SystemExit("SWIR roadmap style lock missing: " + token)

items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(v == 'x' for v in items)
total = len(items)
remaining = total - done
percent = round(done * 100.0 / total, 1) if total else 0.0
filled = round(done * 20.0 / total) if total else 0
bar = '█' * filled + '░' * (20 - filled)
for token in (
    f'ROADMAP-{percent:.1f}%25', f'DONE-{done}%2F{total}', 'STATUS-IN%20PROGRESS',
    f'{bar} {percent:.1f}%', f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap:
        raise SystemExit("Roadmap dashboard drift: missing " + token)
if (done, total, percent) != (125, 130, 96.2):
    raise SystemExit(f"Unit hardening must not claim build/art gates: {done}/{total} = {percent:.1f}%")

print(f"[OK] Native Fieldmaster health uses one normalized 0..1 contract across legacy mirror, collision, heavy haul and garage ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.")
