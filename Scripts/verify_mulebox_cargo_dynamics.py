from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
van_h = (root / 'Source/GTT/Public/Vehicles/GTTFarmVanPawn.h').read_text(encoding='utf-8')
van_cpp = (root / 'Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp').read_text(encoding='utf-8')
job_h = (root / 'Source/GTT/Public/Activities/GTTFarmJobDirector.h').read_text(encoding='utf-8')
job_cpp = (root / 'Source/GTT/Private/Activities/GTTFarmJobDirector.cpp').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')

required = {
    'van load setter': 'SetCargoLoadFactor' in van_h and 'SetCargoLoadFactor' in van_cpp,
    'persistent runtime cargo factor': 'CargoLoadFactor' in van_h,
    'cargo throttle consequence': 'CargoPowerLimit' in van_cpp and 'CaptureThrottleInput(Value * CargoPowerLimit)' in van_cpp,
    'cargo steering consequence': 'CargoSteerLimit' in van_cpp and 'CaptureSteeringInput(Value * CargoSteerLimit)' in van_cpp,
    'farm job loads Mulebox': 'LoadedMulebox->SetCargoLoadFactor(1.0f)' in job_cpp,
    'farm job unloads Mulebox': 'SetCargoLoadFactor(0.0f)' in job_cpp,
    'role economy bonus': 'MuleboxRoleBonus' in job_h and 'MULEBOX ROLE BONUS' in job_cpp,
    'failure cleanup': 'ClearLoadedVehicleCargoState();' in job_cpp,
}
missing = [name for name, ok in required.items() if not ok]
if missing:
    raise SystemExit('Mulebox cargo dynamics verification failed: ' + ', '.join(missing))

checks = re.findall(r'^\s*- \[(x| )\] ', roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(1 for state in checks if state.lower() == 'x')
total = len(checks)
if (done, total) != (125, 130):
    raise SystemExit(f'Roadmap checkbox count changed unexpectedly: {done}/{total}')
for token in (
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->', '<!-- ROADMAP-PROGRESS:END -->',
    '## 📊 Overall progress', '../assets/readme/progress-mini.svg', 'DONE-125%2F130', 'ROADMAP-96.2%25',
    '| **125** | **5** | **130** | **96.2%** |'):
    if token not in roadmap:
        raise SystemExit('Roadmap SVG-only dashboard is stale or inconsistent: ' + token)
if roadmap.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit('Roadmap must embed exactly one canonical progress-mini.svg')
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit('Legacy text/Unicode roadmap progress meter must not return')

print('Mulebox cargo dynamics verified; ROADMAP remains 125/130 (96.2%) with SVG-only progress.')
