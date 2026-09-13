from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")

radio_h = read("Source/GTT/Public/Radio/GTTRadioComponent.h")
radio_cpp = read("Source/GTT/Private/Radio/GTTRadioComponent.cpp")
settings = read("Source/GTT/Public/UI/GTTGameUserSettings.h")
playtest = read("Docs/PLAYTEST_0.0.31.md")
roadmap = read("Docs/ROADMAP.md")
changelog = read("CHANGELOG.md")
workflow = read(".github/workflows/project-sanity.yml")

for token in [
    "UAudioComponent",
    "USoundWaveProcedural",
    "GenerateTrackPcm",
    "StartCurrentTrackAudio",
    "HasAudibleProgram",
    "RadioSampleRate",
]:
    assert token in radio_h, f"radio header missing {token}"

for token in [
    "GTTOriginalRadioAudio",
    "QueueAudio",
    "SetSampleRate",
    "GRAVEL FM",
    "BARNBEAT 96",
    "RUST & DIESEL",
    "NIGHT SHIFT",
    "MasterVolume",
    "RadioVolume",
    "TrackDurationSeconds",
]:
    assert token in radio_cpp, f"radio runtime missing {token}"

assert "float MasterVolume" in settings and "float RadioVolume" in settings
assert "third-party music" in playtest.lower() and "procedural" in playtest.lower()
assert "0.0.31" in playtest
assert "[0.0.31]" in changelog and "Original Radio Audio" in changelog
assert "Verify original radio audio milestone" in workflow
assert "verify_radio_audio.py" in workflow

# No external copyrighted music asset is required by this milestone.
audio_exts = {".wav", ".mp3", ".ogg", ".flac", ".m4a", ".aac"}
external_audio = [p for p in ROOT.rglob("*") if p.is_file() and p.suffix.lower() in audio_exts]
assert not external_audio, f"unexpected external audio files: {external_audio}"

# The radio-audio roadmap task must be complete and the SWIR dashboard must be exact.
assert "- [x] Original/royalty-cleared music and radio audio assets" in roadmap
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "<!-- ROADMAP-PROGRESS:START -->" in roadmap and "<!-- ROADMAP-PROGRESS:END -->" in roadmap
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
unckecked = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + unckecked
assert total == 130, f"unexpected roadmap total: {total}"
percent = round(checked / total * 100, 1)
segments = round(checked / total * 20)
bar = "█" * segments + "░" * (20 - segments)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{unckecked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert f"{bar} {percent:.1f}%" in roadmap
assert checked >= 124, f"radio milestone did not advance roadmap: {checked}/{total}"

print(f"Original radio audio sanity OK: 4 synthesized stations, 16 deterministic programs; roadmap {checked}/{total} ({percent:.1f}%)")
