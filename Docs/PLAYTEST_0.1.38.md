# GTT 0.1.38 Playtest — Roadside Dispatch Contract + Farm Cargo Continuity

Target: source contract now; packaged Win64 execution only when a qualifying UE 5.8 runner is available. Do not mark runtime-only roadmap gates complete from source CI.

## A. Activation and isolation

1. Launch normal play without evidence flags; confirm the 0.1.38 evidence subsystem stays inert.
2. Launch with only `-GTTDemoSmokeScenario`; confirm the dispatch evidence route does not start.
3. Launch with the previous Farm Cargo flags but without `-GTTFarmCargoDispatchScenario`; confirm the new route does not start.
4. Launch with all required flags; confirm exactly one `FARM_CARGO_DISPATCH_RUNTIME_BEGIN` marker appears.
5. Confirm the new route starts after the earlier Farm Cargo breakdown route has restored its baseline.
6. Confirm the route refuses to begin if the Farm Job Director is not idle.
7. Confirm the route refuses to begin while wildlife/legal-work state is dirty.
8. Confirm the route requires the native Mulebox to be ready for legacy takeover.
9. Confirm the route requires Start, Pickup, Hill and Final terminals.
10. Confirm the route restores baseline state after PASS.
11. Confirm the route restores baseline state after a deliberate FAIL.
12. Confirm the route has a hard global deadline and emits a diagnostic failure on timeout.

## B. Contract acceptance and physical cargo authority

13. Seed a clean logistics state and verify a tier-2-or-better stock-backed cargo order can be accepted.
14. Accept through the real Start terminal; stage becomes `ReachPickup`.
15. Enter the real native Mulebox through the normal takeover path.
16. Interact with the real Pickup terminal; stage becomes `DeliverCargo`.
17. Confirm cargo authority binds the exact native Mulebox actor.
18. Confirm bound `PersistentVehicleId` equals the Mulebox persistent ID.
19. Confirm no direct `CompleteCargoContract` call exists in the evidence harness.
20. Confirm no direct cash award exists in the evidence harness.
21. Confirm no private final-stop bypass exists in the evidence harness.
22. Confirm cargo timer is already running before roadside dispatch tests begin.
23. Confirm cargo integrity is captured before roadside dispatch tests begin.
24. Confirm active cargo remains attached to the exact loaded vehicle during all dispatch phases.

## C. Emergency patch request-time contract

25. Stage a patch-eligible `TowRecommended` condition without destroying body-state authority.
26. Request the production emergency patch on the loaded native Mulebox.
27. Confirm pending mode is `EmergencyPatch`.
28. Confirm the request-time patch quote is positive.
29. Confirm the pending target ID equals the cargo `PersistentVehicleId`.
30. Confirm the player is not charged when dispatch is merely accepted.
31. Confirm initial ETA is positive.
32. Mutate vehicle condition/tire state after acceptance while service is pending.
33. Confirm `GetPendingRecoveryQuote` still returns the original locked patch quote.
34. Confirm `GetPendingRecoveryVehicleId` remains the exact original target.
35. Wait less than service-arrival time and confirm the patch remains pending.
36. Confirm live patch ETA decreases from its initial value.
37. Confirm cash remains unchanged while the patch is pending.
38. Confirm HUD-facing API can read mode, quote, ETA and target ID from the same pending contract.

## D. Patch cancellation semantics

39. Cancel the pending patch through `CancelPendingRoadsideService`.
40. Confirm the pending patch flag clears immediately.
41. Confirm pending quote returns to zero after cancellation.
42. Confirm pending target ID returns to `None` after cancellation.
43. Confirm cash delta for the cancelled patch is exactly zero.
44. Confirm `NATIVE_ROADSIDE_DISPATCH_CANCELLED` records `mode=PATCH` and `charged=NO`.
45. Confirm cancellation does not clear Farm Cargo authority.
46. Confirm cancellation does not reset the Farm Cargo timer.

## E. Tow contract and cancellation

47. Request the production roadside tow on the same damaged cargo Mulebox.
48. Confirm pending mode is `RoadsideAssistance`.
49. Confirm the request-time tow quote is positive.
50. Confirm the pending tow target ID equals the cargo vehicle ID.
51. Confirm no tow charge is taken on acceptance.
52. Wait less than tow arrival time and confirm live tow ETA decreases.
53. Confirm the pending tow quote remains locked during the observation window.
54. Cancel the pending tow before arrival.
55. Confirm the tow pending flag clears.
56. Confirm tow cancellation cash delta is exactly zero.
57. Confirm `NATIVE_ROADSIDE_DISPATCH_CANCELLED` records `mode=TOW` and `charged=NO`.
58. Confirm exact cargo vehicle authority survives the cancelled tow.

## F. Re-request, completion and economy

59. Restore a production-valid patch-eligible breakdown state on the same Mulebox.
60. Re-request emergency patch after both prior cancellations.
61. Confirm a new positive locked patch quote is recorded.
62. Confirm no charge occurs before the re-requested service arrives.
63. Allow the real patch dispatch timer to complete.
64. Confirm the cash deduction equals the re-requested locked patch quote exactly.
65. Confirm the patch completes on the same `PersistentVehicleId`.
66. Confirm the Farm Cargo timer continued to decrease across the dispatch sequence.
67. Confirm cargo integrity did not improve as a side effect of roadside service.
68. Confirm the real native patch-complete marker reports locked quote and identity preservation.

## G. Delivery, persistence and release gates

69. Put a decoy van at Hill Farm while the exact Mulebox is far away; confirm handoff is rejected.
70. Bring the exact Mulebox to Hill Farm, then North Wood Yard; confirm one payout, one completed cargo run, reputation gain and authority cleanup.
71. Save through the real primary save path after completion and confirm the persistence marker passes.
72. On a qualifying Win64 UE 5.8 runner, require `FARM_CARGO_DISPATCH_RUNTIME.json` PASS from the exact candidate before accepting the technical bundle; source CI alone must leave Demo Release NOT READY.
