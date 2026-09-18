#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
required = {
    "Source/GTT/Public/UI/GTTGameUserSettings.h": [
        "UGTTGameUserSettings", "LookSensitivity", "bInvertLookY", "HUDScale",
        "bSubtitlesEnabled", "SubtitleScale", "bReduceCameraMotion", "ColorVisionMode",
        "MasterVolume", "RadioVolume", "bControllerVibration", "ControllerDeadZone"
    ],
    "Source/GTT/Private/UI/GTTGameUserSettings.cpp": [
        "ApplyGTTSettings", "SaveSettings", "ResetGTTSettings", "FMath::Clamp"
    ],
    "Source/GTT/Private/Characters/GTTCharacter.cpp": [
        "GTTGameUserSettings", "LookSensitivity", "bInvertLookY"
    ],
    "Config/DefaultEngine.ini": ["GameUserSettingsClassName=/Script/GTT.GTTGameUserSettings"],
    "Config/DefaultInput.ini": [
        "Gamepad_LeftY", "Gamepad_LeftX", "Gamepad_RightX", "Gamepad_RightY",
        "Gamepad_FaceButton_Bottom", "Gamepad_FaceButton_Left", "Gamepad_FaceButton_Right",
        "Gamepad_RightTrigger", "Gamepad_RightShoulder", "Gamepad_DPad_Down",
        "Gamepad_RightTriggerAxis", "Gamepad_LeftTriggerAxis"
    ],
    "Docs/PLAYTEST_0.0.26.md": ["Controller smoke test", "Persistent settings", "Accessibility regression"],
    "CHANGELOG.md": ["[0.0.26]", "Player Experience", "controller", "GameUserSettings"],
    ".github/workflows/project-sanity.yml": ["Verify player experience milestone", "verify_player_experience.py"],
}

for rel, tokens in required.items():
    path = ROOT / rel
    if not path.is_file():
        raise SystemExit(f"[FAIL] missing {rel}")
    text = path.read_text(encoding="utf-8")
    missing = [token for token in tokens if token not in text]
    if missing:
        raise SystemExit(f"[FAIL] {rel} missing hooks: {missing}")

roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
):
    if token not in roadmap:
        raise SystemExit(f"[FAIL] roadmap SVG-only structure missing: {token}")
if "- [x] Accessibility/settings" not in roadmap or "- [x] Controller support" not in roadmap:
    raise SystemExit("[FAIL] player-experience roadmap tasks not checked")
checks = re.findall(r'^- \[(x| )\]', roadmap, flags=re.M | re.IGNORECASE)
done = sum(x.lower() == "x" for x in checks)
total = len(checks)
remaining = total - done
percent = round(done * 100.0 / total, 1)
for token in [
    f"ROADMAP-{percent:.1f}%25",
    f"DONE-{done}%2F{total}",
    f"| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |",
]:
    if token not in roadmap:
        raise SystemExit(f"[FAIL] roadmap dashboard mismatch: missing {token}")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    raise SystemExit("[FAIL] roadmap must embed exactly one progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit("[FAIL] legacy text/Unicode roadmap progress meter must not return")
print(f"[OK] GTT 0.0.26 player experience sane; roadmap {done}/{total} = {percent:.1f}% with SVG-only presentation.")
