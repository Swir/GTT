#!/usr/bin/env python3
"""Fast repository sanity checks that do not require Unreal Engine to be installed."""

from __future__ import annotations

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "GTT.uproject",
    "Config/DefaultEngine.ini",
    "Config/DefaultGame.ini",
    "Config/DefaultInput.ini",
    "Source/GTT.Target.cs",
    "Source/GTTEditor.Target.cs",
    "Source/GTT/GTT.Build.cs",
    "Source/GTT/GTT.cpp",
    "Source/GTT/Public/Characters/GTTCharacter.h",
    "Source/GTT/Public/Core/GTTGameplayStatics.h",
    "Source/GTT/Public/UI/GTTGameHUD.h",
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h",
    "Source/GTT/Public/Wanted/GTTWantedComponent.h",
    "Source/GTT/Public/Police/GTTPoliceDirector.h",
    "Source/GTT/Public/Police/GTTPoliceAIController.h",
    "Source/GTT/Public/Police/GTTPolicePawn.h",
]

EXPECTED_SOURCE_TOKENS = {
    "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp": [
        "AddForce(",
        "AddTorqueInRadians(",
        "NotifyVehicleStolen",
        "Wanted->AddHeat",
    ],
    "Source/GTT/Private/Police/GTTPoliceAIController.cpp": [
        "MoveToActor(",
        "GetPlayerWantedLevel",
    ],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": [
        "WANTED [",
        "GetConditionPercent",
        "BuildMissionText",
    ],
}


def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)


def main() -> int:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    if missing:
        fail("Missing required files: " + ", ".join(missing))

    try:
        project = json.loads((ROOT / "GTT.uproject").read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        fail(f"GTT.uproject is not valid JSON: {exc}")

    module_names = {module.get("Name") for module in project.get("Modules", [])}
    if "GTT" not in module_names:
        fail("GTT runtime module is not declared in GTT.uproject")

    enabled_plugins = {
        plugin.get("Name")
        for plugin in project.get("Plugins", [])
        if plugin.get("Enabled") is True
    }
    expected_plugins = {"EnhancedInput", "ChaosVehiclesPlugin"}
    missing_plugins = expected_plugins - enabled_plugins
    if missing_plugins:
        fail("Required plugins are not enabled: " + ", ".join(sorted(missing_plugins)))

    for relative_path, tokens in EXPECTED_SOURCE_TOKENS.items():
        path = ROOT / relative_path
        if not path.is_file():
            fail(f"Missing gameplay source: {relative_path}")
        text = path.read_text(encoding="utf-8")
        missing_tokens = [token for token in tokens if token not in text]
        if missing_tokens:
            fail(f"{relative_path} is missing expected gameplay hooks: {missing_tokens}")

    forbidden = ["Binaries", "Intermediate", "DerivedDataCache", "Saved"]
    present_forbidden = [name for name in forbidden if (ROOT / name).exists()]
    if present_forbidden:
        fail("Generated Unreal directories should not be committed: " + ", ".join(present_forbidden))

    print("[OK] GTT repository structure and gameplay hooks look sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
