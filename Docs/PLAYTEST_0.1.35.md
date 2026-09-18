# GTT 0.1.35 Playtest — Packaged Farm Cargo Emergency Patch Evidence

## Purpose

Validate the emergency-patch branch of the Farm Cargo recovery choice as an end-to-end packaged route. Source CI may validate wiring and invariants, but the Win64 rows remain unverified until the exact commit is compiled, packaged and executed with Unreal Engine 5.8 on the qualifying Windows runner.

## A. Opt-in and sequencing

1. Launch normal gameplay without smoke flags; confirm the 0.1.35 evidence subsystem remains inert.
2. Launch with only `-GTTFarmCargoPatchScenario`; confirm the evidence subsystem remains inert.
3. Launch with patch plus demo flag but without the earlier Farm Cargo evidence flags; confirm it remains inert.
4. Launch with all required deterministic smoke flags; confirm exactly one `FARM_CARGO_PATCH_RUNTIME_BEGIN` marker.
5. Confirm the patch route does not begin before 252 seconds.
6. Confirm the 0.1.33 tow route can finish before the patch route begins.
7. Confirm the patch route reaches a hard deadline at 274 seconds instead of hanging indefinitely.
8. Confirm the full packaged smoke window is at least 276 seconds.
9. Confirm the package launch timeout is greater than the required smoke window.
10. Confirm any `FARM_CARGO_PATCH_RUNTIME phase=DIAGNOSTIC result=FAIL` makes evidence fail.

## B. Baseline and real contract authority

11. Confirm the route refuses to start if Farm Cargo is already active.
12. Confirm wildlife/legal-work blocking is not bypassed by the evidence route.
13. Confirm the temporary evidence baseline records economy, logistics, world time and Wanted state.
14. Confirm the evidence baseline records the native Mulebox migration/body state and transform.
15. Confirm the evidence route moves time only into the real staffed depot window.
16. Confirm evidence cash seeding is temporary and restored after the route.
17. Confirm logistics tier seeding is temporary and restored after the route.
18. Confirm the native Mulebox must report Native readiness.
19. Confirm legacy takeover must be active before the player drives the native Mulebox.
20. Confirm the contract is accepted through the real Start terminal.
21. Confirm the director enters `ReachPickup` rather than being assigned a stage directly.
22. Confirm pickup is performed through the real Pickup terminal.
23. Confirm the director enters `DeliverCargo` through normal contract logic.
24. Confirm `UGTTFarmCargoAuthoritySubsystem` binds the controlled native Mulebox actor.
25. Confirm the bound cargo ID exactly equals the native Mulebox `PersistentVehicleId`.
26. Confirm the route never calls a direct completion/payout shortcut.

## C. Patch-eligible native breakdown

27. Confirm the evidence damage probe uses the native vehicle damage path rather than editing Farm Cargo state.
28. Confirm the post-damage tire integrity reaches the roadside recovery threshold at or below 0.08.
29. Confirm `UGTTBreakdownDecisionSubsystem` reports `TowRecommended` or `Immobilized`.
30. Confirm `bEmergencyPatchPossible` is true for the controlled damage profile.
31. Confirm `CanEmergencyPatch()` agrees that the native Mulebox is patch-eligible.
32. Confirm structural/body damage severe enough to forbid a patch causes the route to fail rather than silently tow.
33. Confirm the cargo timer is sampled before the patch request.
34. Confirm cargo integrity is sampled before the patch request.
35. Confirm cash is sampled before the patch request.
36. Confirm the request goes through `RequestEmergencyRoadsidePatch()`.
37. Confirm one `NATIVE_ROADSIDE_PATCH_REQUESTED` marker carries a positive quote and `player_authorized=YES`.
38. Confirm the patch and tow requests remain mutually exclusive.
39. Confirm active Wanted heat would block the ordinary patch path.

## D. Production checkpoint and paid patch

40. Confirm Farm Cargo detects the pending patch through the production roadside subsystem.
41. Confirm one pre-service primary-save checkpoint is attempted with reason `cargo-roadside-patch-pre-service`.
42. Confirm `PATCH_CHECKPOINT` reports the exact bound vehicle ID.
43. Confirm `PATCH_CHECKPOINT` reports `timer_paused=NO`.
44. Confirm `PATCH_CHECKPOINT` reports `transfer_allowed=NO`.
45. Confirm the patch dispatch waits for the real service delay rather than being completed synchronously by the harness.
46. Confirm the patch charges exactly one positive cash delta.
47. Confirm `NATIVE_ROADSIDE_PATCH_COMPLETE` reports `result=PASS`.
48. Confirm the native patch completion reports `identity_preserved=YES`.
49. Confirm the native patch completion reports `body_preserved=YES`.
50. Confirm the native patch completion still reports `workshop_repair_still_required=YES`.
51. Confirm the exact `PersistentVehicleId` is unchanged after the patch.
52. Confirm cargo authority still points to that exact native Mulebox actor after the patch.
53. Confirm the delivery timer is lower after patch dispatch than before it.
54. Confirm cargo integrity does not improve during the roadside service.
55. Confirm front/rear/left/right body health is unchanged by the patch.
56. Confirm cooling stress is unchanged by the patch.
57. Confirm detached-panel count and detached mask are unchanged by the patch.
58. Confirm condition is at least the 30% limp-home floor, not silently restored to full workshop condition.
59. Confirm tire integrity is at least the 32% limp-home floor, not silently restored to full tire condition.
60. Confirm fuel is not reduced and receives only the emergency floor when required.
61. Confirm production recovery writes the post-service checkpoint `cargo-roadside-patch-post-service`.
62. Confirm `POST_PATCH_VERIFY result=PASS` carries the exact vehicle ID.
63. Confirm post-patch verification reports `saved=YES`, `timer_paused=NO` and `transfer_allowed=NO`.

## E. Anti-transfer and route completion

64. Move the patched exact Mulebox outside Hill Farm handoff range.
65. Park the spawned decoy van inside the Hill Farm handoff zone.
66. Confirm the decoy interaction leaves the director in `DeliverCargo`.
67. Confirm cargo authority still points at the patched native Mulebox after the rejected decoy.
68. Confirm `WRONG_VEHICLE_AFTER_PATCH` reports `wrong_vehicle_rejected=1`.
69. Return the same patched Mulebox to Hill Farm.
70. Confirm the player can re-enter the same native Mulebox after the patch.
71. Confirm Hill Farm accepts the exact vehicle and advances to `DeliverFinalStop`.
72. Confirm the same persistent cargo ID survives the Hill Farm relay.
73. Move the same native Mulebox to North Wood Yard.
74. Confirm North Wood Yard completes the contract exactly once.
75. Confirm the final payout is positive after paying for the patch.
76. Confirm `CargoCompletedRuns` increases by exactly one.
77. Confirm logistics reputation increases exactly through the normal successful-contract authority.
78. Confirm cargo authority is cleared after final completion.
79. Confirm a final explicit primary save succeeds.
80. Confirm a reload of the completed save does not resurrect the finished Farm Cargo route.

## F. Cleanup and evaluator hardening

81. Confirm the temporary decoy actor is destroyed during evidence cleanup.
82. Confirm the original native Mulebox migration state is restored after the evidence route.
83. Confirm the original native Mulebox body damage/detached state is restored after the evidence route.
84. Confirm the original native Mulebox transform is restored after the evidence route.
85. Confirm baseline economy, logistics, time and Wanted state are restored after the evidence route.
86. Confirm the evaluator rejects a missing `BREAKDOWN_PATCH_REQUEST` phase.
87. Confirm the evaluator rejects a fake completion marker without the native patch request marker.
88. Confirm the evaluator rejects a fake completion marker without the native patch completion marker.
89. Confirm the evaluator rejects a missing production pre-patch save checkpoint.
90. Confirm the evaluator rejects a missing production post-patch exact-ID verification.
91. Confirm the evaluator rejects a zero or negative patch cost.
92. Confirm the evaluator rejects a timer that pauses or increases during patch service.
93. Confirm the evaluator rejects cargo integrity that improves during patch service.
94. Confirm the evaluator rejects a post-patch tire value below the limp-home floor.
95. Confirm the evaluator rejects a post-patch condition value below the limp-home floor.
96. Confirm the evaluator rejects a wrong-vehicle probe that succeeds.
97. Confirm the evaluator rejects payout <= 0, completion-run delta other than one, or reputation delta <= 0.
98. Confirm phase values and the final completion marker must agree before `FARM_CARGO_PATCH_RUNTIME.json` can PASS.
99. Confirm evidence SHA must match the exact packaged candidate SHA.
100. Confirm demo technical gate schema 10 requires `gtt.farm-cargo-patch-runtime.v1` in addition to all older evidence manifests.

## G. Documentation, progress and release truth

101. Confirm `Docs/ROADMAP.md` still contains exactly 125 checked and 5 open canonical tasks.
102. Confirm the numeric roadmap dashboard remains 125/130 = 96.2%.
103. Confirm the protected `SWIR-ROADMAP-STANDARD:v1` and `ROADMAP-PROGRESS` markers remain present.
104. Confirm the Roadmap embeds `../assets/readme/progress-mini.svg`.
105. Confirm no legacy character-based progress meter is present in the maintained Roadmap dashboard.
106. Run `python Scripts/generate_progress_svg.py --check`; confirm deterministic math, XML, geometry and SVG-only presentation pass.
107. Confirm README remains `SWIR-README-STANDARD:v2` and retains `## 🔎 Search Keywords`.
108. Confirm README shows milestone 0.1.35 while keeping release readiness separate from 96.2% roadmap completion.
109. Confirm source CI does not mark any of the five Native Chaos/trailer/Win64 blockers complete.
110. Confirm no demo Release is created unless the exact Win64 candidate also passes compile/package, packaged runtime, required evidence manifests and rendered visual acceptance.

## Release acceptance

0.1.35 is a source/evidence-contract milestone until a qualifying Windows x64 runner with Unreal Engine 5.8 compiles and packages the exact commit and the packaged executable emits a PASS `FARM_CARGO_PATCH_RUNTIME.json`. Even a successful technical manifest does not authorize a public demo by itself; the exact candidate must also satisfy the project’s rendered visual acceptance gate.