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
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h",
    "Source/GTT/Public/Wanted/GTTWantedComponent.h",
]


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

    forbidden = ["Binaries", "Intermediate", "DerivedDataCache", "Saved"]
    present_forbidden = [name for name in forbidden if (ROOT / name).exists()]
    if present_forbidden:
        fail("Generated Unreal directories should not be committed: " + ", ".join(present_forbidden))

    print("[OK] GTT repository structure and project descriptor look sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
