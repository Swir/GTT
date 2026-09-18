# GTT 0.1.36 — Roadside Dispatch Contract Playtest

Status: **source contract implemented; packaged Win64 runtime NOT YET VERIFIED**.

This milestone hardens voluntary emergency patch/tow into a player-authorized dispatch contract: request-time quote lock, exact `PersistentVehicleId` pinning, cancellation before arrival, cross-service conflict protection, and a separate non-cancellable police impound path.

## Core dispatch acceptance

1. Damage a native road vehicle until Breakdown Decision returns `TowRecommended` while speed is <= 3.5 km/h.
2. Press `T`: tow dispatch starts once; record the displayed locked quote.
3. Verify `GetPendingRecoveryMode()` reports `RoadsideAssistance`.
4. Verify `GetPendingRecoveryQuote()` equals the displayed request-time quote.
5. Verify `GetPendingRecoveryVehicleId()` equals the exact vehicle `PersistentVehicleId`.
6. Verify remaining dispatch seconds decrease toward zero.
7. Change damage/fuel state after dispatch without changing vehicle identity; the accepted tow quote must not be recomputed.
8. Allow tow arrival: exactly the locked quote is charged.
9. Verify the same physical vehicle arrives at the workshop.
10. Verify tow preserves damage rather than silently applying workshop repair.

## Cancellation and service switching

11. Request tow and press `T` again before arrival: dispatch cancels.
12. Verify cancellation charges $0 and does not move the vehicle.
13. Re-request tow after cancellation: a clean new dispatch state is created.
14. Request emergency patch and press `Y` again before arrival: patch cancels.
15. Verify patch cancellation charges $0 and changes no vehicle state.
16. While patch is inbound press `T`: tow must not silently replace patch.
17. While tow is inbound press `Y`: patch must not silently replace tow.
18. Cancel the active service with its own key before selecting the other service.
19. Verify cancelled quote, target ID and timer are cleared.
20. Verify cooldown blocks immediate service spam only after successful completion, not after cancellation.

## Emergency patch contract

21. Create a patch-eligible `TowRecommended` breakdown and press `Y`.
22. Record locked patch quote and exact pinned vehicle ID.
23. Verify body/panel damage is unchanged while service is inbound.
24. Let patch arrive; exactly the locked quote is charged.
25. Verify minimum limp-home condition/tire/fuel floors are applied.
26. Verify detached panels and body damage remain unchanged.
27. Verify full workshop repair is still required/recommended.
28. If cash is spent elsewhere after request so the locked quote is unaffordable at arrival, service ends without patching or negative balance.
29. Verify a failed patch verification rolls vehicle migration state back and refunds the service charge.

## Identity and authority attacks

30. Start a tow for vehicle A and alter/replace the actor so its persistent ID no longer matches: dispatch must cancel before charge/movement.
31. Start a patch for vehicle A and present vehicle B: patch must reject before charge/state mutation.
32. Two vehicles of the same model must remain distinguishable by persistent ID.
33. A respawned/recovered actor with the expected ID may only continue the higher-level cargo authority if that existing system rebinds it; roadside service itself never guesses nearest vehicle.
34. Query APIs must expose no pending quote/ID after successful completion.
35. Query APIs must expose no pending quote/ID after cancellation.

## Wanted / police separation

36. Request voluntary tow at Wanted 0, then raise Wanted to 1 before arrival: voluntary dispatch cancels with no charge.
37. At Wanted 1, new patch/tow requests are rejected.
38. At Wanted >=2 with a merely `TowRecommended` but still mobile vehicle, police impound must not auto-fire.
39. At Wanted >=2 with a truly stranded vehicle, police impound arms independently.
40. Police impound cannot be cancelled with `T` or `Y`.
41. Police impound uses its own fine/service calculation rather than a locked voluntary quote.
42. Police impound may perform mandatory safety service; voluntary tow must not.

## Farm Cargo continuity

43. Accept Farm Cargo, load Mulebox, then create a roadside-recovery condition.
44. Request tow: pinned roadside ID must equal cargo authority’s exact loaded vehicle ID.
45. Cancel tow: cargo contract, reserved stock, timer and vehicle authority remain unchanged.
46. Complete tow: same cargo vehicle ID remains authoritative at Hill Farm/North Wood Yard.
47. Attempt delivery with a second Mulebox after roadside recovery: reject it.
48. Emergency patch on loaded Mulebox must not duplicate or transfer cargo.
49. Contract timer continues while dispatch is inbound; roadside system must not pause Farm Job authority.
50. Roadside service must not directly modify cargo payout, reputation, stock reservation or completion history.
51. Save/load cargo authority remains owned by existing Farm Cargo persistence; pending roadside dispatch is transient and must not fabricate persisted service state.

## UX / input / regression

52. Keyboard `Y` and controller D-pad Left invoke the same patch path.
53. Keyboard `T` and controller D-pad Up invoke the same tow path.
54. Recovery prompt says quote locks on dispatch and the same key cancels before arrival.
55. Insufficient cash at request denies dispatch and leaves no pending state.
56. Leaving the eligible recovery state before arrival drops voluntary dispatch with no charge.
57. Existing repair/tuning/fuel/workshop systems remain authoritative after roadside recovery.
58. Existing 0.1.35 emergency-patch runtime verifier remains green.
59. `Scripts/verify_roadside_dispatch_contract.py` passes on the exact candidate SHA.
60. Protected Roadmap remains 125/130 = 96.2%; no checkbox closes from source-only evidence.

## Packaged demo gate — still required

61. Compile Unreal Engine 5.8 Win64 candidate from the exact release SHA.
62. Cook/package the game and retain the exact artifact identity.
63. Execute packaged EXE smoke test, including patch/tow cancellation and exact-ID cases.
64. Exercise Native Chaos drivetrain/wheels and authored trailer runtime acceptance.
65. Execute Farm Cargo + roadside recovery end-to-end in packaged runtime.
66. Capture rendered visual acceptance for world, vehicles, NPCs, HUD/UI, lighting and representative combat/mission flow.
67. Confirm all relevant exact-SHA Actions are green.
68. Only after every technical and visual gate passes may a public Demo Release be created.
