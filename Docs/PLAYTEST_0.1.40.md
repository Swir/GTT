# GTT 0.1.40 Playtest — Packaged Roadside Dispatch Persistence + Save/Load Evidence

> Source milestone now; packaged cases become evidence only after a qualifying UE 5.8 Win64 runner executes the exact candidate. Do not infer runtime PASS from source CI.

## A. Activation and isolation
1. Launch normal play without smoke flags and verify the 0.1.40 evidence route stays inert.
2. Launch with `-GTTDemoSmokeScenario` only and verify the route stays inert.
3. Launch with all earlier Farm Cargo flags but without `-GTTFarmCargoDispatchPersistenceScenario` and verify the route stays inert.
4. Launch with the complete flag set and verify exactly one `FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME_BEGIN` marker.
5. Verify the route starts after the 0.1.38 dispatch route has finished its window.
6. Verify the route refuses to start if required Farm Cargo terminals are unavailable.
7. Verify the route requires a native Mulebox that is ready for takeover.
8. Verify the route has a hard global deadline and emits a diagnostic FAIL on timeout.

## B. Farm Cargo contract setup
9. Begin from an idle Farm Job Director and clean wildlife/legal-work state.
10. Seed a Tier-2-or-better stock-backed cargo order through the existing logistics authority.
11. Accept the contract through the real Start terminal.
12. Enter the real native Mulebox through the normal takeover path.
13. Pick up cargo through the real Pickup terminal.
14. Verify cargo binds the exact native Mulebox actor.
15. Verify cargo-bound `PersistentVehicleId` equals the native Mulebox persistent ID.
16. Capture cargo timer and integrity before persistence tests begin.

## C. Tow checkpoint creation
17. Stage the Mulebox into a real `TowRecommended` breakdown state.
18. Request the production roadside tow.
19. Verify the tow quote is positive and locked at request time.
20. Verify the pending target is the exact cargo `PersistentVehicleId`.
21. Verify no cash is charged when the tow is requested.
22. Wait for the production persistence subsystem to write `GTT_RoadsideDispatch_01`.
23. Verify sidecar schema is 2.
24. Verify sidecar mode is RoadsideAssistance.
25. Verify sidecar locked quote equals the accepted tow quote.
26. Verify sidecar ETA is positive and less than or equal to the original dispatch window.
27. Verify sidecar `PersistentVehicleId` equals the cargo-bound vehicle.
28. Verify sidecar authorization cash/revision fields are populated.

## D. Tow SaveGame reload and restore
29. Save the primary world while the tow sidecar is in flight.
30. Cancel only the in-memory tow before simulating the reload.
31. Load the primary world through `GameMode->LoadProgress()`.
32. Invoke the guarded evidence re-arm and verify the evidence flag is required.
33. Verify the re-arm reads the existing sidecar rather than writing a replacement.
34. Verify the production restore path rebuilds a pending TOW.
35. Verify the restored tow keeps the original locked quote.
36. Verify restored ETA does not reset to a fresh full dispatch duration.
37. Verify the restored target remains the exact cargo `PersistentVehicleId`.
38. Verify Farm Cargo still binds the same native Mulebox.
39. Verify cash is unchanged before restored tow arrival.
40. Cancel the restored tow and verify no charge is taken.

## E. Wanted fail-closed restore
41. Request a fresh production tow while the same cargo contract is active.
42. Wait for a fresh sidecar checkpoint and verify the locked quote is positive.
43. Save the primary world while that tow is pending.
44. Cancel only the in-memory request.
45. Reload the primary world.
46. Add Wanted heat after the reload and verify Wanted level becomes positive.
47. Re-arm the sidecar loader through the guarded evidence hook.
48. Verify the production restore rejects the voluntary tow because Wanted is active.
49. Verify no roadside service remains pending after rejection.
50. Verify the sidecar is deleted after rejection.
51. Verify cash is unchanged by the rejected restore.
52. Verify the exact Farm Cargo vehicle remains authoritative.
53. Clear Wanted before the next voluntary roadside request.

## F. Emergency patch checkpoint and restore
54. Re-stage a patch-eligible `TowRecommended` condition on the same Mulebox.
55. Request the production emergency patch.
56. Verify a positive locked patch quote and exact target ID.
57. Verify no cash is charged at patch request time.
58. Wait for a patch sidecar checkpoint.
59. Verify sidecar mode is EmergencyPatch and quote/ID match the live contract.
60. Save the primary world while patch dispatch is in flight.
61. Cancel only the in-memory patch.
62. Reload the primary world.
63. Re-arm the sidecar loader through the guarded evidence hook.
64. Verify production restore rebuilds a pending PATCH.
65. Verify restored locked quote equals the pre-reload quote.
66. Verify restored ETA is positive and does not reset beyond the saved ETA tolerance.
67. Verify restored target remains the exact cargo `PersistentVehicleId`.
68. Verify no cash is charged before the restored patch arrives.
69. Let the restored patch complete through the normal production completion path.
70. Verify the cash debit occurs exactly once.
71. Verify the debit equals the locked patch quote exactly.
72. Verify cargo timer continued to run across dispatch and reload.
73. Verify cargo integrity did not improve because of roadside recovery.
74. Verify cargo remains bound to the same native Mulebox after patch completion.

## G. Delivery authority after persistence
75. Place a decoy van at Hill Farm while the loaded Mulebox is far away.
76. Attempt Hill Farm handoff and verify the decoy is rejected.
77. Place the exact loaded Mulebox at Hill Farm and complete the relay.
78. Verify stage advances to the final stop with the same vehicle authority.
79. Deliver the exact Mulebox at North Wood Yard.
80. Verify contract returns to Idle and cargo authority clears.
81. Verify payout delta is positive.
82. Verify exactly one cargo completion is recorded.
83. Verify logistics reputation increases exactly through the real completion path.
84. Save the primary world after completion and verify the save succeeds.

## H. Evidence evaluator and release gate
85. Run `evaluate_farm_cargo_dispatch_persistence_runtime.ps1` and require schema `gtt.farm-cargo-dispatch-persistence-runtime.v1` PASS.
86. Verify the manifest carries the exact packaged build SHA.
87. Verify the manifest reports at least three guarded sidecar reload markers, two successful restore markers and one production Wanted rejection marker.
88. Verify `DEMO_TECHNICAL_GATE.json` schema 12 refuses to pass if the persistence manifest is missing.
89. Verify schema 12 refuses a mismatched manifest SHA.
90. Verify schema 12 refuses any missing tow/patch restore, no-charge, exact-ID or single-charge gate.
91. Verify the Win64 workflow retains the persistence manifest in successful candidate artifacts.
92. Verify failure diagnostics also retain the persistence manifest when it exists.
93. Verify the final ZIP hash step hashes the ZIP path directly and emits a SHA256 file.

## I. Regression and presentation honesty
94. Run the 0.1.39 persistence source verifier and require PASS.
95. Run the 0.1.40 source verifier and require PASS.
96. Run deterministic SWIR progress SVG generation/check and require PASS.
97. Verify Roadmap remains exactly 125 completed of 130 total, 96.2%.
98. Verify README contains exactly one `progress-card.svg` and no `progress-mini.svg`.
99. Verify Roadmap contains exactly one `progress-mini.svg` and no ASCII/Unicode progress meter.
100. Verify `progress-template.svg` remains TEMPLATE/N/A and is not embedded as project data.
101. Verify source CI does not claim Unreal compilation or packaged runtime PASS.
102. Verify the new evidence re-arm contains no economy mutation and no direct restore bypass.
103. Verify the evidence harness contains no direct cargo completion or cash-award shortcut.
104. Do not create a Demo Release unless Win64 compile/package, packaged runtime, Native Chaos/trailer and rendered visual gates all pass on the exact candidate.

## Required evidence boundary
- Source verifier PASS proves repository wiring and invariants only.
- The 0.1.40 packaged route proves a production SaveGame sidecar roundtrip only when executed inside the packaged EXE on the qualifying Windows runner.
- The guarded re-arm simulates the sidecar reader restart inside the same packaged process; it is not a claim of OS-process restart or cold-boot recovery.
- Demo readiness still requires the exact-candidate technical and visual gates in addition to this persistence evidence.
