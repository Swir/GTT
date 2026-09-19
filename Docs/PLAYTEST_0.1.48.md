# GTT 0.1.48 — Packaged Multi-Vehicle Workshop Capacity Playtest

Source contract and future packaged-runtime review plan. Check items only from observed evidence; this document itself is not runtime proof.

## A. Preconditions and identity
- [ ] 1. Packaged candidate SHA matches BUILD_INFO.json.
- [ ] 2. Two distinct owned native road vehicles exist.
- [ ] 3. Both vehicles expose non-empty PersistentVehicleId values.
- [ ] 4. Workshop terminal resolves in the playable world.
- [ ] 5. Workshop repair queue starts empty for the evidence window.
- [ ] 6. Queue capacity reports four appointments.
- [ ] 7. World time is moved after regular workshop closing.
- [ ] 8. Primary Farm Cargo authority baseline is readable.
- [ ] 9. Active Farm Cargo correctly prevents a fake two-vehicle proof.

## B. Two independent bookings
- [ ] 10. First damaged owned vehicle can be selected alone near the workshop.
- [ ] 11. First booking records its exact PersistentVehicleId.
- [ ] 12. First booking records a positive locked quote.
- [ ] 13. First booking does not debit cash.
- [ ] 14. Second damaged owned vehicle can be selected independently.
- [ ] 15. Second booking records its exact PersistentVehicleId.
- [ ] 16. Second booking records its own positive locked quote.
- [ ] 17. Second booking does not debit cash.
- [ ] 18. Queue contains exactly two appointments.
- [ ] 19. First and second IDs differ.
- [ ] 20. Severe first damage produces a higher quote than the mild second damage.
- [ ] 21. First slot is the next opening capacity slot.
- [ ] 22. Second slot is exactly 45 minutes after the first.

## C. Additive disk persistence
- [ ] 23. GTT_WorkshopQueue_01 exists after booking.
- [ ] 24. Sidecar schema remains additive schema 1.
- [ ] 25. Appointments contains exactly two entries.
- [ ] 26. First on-disk ID matches the first runtime vehicle.
- [ ] 27. Second on-disk ID matches the second runtime vehicle.
- [ ] 28. First on-disk locked quote matches runtime.
- [ ] 29. Second on-disk locked quote matches runtime.
- [ ] 30. Ready day/hour survives the disk checkpoint.
- [ ] 31. Legacy first-entry mirror remains compatible with 0.1.46 evidence.

## D. Independent cancellation and rebooking
- [ ] 32. Cancelling the second exact ID succeeds.
- [ ] 33. Cancellation removes only the second appointment.
- [ ] 34. First appointment remains unchanged.
- [ ] 35. Cancellation does not debit cash.
- [ ] 36. Rebooking the second vehicle succeeds.
- [ ] 37. Rebook restores the same request-time locked quote.
- [ ] 38. Rebook recreates the deterministic second capacity slot.
- [ ] 39. Rebook does not debit cash.
- [ ] 40. Queue returns to exactly two appointments.

## E. Underfunded non-blocking execution
- [ ] 41. Evidence cash is constrained to exactly the cheaper second quote.
- [ ] 42. Both exact vehicles are physically staged at the workshop.
- [ ] 43. World time advances to the later due slot.
- [ ] 44. Earlier expensive appointment fails affordability without mutation.
- [ ] 45. Earlier appointment remains queued.
- [ ] 46. Earlier vehicle remains damaged.
- [ ] 47. Later affordable appointment is still evaluated in the same production loop.
- [ ] 48. Later appointment services only its exact vehicle.
- [ ] 49. Later service charges exactly its locked quote once.
- [ ] 50. Player cash reaches zero, not negative.
- [ ] 51. Later vehicle condition is repaired.
- [ ] 52. Later vehicle tire integrity is repaired.
- [ ] 53. Later vehicle is refuelled.
- [ ] 54. Later appointment is removed.
- [ ] 55. Queue contains exactly the earlier appointment afterward.

## F. Authority and persistence safety
- [ ] 56. Both PersistentVehicleId values are unchanged after execution.
- [ ] 57. Primary Farm Cargo active flag is unchanged.
- [ ] 58. Primary Farm Cargo stage is unchanged.
- [ ] 59. Primary Farm Cargo bound vehicle is unchanged.
- [ ] 60. Cargo integrity is not magically improved.
- [ ] 61. Cargo timer is not rewound.
- [ ] 62. Hard WORKSHOP HOLD entries remain outside deferred appointments.
- [ ] 63. Evidence cleanup cancels the remaining test appointment.
- [ ] 64. Evidence cleanup restores both vehicle snapshots.
- [ ] 65. Evidence cleanup restores player economy.
- [ ] 66. Evidence cleanup restores world time.

## G. Manifest and release gate
- [ ] 67. WORKSHOP_CAPACITY_RUNTIME.json is emitted only after all route phases PASS.
- [ ] 68. Manifest schema is gtt.workshop-capacity-runtime.v1.
- [ ] 69. Manifest git_sha matches the packaged candidate SHA.
- [ ] 70. Diagnostic failure count is zero.
- [ ] 71. Schema-15 technical gate promotes to schema 16 only with same-SHA capacity PASS evidence.
- [ ] 72. No demo Release is created unless Win64 package, runtime, visual and all remaining release gates independently pass.
