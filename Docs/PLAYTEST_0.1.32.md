# GTT 0.1.32 — Farm Cargo Breakdown & Tow Recovery Playtest

Milestone goal: prove that an active Farm Cargo load remains attached to the same physical/persistent vehicle through native breakdown, voluntary roadside tow and police impound consequences. Recovery must not pause the delivery clock, transfer cargo, duplicate stock, grant duplicate payout/reputation, repair damage for free, or bypass the existing roadside-recovery economy authority.

> Source CI verifies the wiring/invariants below. Items requiring Unreal/Win64 runtime remain manual acceptance until a packaged UE 5.8 candidate actually produces evidence.

## A. Baseline exact-vehicle authority

- [ ] 01. Accept Farm Cargo, pick up with the native Mulebox and record its `PersistentVehicleId`.
- [ ] 02. Park a second same-model vehicle at Hill Farm; it cannot complete the handoff.
- [ ] 03. Original cargo vehicle can complete the same handoff when present/stopped.
- [ ] 04. Destroy/recreate a same-ID actor and verify authority rebinds only that ID.
- [ ] 05. Remove the exact actor while another same-model actor is nearby; handoff remains blocked.
- [ ] 06. Restore the exact actor; authority rebinds without a new depot pickup.

## B. Degraded vehicle consequences

- [ ] 07. Damage the cargo vehicle enough for `LimpToWorkshop`; recovery state becomes DEGRADED.
- [ ] 08. Cargo timer keeps decreasing while the vehicle is degraded.
- [ ] 09. Existing cargo-integrity damage penalty still comes from `AGTTFarmJobDirector`.
- [ ] 10. New recovery policy does not directly mutate cargo integrity.
- [ ] 11. Recovery policy does not pay, refund or alter logistics reputation.
- [ ] 12. Repair/tow estimates still come from `UGTTBreakdownDecisionSubsystem`.

## C. Player-authorized roadside tow

- [ ] 13. Disable the native cargo Mulebox at Wanted 0 and request tow with T / D-pad Up.
- [ ] 14. Tow request still checks existing eligibility and player cash.
- [ ] 15. Tow cost is charged exactly once by `UGTTRoadsideRecoverySubsystem`.
- [ ] 16. Entering TOW PENDING writes a primary-save checkpoint before vehicle movement.
- [ ] 17. Pre-tow checkpoint contains the active cargo route and exact bound vehicle ID.
- [ ] 18. Delivery timer continues while tow dispatch is pending.
- [ ] 19. Tow moves the existing vehicle actor to the workshop; cargo policy performs no teleport itself.
- [ ] 20. Native mechanical/body damage remains preserved by ordinary roadside tow.
- [ ] 21. After tow, cargo authority still reports the same `PersistentVehicleId`.
- [ ] 22. Post-tow primary-save checkpoint succeeds after identity verification.
- [ ] 23. A second same-model vehicle at the workshop does not inherit the load.
- [ ] 24. Hill Farm accepts the original recovered vehicle when stopped in the yard.

## D. Police impound

- [ ] 25. With Wanted >=2 and a severely disabled cargo vehicle, policy enters POLICE IMPOUND PENDING.
- [ ] 26. A primary-save checkpoint is attempted before automatic impound movement.
- [ ] 27. Existing impound fee/fine remains owned by roadside recovery/economy code.
- [ ] 28. Existing mandatory safety service remains owned by police impound logic.
- [ ] 29. Impound does not create a new cargo binding.
- [ ] 30. After impound, exact persistent identity is re-verified before recovery state becomes RECOVERED.
- [ ] 31. Delivery timer was not paused by the cargo-recovery policy.
- [ ] 32. Wrong same-model vehicle still fails the next buyer handoff after impound.

## E. Save/load around recovery

- [ ] 33. Save while DEGRADED, reload and keep the same active route/vehicle identity.
- [ ] 34. Save immediately after tow request checkpoint, reload and do not reserve stock again.
- [ ] 35. Reload after completed tow and resolve only the saved exact vehicle ID.
- [ ] 36. Reload with exact actor absent; state becomes AWAITING EXACT VEHICLE and delivery stays blocked.
- [ ] 37. Spawn/recall the correct exact-ID actor after reload; authority recovers it.
- [ ] 38. Spawn a wrong same-model actor first; it never receives the load.
- [ ] 39. Hill Farm relay save/load still preserves timer, integrity and stock reservation.
- [ ] 40. North Wood Yard completion after recovery records one completion only.

## F. Economy, idempotence and failure paths

- [ ] 41. Insufficient tow cash leaves the vehicle/cargo in place and grants no free recovery.
- [ ] 42. Wanted 1 blocks voluntary tow as before; no duplicate charge is attempted.
- [ ] 43. Repeated T presses cannot create duplicate pending tow charges.
- [ ] 44. Repeated cargo-policy ticks in one state do not spam save checkpoints.
- [ ] 45. Missing exact vehicle never causes nearest-vehicle fallback.
- [ ] 46. Failed post-recovery identity verification keeps delivery locked.
- [ ] 47. Contract timeout during recovery fails through the existing Farm Job director path.
- [ ] 48. Failed/expired contract clears cargo authority normally and recovery state returns NONE.

## G. Vertical-slice regression

- [ ] 49. Tier 1 Feed Depot → Hill Farm still completes normally with no breakdown.
- [ ] 50. Tier 2 Hill Farm relay → North Wood Yard still completes normally with no breakdown.
- [ ] 51. Tier 3 heavier cargo keeps its existing load factor after exact-ID rebind.
- [ ] 52. Traffic, ranger road-stop and police systems still operate during an active cargo route.
- [ ] 53. Day/night transition does not reset recovery state or cargo authority.
- [ ] 54. Garage/vehicle identity refresh does not rename an already persisted cargo ID.
- [ ] 55. Farm Cargo route beacon still follows the authoritative contract stage after recovery.
- [ ] 56. Completion reload does not resurrect the contract, reserve stock again or pay twice.

## H. Packaged candidate gate (future Win64 evidence)

- [ ] 57. UE 5.8 Development Win64 compile succeeds for the exact candidate SHA.
- [ ] 58. Cook/package succeeds and produces the expected Windows executable/artifact.
- [ ] 59. Packaged EXE runtime smoke survives the required evidence window.
- [ ] 60. Existing `FARM_CARGO_RUNTIME.json` passes.
- [ ] 61. Existing `FARM_CARGO_RECOVERY_RUNTIME.json` passes.
- [ ] 62. Runtime log contains pre-tow checkpoint, exact-ID recovery and wrong-vehicle rejection evidence for a cargo breakdown/tow path.
- [ ] 63. Native Chaos drivetrain/wheels and authored-trailer acceptance gates pass on the same candidate.
- [ ] 64. Rendered visual acceptance confirms the vertical slice is presentable; only then may demo-release readiness be reconsidered.

## Acceptance notes

The 0.1.32 source milestone is accepted when its dedicated verifier and all inherited cargo recovery verifiers pass. This document deliberately does **not** mark Win64/runtime items complete from source CI. A public demo remains blocked until the exact packaged candidate satisfies the independent build, runtime, Native Chaos, trailer and visual gates defined by the main roadmap.
