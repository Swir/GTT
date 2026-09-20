# GTT 0.1.60 — Authored Trailer UE Import Automation Acceptance

This acceptance gate converts the deterministic project-owned glTF source recipe into a repeatable Unreal Engine 5.8 editor import. It is a blocker-reduction step for the authored-trailer roadmap item, not proof that a packaged Windows candidate has passed.

## What the automation now owns

- `Scripts/import_gtt_farm_trailer_unreal.ps1` regenerates and verifies the source rig before Unreal is started.
- `Scripts/Unreal/import_gtt_farm_trailer.py` uses UE 5.8 Interchange in unattended editor mode.
- Import is forced to a skeletal mesh at `/Game/GTT/Vehicles/Trailer/SK_GTT_FarmTrailer`.
- The source rig exports socket anchors with Unreal Interchange's `SOCKET_` naming convention under the `body` skeleton root.
- The editor script normalizes imported socket names to `socket_hitch`, `socket_cargo`, `socket_axle_l`, `socket_axle_r`, matching the runtime presentation bridge.
- A PhysicsAsset must be imported/created and assigned before the script emits PASS.
- Required `body`, `wheel_l`, `wheel_r` hierarchy is checked before save.
- Source CI verifies the automation contract but never fabricates UE runtime results.

## Real UE 5.8 execution

On a Windows machine/runner with Unreal Engine 5.8 installed:

```powershell
pwsh -File Scripts/import_gtt_farm_trailer_unreal.ps1 `
  -UnrealEditorCmd "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
```

The command must fail unless the Unreal log contains exactly the machine-readable success family:

`AUTHORED_TRAILER_IMPORT result=PASS`

Any `AUTHORED_TRAILER_IMPORT result=FAIL` is a gate failure.

## Editor acceptance checks

1. Confirm the resulting asset is a SkeletalMesh at `/Game/GTT/Vehicles/Trailer/SK_GTT_FarmTrailer`.
2. Confirm `body`, `wheel_l`, `wheel_r` exist and both wheel bones are children of `body`.
3. Confirm final mesh sockets exist with the exact lowercase runtime names and sit on the intended hitch/cargo/axle locations.
4. Confirm a PhysicsAsset is assigned and collision bodies are sensible for the body/wheels.
5. Open the existing 0.1.60 authored-trailer runtime bridge playtest and verify placeholder takeover only occurs after its strict validation.
6. Drive the loaded Native Fieldmaster scenario and collect the existing same-SHA authored-trailer runtime evidence.
7. Reject the candidate for wheel clipping/floating, hitch misalignment, scale/orientation mismatch, placeholder overlap or malformed PhysicsAsset.

## Gate boundary

A successful headless editor import materially reduces the authored-asset setup gap but does **not** close the roadmap checkbox by itself. Closure still requires real UE 5.8 authored asset review, packaged Win64 runtime evidence from the exact candidate SHA and rendered visual acceptance. The roadmap remains 125/130 (96.2%) until those conditions are true.
