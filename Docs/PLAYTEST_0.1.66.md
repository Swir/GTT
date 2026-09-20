# GTT 0.1.66 — Native Fieldmaster HUD & Trailer Brake Safety UX Playtest

## Scope

This milestone closes a presentation/gameplay integration gap in the native Rusty Fieldmaster 60 takeover path. The native tractor now participates in the normal player HUD instead of falling through to the on-foot combat fallback, and the existing authoritative Chaos hill-haul / trailer-brake thermal state is translated into concise driver-facing feedback.

This document does **not** close any Native Chaos, authored-trailer, Win64 package, or demo-release roadmap gate. Those gates still require real UE 5.8 packaged runtime evidence from the exact candidate.

## Source sanity

Run from the repository root:

```bash
python Scripts/verify_v0_1_66_native_fieldmaster_hud.py
python Scripts/verify_demo_hud.py
python Scripts/verify_fieldmaster_native_pawn.py
python Scripts/verify_v0_1_63_trailer_brake_thermal_control.py
python Scripts/verify_v0_1_64_trailer_brake_runaway_safety.py
```

Expected result: every verifier exits successfully.

## UE 5.8 packaged playtest

Use the exact Win64 candidate produced by the canonical sealed acceptance pipeline. Do not substitute an Editor-only PIE session for the packaged acceptance result.

### 1. Native takeover presentation

1. Load a save where the player owns the Rusty Fieldmaster 60 and native takeover prerequisites are valid.
2. Enter the native Fieldmaster.
3. Confirm the lower-left vehicle panel remains visible after possession transfers to `AGTTFieldmasterNativePawn`.
4. Confirm the status line shows vehicle display name, speed, fuel liters, condition, tire integrity and current tow-load percentage.
5. Confirm the bottom context hint is a vehicle hint (`F exit`, radio, save/load) and does not show on-foot combat controls.

### 2. Cold / normal trailer brakes

1. Attach the authored farm trailer with a legal heavy-haul load.
2. Drive on level road without sustained braking.
3. Confirm no thermal warning is shown while the thermal state is Normal and no hill/descent assist is active.
4. Confirm normal vehicle/objective/radio HUD remains readable and no raw debug telemetry wall appears.

### 3. Descent assist and heat build

1. Take the loaded trailer onto the hill-haul validation descent.
2. Release throttle and allow downhill tow braking to engage.
3. Confirm a concise `DESCENT ASSIST` message appears with live brake heat.
4. Continue the descent long enough to enter Hot.
5. Confirm `TRAILER BRAKES HOT` is shown and the displayed heat increases consistently with runtime telemetry.

### 4. Fade and critical safety

1. Continue sustained loaded-trailer descent until the movement component enters Fading.
2. Confirm `TRAILER BRAKE FADE` appears with heat and current brake authority.
3. If the exact test route safely reaches Critical, confirm `TRAILER BRAKES CRITICAL`.
4. At the configured high-load / steep-grade / speed condition, confirm `TRAILER RUNAWAY ASSIST` has priority over the generic critical message.
5. Confirm the warning color becomes clearly urgent without adding extra permanent HUD rows.

### 5. Cooling recovery

1. Leave the descent or stop applying the downhill tow-brake condition.
2. Confirm `TRAILER BRAKES COOLING` is shown while meaningful residual heat remains.
3. Confirm the warning disappears after the thermal system returns to Normal / low heat.
4. Confirm brake authority restores according to the existing 0.1.63/0.1.64 thermal model.

### 6. Hill hold

1. Stop with a loaded trailer on a qualifying uphill grade.
2. Confirm `HILL HOLD ACTIVE` appears while hill hold is authoritative.
3. Apply throttle and confirm the hint clears as normal driving resumes.

## Regression checks

- Legacy `AGTTVehicleBase` vehicle HUD remains unchanged.
- Native Rattleback/Mulebox road-vehicle HUD and roadside-recovery line remain unchanged.
- Wanted/Warden/objective/radio presentation remains contextual.
- On-foot combat controls still appear only when the controlled pawn is not a vehicle.
- No 0.0.38-era raw `VEHICLE DYNAMICS`, tuning, or controls telemetry wall returns.
- The 0.1.63 thermal fade and 0.1.64 runaway mitigation remain movement-authoritative; HUD code only reads telemetry and never applies drivetrain/brake forces.

## Acceptance boundary

Repository/source verification can prove the HUD wiring and regression contract, but it cannot prove final font sizing, overlap safety, color legibility, or packaged runtime behavior. The milestone is ready to merge when source CI is green, but demo readiness remains **NOT READY** until the exact UE 5.8 Win64 candidate passes package/smoke/runtime evidence and human visual acceptance.

Roadmap remains **125/130 = 96.2%**.
