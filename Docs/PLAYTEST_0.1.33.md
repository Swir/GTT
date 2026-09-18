# GTT 0.1.33 — Packaged Farm Cargo Breakdown Recovery Evidence

This matrix covers the deterministic packaged evidence route. Source sanity can validate wiring only; Win64 rows remain unverified until the exact commit runs on the UE 5.8 Windows runner.

1. Launch without `-GTTFarmCargoBreakdownScenario`; confirm the 0.1.33 harness stays inert.
2. Launch with only the breakdown flag; confirm it still stays inert without `-GTTDemoSmokeScenario`.
3. Launch with all required smoke flags and confirm one `FARM_CARGO_BREAKDOWN_RUNTIME_BEGIN` marker.
4. Confirm the route waits until 228 seconds so earlier drivetrain/trailer/Farm Cargo recovery evidence is undisturbed.
5. Confirm the scenario fails instead of overwriting an already-active Farm Cargo contract.
6. Confirm wildlife alert blocks evidence setup rather than bypassing legal-work rules.
7. Confirm Wanted heat is cleared only inside the isolated evidence baseline.
8. Confirm world time moves to the staffed depot window and is restored afterwards.
9. Confirm evidence-only cash seeding is restored afterwards.
10. Confirm evidence-only logistics seeding is restored afterwards.
11. Confirm the native Mulebox is required and a legacy-only cargo vehicle is not accepted for this route.
12. Confirm Native Chaos readiness is required before the route proceeds.
13. Confirm legacy takeover is active before the player enters the native Mulebox.
14. Confirm the contract is accepted through the real Start terminal.
15. Confirm depot stock is reserved through the real logistics authority.
16. Confirm pickup is performed through the real Pickup terminal.
17. Confirm cargo authority binds the controlled native Mulebox actor.
18. Confirm the bound ID equals the native Mulebox `PersistentVehicleId`.
19. Confirm the contract enters `DeliverCargo` with one authoritative timer.
20. Confirm cargo load factor is applied by the existing Farm Job Director.
21. Confirm the breakdown probe uses native tire/condition damage rather than editing the Farm Cargo timer.
22. Confirm post-damage tire integrity is at or below the roadside-recovery threshold.
23. Confirm the real Breakdown Decision subsystem reports tow recommendation/immobilization.
24. Confirm the tow request uses `RequestRoadsideTow` rather than direct teleportation.
25. Confirm a player-authorized `NATIVE_ROADSIDE_TOW_REQUESTED` marker is emitted.
26. Confirm a pre-move Farm Cargo primary-save checkpoint is written.
27. Confirm the cargo timer continues while tow dispatch is pending.
28. Confirm cargo integrity never improves during the tow.
29. Confirm roadside tow charges a positive amount exactly once.
30. Confirm ordinary roadside tow does not repair the disabled tires.
31. Confirm body/condition damage is preserved by roadside assistance.
32. Confirm the native vehicle moves to the workshop through the real recovery service.
33. Confirm the exact bound cargo vehicle actor remains the same after recovery.
34. Confirm the exact `PersistentVehicleId` remains unchanged after recovery.
35. Confirm production Farm Cargo recovery emits `POST_RECOVERY_VERIFY result=PASS`.
36. Confirm production recovery writes its post-move primary-save checkpoint.
37. Confirm another vehicle cannot inherit the load after the tow.
38. Park the decoy at Hill Farm while the native Mulebox is outside the handoff zone; confirm rejection.
39. Confirm the wrong-vehicle probe leaves the director in `DeliverCargo`.
40. Confirm the cargo authority still points at the native Mulebox after the rejected probe.
41. Return/re-enter the same native Mulebox and place it in the Hill Farm zone.
42. Confirm Hill Farm accepts the exact vehicle and moves to `DeliverFinalStop`.
43. Confirm the same cargo vehicle ID survives the Hill Farm relay.
44. Move the same native Mulebox to North Wood Yard.
45. Confirm the final handoff completes the route exactly once.
46. Confirm payout after the paid tow is positive.
47. Confirm `CargoCompletedRuns` increases by exactly one.
48. Confirm logistics reputation increases after successful completion.
49. Confirm cargo authority is cleared after final completion.
50. Confirm a final explicit `SaveProgress()` succeeds.
51. Confirm baseline economy/logistics/time/Wanted state is restored after the proof.
52. Confirm baseline native migration/body state is restored after the proof.
53. Confirm the decoy actor is destroyed during evidence cleanup.
54. Confirm no `FARM_CARGO_BREAKDOWN_RUNTIME phase=DIAGNOSTIC result=FAIL` exists in a passing log.
55. Confirm the completion marker repeats every mandatory gate and agrees with phase evidence.
56. Confirm `evaluate_farm_cargo_breakdown_runtime.ps1` rejects a missing BREAKDOWN_TOW_REQUEST phase.
57. Confirm the evaluator rejects a fake completion marker without native tow request/complete production markers.
58. Confirm the evaluator rejects zero/negative tow cost.
59. Confirm the evaluator rejects a timer that paused or increased during recovery.
60. Confirm the evaluator rejects cargo integrity that improves during tow.
61. Confirm the evaluator rejects tires repaired by an ordinary roadside tow.
62. Confirm the evaluator rejects wrong-vehicle acceptance after tow.
63. Confirm the evaluator rejects missing pre-tow primary-save checkpoint evidence.
64. Confirm the evaluator rejects missing post-tow exact-ID recovery evidence.
65. Confirm the evaluator rejects payout <= 0, completion delta != 1, or reputation delta <= 0.
66. Confirm `FARM_CARGO_BREAKDOWN_RUNTIME.json` uses schema `gtt.farm-cargo-breakdown-runtime.v1`.
67. Confirm `RUNTIME_SMOKE.json` records `-GTTFarmCargoBreakdownScenario`.
68. Confirm packaged smoke stays alive for at least 250 seconds with a 275-second launch timeout.
69. Confirm the Win64 workflow evaluates the new JSON before the demo technical gate.
70. Confirm the demo technical gate refuses a candidate when breakdown evidence is missing or failed.
71. Confirm the technical candidate bundle uploads the new evidence JSON when available.
72. Confirm failure diagnostics also retain the new evidence JSON and runtime log.
73. Confirm source sanity preserves 0.1.32 production recovery checks.
74. Confirm source sanity preserves 0.1.31 save/load cargo evidence checks.
75. Confirm SWIR Progress SVG remains mathematically tied to the authoritative roadmap.
76. Confirm roadmap remains 125/130 = 96.2%; this source milestone closes no Win64/Native Chaos/trailer acceptance checkbox.
77. Confirm no GitHub Release is created from source-only evidence.
78. On a qualifying UE 5.8 Windows runner, compile the exact candidate SHA before accepting any runtime result.
79. On that runner, package Win64 and verify the packaged EXE remains alive through the full 250-second route.
80. Only after real JSON PASS plus visual acceptance may 0.1.33 evidence contribute to demo readiness.
