# GTT 0.1.34 Playtest — Roadside Emergency Patch & Farm Cargo Recovery Choice

## Purpose

Validate the new native roadside emergency patch as a real player choice beside towing, and prove that Farm Cargo keeps the same exact loaded vehicle, timer, save authority and delivery rules through either recovery path.

This checklist is for an eventual UE 5.8 runtime pass. Source-contract CI can validate the wiring and invariants, but cannot substitute for the packaged Windows scenarios below.

## A. Recovery choice and pricing

1. Damage a native Rattleback until `TowRecommended`, stop below 3.5 km/h and confirm recovery UI appears.
2. Confirm the HUD shows separate PATCH, TOW and full REPAIR estimates.
3. Confirm `Y` requests a patch.
4. Confirm controller D-pad Left requests the same patch.
5. Confirm `T` requests a tow.
6. Confirm controller D-pad Up requests the same tow.
7. Confirm a patch request blocks a second patch request.
8. Confirm a patch request blocks a tow request until the patch resolves/cancels.
9. Confirm a tow request blocks a patch request until the tow resolves/cancels.
10. Confirm insufficient cash rejects patch without modifying vehicle state.
11. Confirm insufficient cash rejects tow without moving the vehicle.
12. Confirm patch is not offered on a healthy native vehicle.
13. Confirm tow is not offered while moving faster than the recovery speed gate.
14. Confirm patch is not offered while moving faster than the recovery speed gate.
15. Confirm `TowRecommended` can now accept voluntary tow even when not fully immobilized.
16. Confirm this voluntary-tow expansion does not trigger automatic police impound by itself.

## B. Emergency patch behavior

17. Patch a condition-disabled vehicle and confirm condition reaches at least 30%, not 100%.
18. Patch a tire-disabled vehicle and confirm tire integrity reaches at least 32%, not 100%.
19. Patch an out-of-fuel vehicle and confirm emergency fuel reaches up to 5 L, not a full tank.
20. Confirm already-better condition is not reduced by patch.
21. Confirm already-better tire integrity is not reduced by patch.
22. Confirm already-higher fuel is not reduced by patch.
23. Confirm body front health is unchanged by patch.
24. Confirm body rear health is unchanged by patch.
25. Confirm body left/right health is unchanged by patch.
26. Confirm detached-panel count and mask are unchanged by patch.
27. Confirm engine/tire upgrade levels are unchanged by patch.
28. Confirm `PersistentVehicleId` is unchanged by patch.
29. Confirm cargo load factor is not reset by patch.
30. Confirm patch charges exactly the quoted amount once.
31. Confirm full workshop repair remains priced separately after patch.
32. Confirm structurally/body-disabled vehicle refuses patch and offers tow instead.

## C. Wanted / police interaction

33. With wanted level 1, confirm patch is rejected.
34. With wanted level 1, confirm tow is rejected.
35. With wanted level 2+, confirm patch is rejected.
36. With wanted level 2+, confirm normal tow is rejected.
37. Start a patch request at wanted 0, gain wanted before completion and confirm the player service is cancelled without charging.
38. Start a tow request at wanted 0, gain wanted before completion and confirm the player service is cancelled without charging.
39. Fully immobilize during wanted 2+ and confirm existing police impound still arms.
40. Confirm police impound clears/handles wanted through the existing authority and does not use the emergency-patch path.

## D. Farm Cargo exact-vehicle continuity

41. Accept Farm Cargo and load the native Mulebox; record its persistent ID.
42. Damage the loaded Mulebox into patch-eligible recovery.
43. Confirm the cargo recovery panel names the recovery state and keeps the same vehicle ID.
44. Confirm the Farm Cargo timer continues during patch dispatch.
45. Confirm a primary-save checkpoint is attempted before patch service.
46. Complete patch and confirm a primary-save checkpoint is attempted afterward.
47. Confirm exact loaded vehicle remains cargo authority after patch.
48. Park another Mulebox at Hill Farm and confirm it cannot hand off the patched vehicle's cargo.
49. Confirm the patched exact Mulebox can perform the Hill Farm handoff.
50. Continue to North Wood Yard and complete with the same persistent ID.
51. Confirm payout occurs once.
52. Confirm logistics reputation increases once.
53. Confirm stock/reservation is not duplicated by patch checkpoints.
54. Confirm save/load after patch rebinds only the exact persistent ID.
55. Confirm destroying/recreating the exact actor after patch can recover through the existing identity path.
56. Confirm a different same-model van still cannot inherit the load after reload.

## E. Tow and regression coverage

57. Repeat the cargo breakdown but choose tow; confirm exact vehicle identity remains unchanged.
58. Confirm ordinary tow still preserves condition/tire/body damage rather than repairing it.
59. Confirm cargo timer continues during tow.
60. Confirm post-tow exact-ID verification succeeds before delivery resumes.
61. Confirm 0.1.33 packaged breakdown evidence harness remains source-compatible.
62. Confirm 0.1.32 breakdown/tow recovery source verifier still passes.
63. Confirm SWIR progress generator `--check` stays green at 125/130 = 96.2%.
64. Confirm no GitHub Demo Release is created unless the exact Win64 candidate also passes packaged runtime and visual acceptance gates.

## Release acceptance

0.1.34 is a source/gameplay milestone only until an actual UE 5.8 Win64 package executes these recovery paths. Do not mark any of the five remaining Roadmap blockers complete from this checklist alone.
