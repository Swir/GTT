# GTT 0.1.49 — Workshop Service Lifecycle Playtest

This matrix validates the new timed deferred-workshop lifecycle without treating source checks as packaged Win64 proof. Use a real Unreal Engine 5.8 game session for manual scenarios; packaged-demo acceptance remains gated separately.

## A. Booking and READY state
- [ ] 1. After closing, book one eligible owned damaged native road vehicle from the garage desk.
- [ ] 2. Confirm booking takes no cash.
- [ ] 3. Confirm booking does not repair condition.
- [ ] 4. Confirm booking does not repair tires.
- [ ] 5. Confirm booking does not refill fuel.
- [ ] 6. Confirm the request-time locked quote is positive.
- [ ] 7. Confirm the exact PersistentVehicleId is shown in queue data.
- [ ] 8. Confirm the appointment receives the deterministic next-opening slot.
- [ ] 9. Before the slot, the snapshot state remains QUEUED.
- [ ] 10. At/after the slot with the vehicle away from the workshop, the snapshot becomes READY.
- [ ] 11. A READY vehicle away from the workshop is not charged.
- [ ] 12. A READY vehicle away from the workshop is not repaired.

## B. Check-in and timed service
- [ ] 13. Park the exact READY vehicle inside a real workshop service area while the workshop is open.
- [ ] 14. Confirm the vehicle transitions to IN_SERVICE.
- [ ] 15. Confirm check-in takes no cash.
- [ ] 16. Confirm check-in performs no repair/refuel mutation.
- [ ] 17. Confirm ServiceStartDay/Hour are persisted.
- [ ] 18. Confirm ServiceCompleteDay/Hour are persisted.
- [ ] 19. Confirm service duration is never below 30 in-world minutes.
- [ ] 20. Confirm service duration is never above 90 in-world minutes.
- [ ] 21. Compare light and heavy damage; heavier workload must not produce a shorter service time.
- [ ] 22. Confirm tire deficit contributes to service time.
- [ ] 23. Confirm fuel deficit contributes to service time.
- [ ] 24. Confirm body damage/detached panels contribute to service time.

## C. Persistence
- [ ] 25. Save/reload while IN_SERVICE and confirm exact vehicle identity survives.
- [ ] 26. Confirm locked quote survives reload unchanged.
- [ ] 27. Confirm service start survives reload unchanged.
- [ ] 28. Confirm service completion survives reload unchanged.
- [ ] 29. Confirm reload does not debit cash.
- [ ] 30. Confirm reload does not mutate vehicle condition/fuel/tires.
- [ ] 31. Load a pre-0.1.49 schema-v1 queue save and confirm it becomes waiting/not checked in.
- [ ] 32. Corrupt lifecycle timestamps while retaining a valid appointment and confirm lifecycle resets without deleting the valid booking.
- [ ] 33. Confirm duplicate PersistentVehicleId entries are still rejected during restore.
- [ ] 34. Confirm invalid/impounded authority entries are still removed during restore.

## D. Leaving service early
- [ ] 35. Drive/move the checked-in vehicle outside the workshop radius before completion.
- [ ] 36. Confirm state returns to READY.
- [ ] 37. Confirm the locked quote remains unchanged.
- [ ] 38. Confirm no cash is charged when service is paused.
- [ ] 39. Confirm no repair/refuel occurs when service is paused.
- [ ] 40. Confirm only lifecycle timestamps are reset.
- [ ] 41. Return the exact vehicle to the workshop and confirm a fresh service timer starts.
- [ ] 42. Confirm another vehicle cannot inherit the paused appointment.

## E. Checkout and economy
- [ ] 43. Keep the exact checked-in vehicle in the workshop until ServiceComplete time.
- [ ] 44. Before ServiceComplete, confirm zero debit and zero repair mutation.
- [ ] 45. At/after ServiceComplete with sufficient cash, confirm exactly one locked-quote debit.
- [ ] 46. Confirm condition repairs through the existing production workshop mutation.
- [ ] 47. Confirm tires repair through the existing production workshop mutation.
- [ ] 48. Confirm fuel is restored by the existing workshop service path.
- [ ] 49. Confirm successful checkout removes only that appointment.
- [ ] 50. Confirm primary progress is saved after successful checkout.
- [ ] 51. Force service mutation failure and confirm the debit is refunded.
- [ ] 52. Confirm mutation failure clears only the affected appointment, matching the existing rollback policy.

## F. Capacity and authority isolation
- [ ] 53. Queue multiple exact vehicles and confirm one checked-in vehicle does not erase other appointments.
- [ ] 54. Let an earlier completed service lack funds and confirm its state becomes AWAITING_PAYMENT.
- [ ] 55. Confirm the underfunded completed service remains unpaid and unrepaired.
- [ ] 56. Confirm a later due appointment can still check in/process independently.
- [ ] 57. Confirm active Farm Cargo still permits only its bound vehicle where the existing authority rule applies.
- [ ] 58. Confirm an impounded vehicle cannot proceed through queued service.
- [ ] 59. Confirm hard TOW/IMMOBILE WORKSHOP HOLD still takes the separate emergency path.
- [ ] 60. Confirm exact-ID ambiguity fails closed without charging.

## G. Presentation and release honesty
- [ ] 61. Garage queue text reports IN_SERVICE plus service ETA for a checked-in vehicle.
- [ ] 62. Garage queue text reports AWAITING_PAYMENT after the timer when checkout cannot be paid.
- [ ] 63. Run all source verifiers plus deterministic SWIR progress SVG check; Roadmap remains 125/130 = 96.2% with no legacy text meter.
- [ ] 64. Do not mark demo ready unless the exact candidate also passes real UE 5.8 Win64 package/runtime smoke and visual acceptance gates.

## H. Direct terminal bypass protection
- [ ] 65. With an exact queued vehicle parked at the workshop before its appointment is due, interact with the workshop terminal and confirm direct walk-up repair/refuel remains blocked with zero debit and zero mutation.
- [ ] 66. At READY state, interact repeatedly and confirm the terminal surfaces the locked quote/queue state while the queue lifecycle, not immediate workshop service, owns check-in and service timing.
- [ ] 67. During IN_SERVICE, interact repeatedly and confirm the terminal cannot bypass the persisted service timer, cannot recalculate the quote and cannot charge a second/direct service path.
- [ ] 68. Put the same vehicle under a real TOW/IMMOBILE WORKSHOP HOLD and confirm hard WORKSHOP HOLD still takes priority over queued authority so the existing daytime/after-hours recovery lane remains available.
