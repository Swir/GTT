# GTT 0.1.31 Playtest — Fleet Identity & Packaged Farm Cargo Recovery

Milestone focus: prove that a loaded Farm Cargo route survives two real primary-save reload points without changing physical vehicle identity, duplicating stock, duplicating payout, or allowing a second same-model vehicle to inherit the load.

> Source/contract CI can verify wiring and invariants. Items marked **Win64 runtime** remain unproven until an Unreal Engine 5.8 packaged executable emits the required evidence on the exact candidate SHA.

## A. Same-model fleet identity

1. Spawn one legacy Mulebox and confirm it keeps its canonical persistent ID when no duplicate exists.
2. Spawn a second unowned Mulebox and confirm `UGTTVehicleIdentitySubsystem` assigns a deterministic actor-suffixed ID.
3. Confirm both physical vans expose different non-empty persistent IDs after refresh.
4. Spawn a third same-model van later and confirm existing observed IDs do not change.
5. Confirm a later actor whose name sorts before an existing actor cannot steal an already-observed ID.
6. Mark a vehicle owned and confirm background identity refresh never renames that persisted ID.
7. Create an artificial already-owned duplicate collision and confirm the subsystem logs an unresolved collision instead of silently mutating saved identity.
8. Confirm a missing/None persistent ID is logged/skipped rather than fabricated into a seemingly valid owned identity.
9. Confirm normal different-model vehicles retain their existing canonical IDs.
10. Confirm identity refresh does not change fuel, damage, tuning, transform, ownership, Wanted, economy or cargo state.

## B. Recovery route preparation

11. Start from an idle Farm Cargo director with no wildlife alert.
12. Confirm the evidence route uses the existing Tier-2 stock-backed order path rather than a synthetic contract.
13. Confirm the scenario creates a primary Mulebox and a decoy Mulebox with distinct IDs.
14. Confirm the primary and decoy remain ordinary legacy farm vans; no test-only payout/economy authority is created.
15. Confirm evidence baseline captures logistics, cash/fish state, time and Wanted heat for cleanup.
16. Confirm the evidence harness is inert without all required command-line flags.
17. Confirm the evidence harness starts only after the existing 0.1.29 Farm Cargo scenario has completed.

## C. Loaded checkpoint

18. Accept the contract through the real Start terminal.
19. Pick up cargo through the real Feed Depot terminal.
20. Confirm `UGTTFarmCargoAuthoritySubsystem` binds the primary physical van and stores its unique persistent ID.
21. Record route timer, cargo integrity and depot stock immediately before the loaded checkpoint.
22. Save through the real `AGTTGameMode::SaveProgress()` primary slot.
23. Confirm the saved route stage is `DeliverCargo`.
24. Confirm the saved physical vehicle ID is the primary van ID, not the decoy ID.
25. Destroy the original loaded van after the checkpoint.
26. Spawn a replacement actor carrying the exact saved ID to model garage/actor recreation.
27. Clear in-memory route/authority state without touching the saved checkpoint.
28. Load through the real `AGTTGameMode::LoadProgress()` path.
29. Confirm route stage restores to `DeliverCargo`.
30. Confirm authority rebinds the recreated actor by exact persistent ID.
31. Confirm the original loaded actor pointer is not required for recovery.
32. Confirm timer restores within the documented tolerance.
33. Confirm cargo integrity restores within the documented tolerance.
34. Confirm depot stock is unchanged by save/load and is not reserved a second time.

## D. Wrong vehicle after reload

35. Move the recovered cargo van outside the Hill Farm handoff zone.
36. Park the decoy same-model van inside the Hill Farm handoff zone.
37. Attempt the real Hill Farm terminal interaction.
38. Confirm the route remains `DeliverCargo`.
39. Confirm authority still points at the recovered exact-ID cargo van.
40. Confirm the decoy receives no cargo authority merely because it is nearer.
41. Confirm no payout, reputation gain or completion-history entry is created by the rejected handoff.

## E. Relay checkpoint

42. Put the recovered exact cargo van inside the Hill Farm handoff zone and stop it.
43. Complete the real Hill Farm handoff.
44. Confirm the route advances to `DeliverFinalStop` while retaining the same vehicle ID.
45. Record relay timer, cargo integrity and stock.
46. Save the relay checkpoint through the real primary SaveGame path.
47. Clear in-memory route/authority state again.
48. Reload the primary save.
49. Confirm route stage restores to `DeliverFinalStop`.
50. Confirm the same recovered actor is rebound by the same persistent ID.
51. Confirm timer/integrity are restored and stock remains unchanged.
52. Confirm reload does not replay Hill Farm or create an extra reservation.

## F. Final handoff and idempotence

53. Complete North Wood Yard using the recovered exact cargo vehicle.
54. Confirm the director becomes idle and physical cargo authority clears.
55. Confirm cash increases through the normal FarmJobDirector payout path.
56. Confirm cargo completion history increases by exactly one.
57. Confirm logistics reputation increases.
58. Reload the just-completed primary save.
59. Confirm the completed route stays idle after reload and does not resurrect.
60. Confirm cash, completion count and reputation do not increment a second time on completion reload.
61. Confirm cargo authority remains empty after completion reload.
62. Perform one final explicit `SaveProgress()` and verify success.

## G. Packaged Win64 evidence gate

63. **Win64 runtime:** package the exact candidate with Unreal Engine 5.8.
64. **Win64 runtime:** launch packaged `GTT.exe` with `-GTTDemoSmokeScenario -GTTFarmCargoRuntimeScenario -GTTFarmCargoRecoveryScenario`.
65. **Win64 runtime:** keep the deterministic evidence process alive for at least 228 seconds.
66. **Win64 runtime:** confirm `FARM_CARGO_RECOVERY_RUNTIME_BEGIN version=1` appears after the base Farm Cargo route.
67. **Win64 runtime:** confirm every required recovery phase emits `result=PASS`.
68. **Win64 runtime:** confirm no `FARM_CARGO_RECOVERY_RUNTIME phase=DIAGNOSTIC result=FAIL` lines exist.
69. **Win64 runtime:** run `Scripts/evaluate_farm_cargo_recovery_runtime.ps1` against the exact runtime log.
70. **Win64 runtime:** confirm `FARM_CARGO_RECOVERY_RUNTIME.json` uses schema `gtt.farm-cargo-recovery-runtime.v1` and exact build SHA.
71. **Win64 runtime:** confirm the manifest proves actor recreation, loaded-stage rebind, relay rebind, wrong-vehicle rejection after reload, stable stock, timer/integrity restoration and completion reload idempotence.
72. **Win64 runtime:** confirm the demo technical gate refuses the candidate if the recovery manifest is missing or FAIL.

## H. Regression / presentation / release safety

73. Run 0.1.29 Farm Cargo runtime verifier and confirm the original exact-vehicle route remains mandatory.
74. Run 0.1.30 Farm Cargo recovery verifier and confirm ordinary player save/load contracts remain intact.
75. Run SWIR progress generator in `--check` mode.
76. Confirm `Docs/ROADMAP.md` remains exactly 125/130 = 96.2% unless real Native Chaos/Win64/trailer evidence closes a checkbox.
77. Confirm the protected SWIR Roadmap v1 dashboard marker, badges, table and 20-segment bar remain unchanged.
78. Confirm README remains SWIR README PRO v2 with Search Keywords and truthful no-public-demo wording.
79. Confirm Progress SVG continues to separate roadmap completion from release readiness.
80. Do **not** create a Demo Release from source sanity alone; require the full Win64/package/runtime/visual gate on the exact candidate.

## Expected evidence

A qualifying packaged candidate must add `FARM_CARGO_RECOVERY_RUNTIME.json` beside the existing runtime manifests. Passing source CI only proves that the recovery route and evaluator are wired; it does **not** claim that Unreal Engine compiled the new C++ or that the packaged scenario has actually run.
