# GTT 0.0.39 — Vehicle Presentation & Road Readability Playtest

## Goal
Validate that the source-built fleet now reads like functioning road vehicles at night without creating a disconnected showcase system. Lighting must follow the real day/night clock, engine state, vehicle movement and damage state for Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.

## Core acceptance
1. Launch the normal sandbox and confirm all three driveable vehicles remain enterable, driveable, damageable, refuelable and garage-compatible.
2. During daylight, start each vehicle. Headlights and normal tail lamps should remain off unless braking/reversing requires a lamp state.
3. Advance/play into night (`AGTTDayNightCycle::IsNight`). With the engine running, both front headlights should illuminate automatically and rear position lamps should become visible.
4. Stop from road speed. Rear lamps should rise to the brighter brake state during meaningful deceleration and remain bright while stopped with the engine running.
5. Reverse each vehicle. White reversing lamps should activate only while the vehicle is actually moving backwards.
6. Turn the engine off/exit. Headlights and ordinary night tail lighting must switch off rather than remaining as orphaned world lights.

## Damage integration
1. Reduce a vehicle below roughly 28% condition without destroying it.
2. At night with the engine running, verify intermittent electrical/headlight instability appears.
3. Repair the same vehicle above the low-condition threshold and confirm stable headlights return.
4. Confirm the damage effect does not modify saved condition, tuning, fuel or tire values; it is presentation derived from authoritative vehicle state.

## Fleet/layout check
- Rusty Fieldmaster 60: headlights should sit around the high tractor nose; rear lamps should read around the rear body/hitch area without replacing the trailer hitch logic.
- Rattleback 82: default compact-road layout should place lights at car-like front/rear heights.
- Mulebox 1200: front/rear spacing should match the longer van body.
- Check the lamps while detached panels/damage smoke are present and ensure there are no collision interactions: presentation components must be non-physical lights only.

## Night-road demo pass
1. Start near the village before 21:30 and drive through the transition into night.
2. Drive the main loop past the 0.0.37 street lamps and roadside reflectors.
3. Confirm vehicle headlights improve road-edge readability without washing out the whole scene; shadows are intentionally disabled on these prototype lights for performance.
4. Trigger traffic/police gameplay and verify the player vehicle presentation remains readable while wanted/ranger HUD context stays intact.
5. Repeat at 720p and 1080p with the 0.0.38 contextual HUD.

## Regression
- Save/load and garage recall all three vehicles.
- Fuel depletion still stalls the engine and therefore extinguishes engine-dependent lights.
- Vehicle breakdown still prevents normal driving and lights do not create propulsion/physics changes.
- Radio, combat, traffic, police, ranger, missions, farm jobs and trailer attachment continue functioning.
- Native Chaos acceptance remains unchanged; this milestone must not disable the legacy drivetrain or claim a skeletal/physics rig exists.

## Demo acceptance boundary
This milestone improves night driving presentation but does **not** approve a public demo by itself. A demo still requires a real UE 5.8 Win64 package, packaged-EXE runtime smoke test, green relevant CI and visual approval from the packaged build. No EXE verification is implied by source sanity.
