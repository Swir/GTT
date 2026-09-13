# GTT 0.0.38 — Demo HUD Presentation Playtest

## Goal
Validate that the normal gameplay HUD reads like a game interface rather than a live debug console while preserving the information needed for driving, combat, missions, police/ranger escalation, economy and saving.

This milestone is a presentation/readability pass. It is **not** approval of a public demo and does not replace a real Unreal Engine 5.8 Win64 package/runtime smoke test.

## 1. Clean on-foot baseline
1. Start a normal sandbox session with no wanted or wildlife alert.
2. Stand in the village without starting an activity.
3. Confirm the top-left persistent line is compact: clock/day, garage occupancy and cash.
4. Confirm quiet police/warden state does not consume permanent HUD rows.
5. Confirm only one primary objective is shown at the top-right.
6. Confirm the bottom-right hint is the short on-foot context hint rather than the old full controls wall.

## 2. Objective priority / no stacked wall
Exercise several gameplay states across separate runs: story objective, Borrowed Tractor, Farm Job, Rural Work, Heavy Haul, Night Shift Favor and Bent Axle Brawl.

Expected:
- exactly one `CURRENT OBJECTIVE` block is presented at a time;
- immediate activities (brawl / haul / active work) take priority over background campaign objectives;
- locked/completed campaign arcs do not create extra permanent rows;
- activity feedback may appear beneath the primary objective without recreating the previous full-screen stack.

## 3. Police and ranger escalation
1. Raise wanted from 0 to at least 3.
2. Confirm `WANTED` and live police counts appear only while wanted is active.
3. Raise wildlife alert through the poaching loop and confirm the Warden row appears only while its alert is non-zero.
4. Clear both systems and confirm those rows disappear again.

## 4. Vehicle driving HUD
Repeat with Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.

Expected bottom vehicle block:
- vehicle display name;
- speed;
- fuel percentage;
- condition percentage;
- tire integrity;
- ownership/stolen/borrowed state.

The old permanent raw drivetrain/contact/suspension tuning telemetry must not occupy the normal player HUD.

## 5. Contextual vehicle warnings
Trigger or reproduce each state independently:
- fuel below 15%;
- tire integrity below 30%;
- an existing mechanical/body fault;
- engine temperature above 108 C.

Expected: one concise warning line appears near the vehicle status. Fault state takes priority when multiple warnings are present.

## 6. Radio and combat
1. Enter a vehicle with radio off: no radio row should be shown.
2. Turn radio on and cycle stations: current radio information should appear.
3. Exit the vehicle and equip/cycle/drop Rural Arsenal items.
4. Confirm combat status remains readable near the lower-left without restoring the old full data stack.

## 7. Resolution/readability regression
Where available, inspect at 1280x720 and 1920x1080; optionally inspect an ultrawide resolution.

Expected:
- top-left status/alerts and top-right objective do not collide at normal desktop resolutions;
- vehicle/combat status remains near the bottom edge;
- context hints stay near the bottom-right;
- distant world-space labels remain governed by the 0.0.37 proximity culling pass.

## 8. Save/gameplay regression
Verify F5/F9 save/load, garage recall, wanted escalation, ranger escalation, traffic, story advancement, jobs, combat and all three drivable vehicles still operate through their existing systems. This HUD pass must not introduce duplicate gameplay state.

## Demo acceptance boundary
Do **not** publish the first demo from this source milestone alone. Public demo acceptance still requires a visually inspected packaged build, a verified UE 5.8 Win64 compile/package, successful packaged-EXE runtime smoke test, green relevant CI and no demo-critical blockers.
