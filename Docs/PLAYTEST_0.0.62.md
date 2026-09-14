# GTT 0.0.62 Playtest — Native Authored Wheel & Suspension Acceptance

## Goal
Verify that the Native Rusty Fieldmaster 60 uses the canonical authored Chaos wheel classes and suspension defaults at runtime, and that a broken wheel/suspension setup safely falls back to the legacy Fieldmaster instead of remaining active.

## Required runtime
This playtest requires Unreal Engine 5.8 with the Native Fieldmaster rig and Physics Asset available. Repository sanity checks validate source contracts only; they do not replace Win64 compile/package/runtime acceptance.

## Test A — Authored setup baseline
1. Start a session with the owned Rusty Fieldmaster 60 and allow Native takeover.
2. Capture `NATIVE_WHEEL_SETUP_EVIDENCE` and `NATIVE_PHYSICS_EVIDENCE`.
3. Confirm four canonical wheel setups are present.

Expected: front wheels use the Fieldmaster front wheel class, rear wheels use the rear wheel class, and telemetry reports plausible non-zero radii, suspension travel, spring rates and damping values.

## Test B — Suspension movement
1. Drive slowly across a shallow ditch, diagonal mound and small crest.
2. Observe wheel/body motion and capture wheel/contact evidence.

Expected: the tractor remains stable, wheel contact changes match terrain, suspension has visible usable raise/drop travel and no wheel appears rigidly welded to the chassis or displaced from its canonical bone.

## Test C — Steering/drive ownership
1. Turn at low speed on flat ground.
2. Accelerate from rest and apply the handbrake.

Expected: front wheels provide steering, rear wheels provide engine drive and handbrake authority, while all wheels retain normal braking. GTT traction/stability controllers remain responsible for intervention; built-in Chaos ABS/TC are not unexpectedly active.

## Test D — Mud and load regression
1. Enter an authored mud zone, then repeat with the Heavy Timber Haul trailer loaded.
2. Capture `NATIVE_TERRAIN_RESPONSE`, `NATIVE_WHEEL_LOAD_EVIDENCE` and `NATIVE_WHEEL_SETUP_EVIDENCE`.

Expected: authored wheel/suspension acceptance remains valid while existing mud, tire, cross-axle and tow-load systems continue to alter gameplay.

## Test E — Broken authored setup fallback
1. In an editor-only test copy, deliberately invalidate one Native wheel class/default or wheel setup bone.
2. Activate Native takeover and leave the invalid state present beyond the runtime acceptance grace period.

Expected: validation reports `authored wheel/suspension setup invalid`, then `NATIVE_PHYSICS_FALLBACK` appears and control returns to the functioning legacy Fieldmaster.

## Test F — Recovery regression
1. Restore the canonical authored setup.
2. Restart/reload and repeat takeover.

Expected: Native acceptance succeeds again with no persistent failure latch.

## Demo gate
Do not publish a demo from source evidence alone. Demo still requires a verified UE 5.8 Win64 package, successful packaged-EXE smoke test, rendered visual acceptance, green relevant GitHub Actions and no demo-critical blockers.
