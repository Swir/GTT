# GTT 0.0.41 — Native Chaos Powertrain Contract Playtest

## Purpose
Validate the first complete source-level Native Chaos mechanical contract: rig + wheels + engine + transmission + differential, while preserving the proven legacy drivetrain until an authored skeletal/physics vehicle passes real Unreal runtime acceptance.

## Canonical setup
1. Add a `UChaosWheeledVehicleMovementComponent` to a test vehicle using one of the canonical persistent IDs.
2. Call `ConfigureNativeMovementFromCanonicalSpec` before accepting Native Chaos takeover.
3. Confirm the setup reports both WHEELS and POWERTRAIN configuration summaries.
4. Verify engine torque, max RPM and idle RPM match `FGTTChaosVehicleSpec`.
5. Verify all forward ratios, reverse ratio and final-drive ratio match the canonical spec.
6. Verify differential type maps correctly for RWD/FWD/AWD layouts.

## Acceptance gate
1. Break only engine torque: bridge must remain WAITING and legacy dynamics must remain enabled.
2. Restore torque and break one forward ratio: bridge must remain WAITING.
3. Restore gearing and use the wrong differential layout: bridge must remain WAITING.
4. Restore canonical powertrain but break a wheel setup or rig socket: bridge must remain WAITING.
5. Only a valid rig + canonical wheels + canonical powertrain may mark native movement READY and disable legacy dynamics.

## Fieldmaster focus
For `RustyFieldmaster60`, exercise forward acceleration, reverse, braking, steering, gear changes, low-condition power loss, engine tuning, tire wear/tuning, fuel-empty lockout and trailer hitch regression once an authored skeletal/physics rig is available.

## Fleet regression
Repeat contract validation for `Rattleback82` and `Mulebox1200`. No vehicle may accept another vehicle's gearing, torque or differential profile.

## Demo/release gate
This milestone is still source/CI acceptance. Do not mark Native Chaos roadmap items complete and do not publish a Windows demo until a real UE 5.8 Win64 compile/package, packaged EXE runtime smoke test and visual approval have passed.
