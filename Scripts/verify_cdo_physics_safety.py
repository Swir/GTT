#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source"
PACKAGE = ROOT / "Scripts" / "package_windows.ps1"
FORBIDDEN = (
    "SetMassOverrideInKg(",
    "SetCenterOfMass(",
    "GetSimplePhysicalMaterial(",
    "SetPhysMaterialOverride(",
)

constructor_re = re.compile(
    r"(?m)^\\s*([AU][A-Za-z0-9_]*)::\\1\\s*\\([^;{}]*\\)\\s*(?::[^{}]+)?\\{"
)

def matching_brace(text: str, open_pos: int) -> int:
    depth = 0
    i = open_pos
    quote = None
    line_comment = False
    block_comment = False
    escape = False
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""
        if line_comment:
            if ch == "\n":
                line_comment = False
            i += 1
            continue
        if block_comment:
            if ch == "*" and nxt == "/":
                block_comment = False
                i += 2
            else:
                i += 1
            continue
        if quote:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == quote:
                quote = None
            i += 1
            continue
        if ch == "/" and nxt == "/":
            line_comment = True
            i += 2
            continue
        if ch == "/" and nxt == "*":
            block_comment = True
            i += 2
            continue
        if ch in ('"', "'"):
            quote = ch
            i += 1
            continue
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1

failures: list[str] = []
for path in SOURCE.rglob("*.cpp"):
    text = path.read_text(encoding="utf-8", errors="replace")
    for match in constructor_re.finditer(text):
        open_pos = text.find("{", match.start())
        close_pos = matching_brace(text, open_pos)
        if close_pos < 0:
            failures.append(f"{path.relative_to(ROOT)}: unable to parse constructor {match.group(1)}")
            continue
        body = text[open_pos : close_pos + 1]
        for token in FORBIDDEN:
            if token in body:
                line = text.count("\n", 0, open_pos + body.index(token)) + 1
                failures.append(
                    f"{path.relative_to(ROOT)}:{line}: {token[:-1]} is forbidden during native CDO construction"
                )

package_text = PACKAGE.read_text(encoding="utf-8", errors="replace")
for token in ("-IgnoreCookErrors", "KnownUE58CookError"):
    if token in package_text:
        failures.append(f"{PACKAGE.relative_to(ROOT)}: cook suppression token remains: {token}")

if failures:
    print("[GTT][AUDIT][FAIL] native CDO / cook fail-closed contract")
    for failure in failures:
        print(f" - {failure}")
    sys.exit(1)

print("[GTT][AUDIT][PASS] no unsafe native-CDO body-instance tuning; Win64 cook is fail-closed")
