# GTT 0.1.19 Playtest — Loaded authored-trailer runtime exercise

Use this checklist on the next qualifying UE 5.8 Win64 package. Source CI validates only the implementation contract.

## Prerequisites

1. Run the normal Win64 evidence workflow on a `self-hosted`, `windows`, `x64`, `unreal-5.8` runner.
2. Confirm preflight, UAT package, packaged EXE smoke, deterministic gameplay scenario, Native Chaos runtime and deterministic drivetrain evidence all PASS first.
3. Keep the packaged process alive for at least 172 seconds; the loaded authored-trailer motion scenario starts at 126 seconds.

## loaded authored-trailer motion

1. At `NATIVE_TRAILER_SCENARIO_BEGIN`, verify the earlier drivetrain scenario has already released control of the Fieldmaster.
2. Confirm the final authored rig and `tow_eye` are present. The test must fail rather than silently use greybox fallback if the authored rig is absent.
3. Confirm the trailer is reset near the Fieldmaster, its authored `tow_eye` is aligned to `rear_hitch`, cargo is loaded, and `AttachToNativeFieldmaster` succeeds.
4. Confirm `AUTHORED_READY` only appears with active authored presentation, dual wheel contact and hitch error <= 80 cm.
5. During `LOADED_MOTION`, visually confirm the tractor pulls the loaded trailer instead of dragging a static or detached body.
6. Confirm both authored wheels remain grounded through the steering oscillation and the trailer does not hover, tunnel or jackknife uncontrollably.
7. Confirm at least eight `NATIVE_TRAILER_SCENARIO_SAMPLE` lines show: speed >= 4 km/h, `loaded=1`, `attached=1`, `active=1`, `contacts=1.0`, both wheel flags = 1, and hitch error <= 80 cm.
8. Confirm measured tow distance reaches at least 900 cm while cargo remains loaded.
9. Observe articulation and hitch error during the turn. Any hitch excursion above the hard 110 cm envelope must fail acceptance.
10. Confirm cargo integrity remains plausible and no unexplained axle loss occurs.
11. Confirm the final controlled stop reaches <= 1.5 km/h while the trailer is still attached, loaded and axle-intact.
12. Confirm exactly one `NATIVE_TRAILER_SCENARIO_COMPLETE result=PASS` is present and no `phase=DIAGNOSTIC result=FAIL` marker exists.

## Evidence and regression

13. `NATIVE_TRAILER_RUNTIME.json` must report `result=PASS`, `deterministic_loaded_tow=PASS`, at least eight `safe_loaded_motion_samples`, >= 4 km/h max tow speed and >= 900 cm tow distance.
14. The manifest Git SHA must match `BUILD_INFO.json`, `NATIVE_CHAOS_RUNTIME.json` and `NATIVE_DRIVETRAIN_SCENARIO.json`.
15. Re-run the earlier 33-step gameplay scenario checks: traffic, NPCs, mission, combat, police pursuit, roadblock/spikes, post-spike escape, persistence and workshop recovery must remain green.
16. Confirm Fieldmaster automatic gears and forward/reverse interlock still pass before the trailer phase.
17. Confirm trailer collision damage, cargo integrity and roadside recovery still work outside the deterministic evidence route.
18. Confirm greybox fallback still keeps gameplay recoverable when no authored rig exists, but does not qualify for final trailer acceptance.
19. Perform rendered visual acceptance separately: trailer materials/mesh, wheel rotation/contact, hitch alignment, world composition, vehicles, character/weapons, HUD, lighting and placeholder clutter.
20. Do not close any of the five remaining Roadmap blockers unless the corresponding real UE 5.8/Win64/runtime/asset requirement is actually proven.
