# GTT 0.1.29 Playtest — Packaged Farm Cargo Runtime Exercise

This milestone turns the Farm Cargo vertical slice into a deterministic packaged-runtime acceptance route. Source sanity is useful, but the Win64 rows below remain **unverified until executed on a qualifying Unreal Engine 5.8 Windows runner**.

## Contract and route setup

1. Launch the packaged candidate with `-GTTDemoSmokeScenario -GTTFarmCargoRuntimeScenario` and confirm exactly one `FARM_CARGO_RUNTIME_BEGIN version=1 route=feed-hill-wood` marker.
2. Confirm the Farm Cargo runtime route does not begin before the authored-trailer evidence window is complete; expected start delay is 180 seconds.
3. Confirm the scenario refuses to start when the FarmJobDirector is already active instead of overwriting a live contract.
4. Confirm an uncleared wildlife alert causes a deterministic diagnostic failure rather than bypassing the legal-work lockout.
5. Confirm Wanted heat left by the earlier police smoke route is cleared only inside the explicit evidence harness before legal contract acceptance.
6. Confirm the evidence route moves world time into the staffed cargo window without changing normal-game starting time settings.
7. Confirm a fresh profile can reach a Tier-2 cargo capability through evidence-only reputation seeding.
8. Confirm the selected active order is Tier 2 or higher and is backed by real Feed Depot stock, Hill Farm demand and North Wood Yard demand.
9. Confirm the scenario logs depot stock and both buyer demand counters before contract acceptance.
10. Confirm contract acceptance runs through the real Start terminal and leaves the director in `ReachPickup`.

## Physical pickup authority

11. Confirm cargo pickup runs through the real Pickup terminal and advances the director to `DeliverCargo`.
12. Confirm `UGTTFarmCargoAuthoritySubsystem` binds a non-null physical vehicle and stable vehicle ID at pickup.
13. Confirm the bound actor is the vehicle actually selected by the existing pickup rules (controlled Native Mulebox when applicable, otherwise the nearest valid legacy work vehicle).
14. Confirm the route records the existing delivery timer and cargo-integrity value instead of creating parallel copies.
15. Confirm cargo mass/vehicle load behavior still comes from the existing FarmJobDirector vehicle path.
16. Confirm normal payout, market, reputation and save state remain absent from the evidence subsystem itself.

## Wrong-vehicle rejection

17. Park the exact loaded vehicle outside the Hill Farm buyer radius and place the evidence decoy van inside the handoff zone.
18. Trigger the real Hill Farm terminal with the wrong vehicle present and confirm the director remains in `DeliverCargo`.
19. Confirm the cargo authority remains bound to the original exact loaded vehicle after the rejected attempt.
20. Confirm the rejected wrong vehicle does not produce payout, reputation or cargo-completion history changes.
21. Confirm the log contains `FARM_CARGO_RUNTIME phase=WRONG_VEHICLE result=PASS`.
22. Repeat the wrong-vehicle probe with the decoy stopped at zero speed; exact identity, not merely speed, must still reject it.
23. Repeat with the decoy closer to the terminal than the loaded vehicle; nearest-vehicle substitution must still fail.
24. Confirm a missing original loaded vehicle produces a failed handoff rather than silently rebinding to the decoy.

## Hill Farm and North Wood Yard continuity

25. Bring the exact loaded vehicle into the Hill Farm zone and stop it; confirm the terminal advances to `DeliverFinalStop`.
26. Confirm the authority vehicle pointer and persistent ID are unchanged across the Hill Farm relay.
27. Confirm Hill Farm does not clear the authority for a Tier-2 route.
28. Confirm the same delivery timer continues after Hill Farm rather than resetting for the final leg.
29. Confirm cargo integrity carries through the relay unchanged except for normal vehicle-condition damage rules.
30. Bring the same exact vehicle into North Wood Yard and stop it; confirm the final terminal completes the contract.
31. Confirm `AGTTFarmJobDirector` returns to `Idle` after final completion.
32. Confirm cargo authority is cleared after final completion.
33. Confirm the evidence decoy never becomes the bound cargo vehicle during the route.

## Economy, reputation and persistence

34. Confirm cash increases by a positive amount through the existing economy payout path.
35. Confirm Cargo Completed Runs increases by exactly one for the evidence contract.
36. Confirm logistics reputation increases by a positive amount for the successful chain.
37. Confirm `SaveProgress()` succeeds after final delivery.
38. Confirm `FARM_CARGO_RUNTIME_COMPLETE result=PASS` includes positive `payout_delta`, `cargo_runs_delta=1`, positive `reputation_delta`, `save=1` and `authority_cleared=1`.
39. Confirm no `FARM_CARGO_RUNTIME phase=DIAGNOSTIC result=FAIL` marker is present in a passing runtime log.
40. Confirm evidence-only logistics seeding, economy values and world time are restored to their captured baseline after the proof and the restored baseline is saved.

## Win64 evidence chain

41. Confirm `smoke_test_windows.ps1` keeps the packaged executable alive for at least 200 seconds and records `-GTTFarmCargoRuntimeScenario` in `RUNTIME_SMOKE.json`.
42. Run `evaluate_farm_cargo_runtime.ps1` and confirm it produces `FARM_CARGO_RUNTIME.json` with schema `gtt.farm-cargo-runtime.v1` and `result=PASS`.
43. Confirm the evaluator rejects a log missing the wrong-vehicle phase even when the final handoff succeeded.
44. Confirm the evaluator rejects non-positive payout or reputation deltas and any cargo-completion delta other than exactly one.
45. Confirm the evaluator rejects missing/failed post-delivery save evidence or uncleared cargo authority.
46. Confirm `evaluate_demo_candidate.ps1` now refuses a technical candidate when `FARM_CARGO_RUNTIME.json` is absent or failed.
47. Confirm the final Win64 evidence artifact includes `FARM_CARGO_RUNTIME.json` next to Native Chaos, drivetrain, trailer and runtime logs.
48. Confirm failure diagnostics also retain `FARM_CARGO_RUNTIME.json` when it exists.

## Regression and release honesty

49. Run `Scripts/verify_farm_cargo_authority.py`; 0.1.28 exact-vehicle handoff rules must remain green.
50. Run `Scripts/verify_farm_cargo_runtime.py`; the new source/runtime-evidence contract must pass.
51. Run `Scripts/generate_progress_svg.py --check`; progress card/mini geometry and values must remain deterministic.
52. Confirm `Docs/ROADMAP.md` remains exactly 125/130 (96.2%) with all five Win64/Native Chaos/authored-trailer blockers still open until real evidence exists.
53. Confirm `<!-- SWIR-ROADMAP-STANDARD:v1 -->` and the complete protected roadmap dashboard remain intact.
54. Confirm README stays on `<!-- SWIR-README-STANDARD:v2 -->`, retains Search Keywords and does not claim a downloadable demo.
55. Confirm source sanity alone does not create a GitHub Release.
56. Do not mark the demo ready until the same exact candidate has a verified UE 5.8 Win64 compile/cook/package, packaged EXE runtime smoke, Native Chaos/drivetrain/trailer evidence, Farm Cargo runtime PASS, rendered visual acceptance and green relevant Actions.
