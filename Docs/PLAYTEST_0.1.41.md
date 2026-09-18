# GTT 0.1.41 — Garage / Workshop Recovery Playtest

This milestone closes the gameplay loophole where an ordinary damage-preserving roadside tow could be followed by a cheaper garage recall that moved the same broken vehicle away from the workshop without repairing it.

## A. Baseline fleet states

1. Start with an owned Rattleback in `READY`; confirm its numbered bay offers normal dispatch.
2. Start with an owned Mulebox in `READY`; confirm normal dispatch remains available.
3. Put a road vehicle into `LIMP`; confirm the bay still permits deliberate dispatch.
4. Put a road vehicle into ordinary `SERVICE`; confirm it remains advisory rather than a hard hold.
5. Put a road vehicle into `TOW`; confirm the bay label shows `WORKSHOP HOLD`.
6. Put a road vehicle into `IMMOBILE`; confirm the bay label shows `WORKSHOP HOLD`.
7. Confirm a workshop hold does not alter the vehicle's persistent ID.
8. Confirm a workshop hold does not alter engine/tire upgrade levels.

## B. Roadside tow handoff

9. Damage a native road vehicle until tow is recommended.
10. Request ordinary roadside tow and note the locked tow quote.
11. Let tow complete; confirm the same exact vehicle reaches the workshop drop.
12. Confirm tow charges exactly the locked quote once.
13. Confirm ordinary tow does not repair condition.
14. Confirm ordinary tow does not repair tires.
15. Confirm ordinary tow does not restore detached/body damage.
16. Confirm ordinary tow keeps the same `PersistentVehicleId`.
17. At the garage office, confirm the fleet summary reports at least one workshop hold.
18. At that vehicle's numbered bay, confirm dispatch is blocked.
19. Confirm the garage recall fee is not charged on the blocked attempt.
20. Confirm the vehicle is not teleported away from the workshop on the blocked attempt.

## C. Workshop release

21. Interact with the workshop while the held native road vehicle is in range.
22. Confirm the interaction text identifies recovery service and current TOW/IMMOBILE state.
23. Confirm the workshop uses the damage-based repair quote rather than the garage recall fee.
24. With insufficient cash, confirm service fails without mutating the vehicle.
25. With sufficient cash, buy the service.
26. Confirm authoritative native workshop service repairs mechanical condition.
27. Confirm tire condition is restored by the workshop service.
28. Confirm body/structural service is applied according to the existing workshop rules.
29. Confirm fuel is restored by the full mechanical service.
30. Confirm the workshop reports `WORKSHOP HOLD cleared`.
31. Confirm progress is saved after successful service.
32. Re-open the garage office; confirm the hold count decreases.
33. Re-open the numbered bay; confirm normal dispatch is available again.
34. Dispatch the serviced vehicle and confirm the recall fee is charged exactly once.
35. Confirm damage/fuel/tuning state after garage dispatch matches the serviced state rather than pre-tow damage.

## D. Advisory-state behavior

36. Reduce condition enough for `LIMP` but not `TOW`/`IMMOBILE`.
37. Confirm garage dispatch is still allowed.
38. Confirm the garage summary still recommends workshop service.
39. Create ordinary `SERVICE` wear and confirm dispatch remains allowed.
40. Confirm a fuel-only visit uses the exact per-litre quote when no hard mechanical hold exists.
41. Confirm a fuel-only visit does not claim that a mechanical workshop hold was cleared.

## E. Farm Cargo continuity

42. Accept Farm Cargo with the authoritative Mulebox.
43. Bind the load to the exact Mulebox at pickup.
44. Damage that same Mulebox until ordinary tow is required.
45. Tow it to the workshop and confirm cargo authority remains on the same persistent ID.
46. Attempt garage recall while the Mulebox is held; confirm it is blocked without charge.
47. Confirm the cargo timer continues during the workshop consequence.
48. Service the exact Mulebox at the workshop.
49. Confirm the workshop save checkpoint does not transfer cargo to another vehicle.
50. Dispatch/continue with the same Mulebox after hold clearance.
51. Attempt Hill Farm with a decoy vehicle; confirm rejection still works.
52. Complete Hill Farm and North Wood Yard with the exact Mulebox.
53. Confirm payout/reputation occur once.
54. Save/load after service and confirm the repaired exact vehicle remains authoritative.

## F. Law-response separation

55. Trigger police impound on a genuinely stranded vehicle during Wanted escalation.
56. Confirm police impound remains its existing safety-service path and is not reported as an ordinary player workshop hold.
57. Confirm Wanted still blocks voluntary roadside dispatch before any garage/workshop handoff.
58. Confirm game-warden/police restrictions on garage dispatch remain unchanged.

## G. Regression / presentation

59. Run `python Scripts/verify_garage_workshop_recovery.py`.
60. Run `python Scripts/generate_progress_svg.py --check`.
61. Confirm `Docs/ROADMAP.md` remains 125 / 130 = 96.2% with the compact SVG and no legacy character meter.
62. Confirm README remains SWIR README PRO v2 with one progress card and Search Keywords.
63. Confirm source CI is green before merge.
64. Do not claim packaged Win64 runtime verification unless a qualifying UE 5.8 Windows run actually produces it.
