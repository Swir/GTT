# GTT 0.1.46 Playtest — Packaged Workshop Repair Queue Evidence

This matrix validates the 0.1.45 deferred workshop queue in the exact packaged Win64 candidate. Source CI verifies wiring only; every runtime row remains unverified until Unreal Engine 5.8 packages and executes the candidate.

## A — After-hours booking
- [ ] 01. Start with no existing `GTT_WorkshopQueue_01` reservation.
- [ ] 02. Use an owned native road vehicle with stable non-empty `PersistentVehicleId`.
- [ ] 03. Vehicle is damaged/mobile and requires workshop service.
- [ ] 04. Vehicle has no TOW/IMMOBILE WORKSHOP HOLD.
- [ ] 05. Set clock after 20:00 and confirm regular workshop is closed.
- [ ] 06. Queue through production `TryQueueNearestEligibleNativeRoadVehicle`.
- [ ] 07. Reservation pins the exact vehicle ID.
- [ ] 08. Locked quote is positive.
- [ ] 09. Ready time resolves to the next 06:30 opening.
- [ ] 10. Booking takes no cash.
- [ ] 11. Booking does not repair/refuel the vehicle.
- [ ] 12. Sidecar exists immediately after accepted booking.

## B — SaveGame checkpoint roundtrip
- [ ] 13. Load `GTT_WorkshopQueue_01` through Unreal SaveGame API.
- [ ] 14. Sidecar schema equals 1.
- [ ] 15. `bQueued` remains true.
- [ ] 16. Persistent vehicle ID equals the accepted target.
- [ ] 17. Locked quote equals the request-time quote.
- [ ] 18. Requested day/hour survive serialization.
- [ ] 19. Ready day/hour survive serialization.
- [ ] 20. Disk load takes no cash.
- [ ] 21. Disk load does not mutate vehicle condition.
- [ ] 22. Queue snapshot still reports the same target.
- [ ] 23. Queue snapshot still reports the same locked quote.
- [ ] 24. Production `WORKSHOP_QUEUE_ACCEPTED` marker exists.

## C — Substitute vehicle rejection
- [ ] 25. Capture a different owned native road vehicle as decoy.
- [ ] 26. Decoy has a different persistent ID.
- [ ] 27. Move exact target outside workshop parking radius before opening.
- [ ] 28. Park decoy inside workshop radius.
- [ ] 29. Advance clock to the queued ready opening.
- [ ] 30. Allow at least two queue tick intervals.
- [ ] 31. Reservation remains queued.
- [ ] 32. Reservation target remains exact original ID.
- [ ] 33. Locked quote remains unchanged.
- [ ] 34. No cash is charged while only decoy is present.
- [ ] 35. Decoy is not repaired by another vehicle's reservation.
- [ ] 36. Sidecar remains present after substitute rejection.

## D — Exact-vehicle service and economy
- [ ] 37. Move exact target into workshop parking radius.
- [ ] 38. Leave clock at/after opening while workshop is open.
- [ ] 39. Production queue executes without a second booking.
- [ ] 40. `SpendCash` uses the persisted locked quote.
- [ ] 41. Cash debit equals locked quote exactly once.
- [ ] 42. No additional queue debit occurs.
- [ ] 43. Existing `ApplyNativeWorkshopService()` is the mutation authority.
- [ ] 44. Mechanical condition returns to service threshold.
- [ ] 45. Tire integrity returns to service threshold.
- [ ] 46. Fuel is restored by normal workshop service.
- [ ] 47. Persistent vehicle ID remains unchanged.
- [ ] 48. Reservation clears after successful service.
- [ ] 49. Sidecar is deleted after successful service.
- [ ] 50. Production `WORKSHOP_QUEUE_COMPLETED` marker appears exactly once.
- [ ] 51. Completion marker reports `locked_quote_match=YES`.
- [ ] 52. Completion marker reports `saved=YES`.

## E — Farm Cargo and authority continuity
- [ ] 53. Capture primary Farm Cargo active flag before booking.
- [ ] 54. Capture primary Farm Cargo stage before booking.
- [ ] 55. Capture primary bound vehicle ID before booking.
- [ ] 56. Capture cargo integrity before booking.
- [ ] 57. Capture remaining route time before booking.
- [ ] 58. Queue never assigns a different cargo vehicle.
- [ ] 59. Queue never advances/revives cargo stage.
- [ ] 60. Queue never improves cargo integrity.
- [ ] 61. Queue never rewinds route timer.
- [ ] 62. Post-service primary save preserves cargo authority fields.

## F — Manifest, gate and regression truth
- [ ] 63. Runtime completion marker is PASS with all required booleans.
- [ ] 64. No `WORKSHOP_QUEUE_RUNTIME phase=DIAGNOSTIC result=FAIL` marker exists.
- [ ] 65. Evaluator emits `gtt.workshop-queue-runtime.v1`.
- [ ] 66. Manifest SHA equals packaged `BUILD_INFO.json` SHA.
- [ ] 67. Manifest records exact ID, positive locked quote and single debit.
- [ ] 68. Manifest records checkpoint disk roundtrip and substitute rejection.
- [ ] 69. Manifest records sidecar cleanup and Farm Cargo authority preservation.
- [ ] 70. Schema-14 workshop-hours technical gate is a required prerequisite.
- [ ] 71. Promotion reaches schema 15 only from same-SHA PASS evidence.
- [ ] 72. Source CI alone leaves roadmap at 125/130 and never claims demo/Win64 runtime readiness.
