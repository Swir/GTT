# GTT 0.1.47 — Multi-Vehicle Workshop Appointments Playtest

Source milestone playtest contract. These scenarios describe editor/packaged verification targets; checking them in source documentation does **not** claim they have run on a packaged Win64 candidate.

## A. Booking and capacity

- [ ] 1. After 20:00, queue the nearest eligible owned damaged native road vehicle from the garage office.
- [ ] 2. Verify the first booking pins the exact persistent vehicle ID.
- [ ] 3. Verify the first booking stores a positive request-time locked quote.
- [ ] 4. Verify booking takes no cash.
- [ ] 5. Queue a second different eligible vehicle without cancelling the first.
- [ ] 6. Queue a third different eligible vehicle.
- [ ] 7. Queue a fourth different eligible vehicle.
- [ ] 8. Verify the appointment book reports 4/4 capacity.
- [ ] 9. Attempt a fifth booking and verify it is rejected without charge or mutation.
- [ ] 10. Verify an already-reserved vehicle is skipped when choosing the next candidate.
- [ ] 11. Verify an undamaged vehicle is not admitted to the appointment book.
- [ ] 12. Verify an unowned vehicle is not admitted to the appointment book.

## B. Scheduling and persistence

- [ ] 13. Verify the first after-hours appointment resolves to the next 06:30 opening.
- [ ] 14. Verify the second appointment is 45 minutes after the first.
- [ ] 15. Verify the third appointment is 45 minutes after the second.
- [ ] 16. Verify the fourth appointment is 45 minutes after the third.
- [ ] 17. Book close to a closing boundary and verify overflow rolls to the next opening day.
- [ ] 18. Save and reload with one appointment and verify exact ID, quote, day and hour survive.
- [ ] 19. Save and reload with four appointments and verify count survives.
- [ ] 20. Verify appointment ordering survives reload.
- [ ] 21. Verify each appointment keeps its own locked quote after reload.
- [ ] 22. Load a legacy single-reservation sidecar and verify it migrates into the additive appointment list.
- [ ] 23. Verify the legacy first-entry mirror still matches the earliest appointment.
- [ ] 24. Verify duplicate persistent IDs in stored appointment data do not restore twice.

## C. Cancellation and exact identity

- [ ] 25. Cancel the second appointment by its exact vehicle ID.
- [ ] 26. Verify cancelling one appointment leaves all other appointments intact.
- [ ] 27. Verify cancellation takes no cash.
- [ ] 28. Verify cancellation does not repair, refuel or move the vehicle.
- [ ] 29. Attempt cancellation with NAME_None/empty ID and verify rejection.
- [ ] 30. Attempt cancellation with a different vehicle ID and verify rejection.
- [ ] 31. Refill the freed slot with a new eligible vehicle and verify capacity returns to 4/4.
- [ ] 32. Verify a recreated actor with the same valid persistent ID remains the reservation target.
- [ ] 33. Create two actors with the same queued persistent ID and verify service fails closed as ambiguous.
- [ ] 34. Verify ambiguous-ID failure removes only the affected appointment.
- [ ] 35. Verify another unaffected due appointment remains serviceable after ambiguous-ID rejection.
- [ ] 36. Verify garage text exposes appointment count/capacity rather than a single global QUEUED flag.

## D. Service execution and non-blocking economy

- [ ] 37. At 06:30, park the first exact vehicle at a real workshop terminal and verify service executes.
- [ ] 38. Verify service charges exactly the stored locked quote once.
- [ ] 39. Verify successful service repairs mechanical condition.
- [ ] 40. Verify successful service refuels the exact vehicle.
- [ ] 41. Verify successful service preserves the persistent vehicle ID.
- [ ] 42. Verify successful service removes only that appointment from the sidecar.
- [ ] 43. Make the earliest due vehicle unaffordable and leave it parked at the workshop.
- [ ] 44. Verify the unaffordable appointment stays queued and unpaid.
- [ ] 45. Verify the unaffordable vehicle receives no repair/refuel mutation.
- [ ] 46. Park a later due affordable vehicle at the workshop.
- [ ] 47. Verify the later due affordable appointment executes despite the earlier unpaid entry.
- [ ] 48. Verify the later service is charged exactly once and the unpaid earlier reservation remains.

## E. Authority, emergency lane and rollback

- [ ] 49. Put an appointment vehicle into authoritative TOW/IMMOBILE WORKSHOP HOLD before execution.
- [ ] 50. Verify hard hold pre-empts/removes the deferred appointment instead of using the queue.
- [ ] 51. Verify the hard-hold vehicle remains eligible for the existing after-hours emergency +35% lane.
- [ ] 52. Verify LIMP/SERVICE advisory state does not become a hard hold solely because an appointment exists.
- [ ] 53. Start active Farm Cargo bound to one vehicle and verify only that bound vehicle is queue-compatible.
- [ ] 54. Verify an unrelated vehicle cannot steal Farm Cargo authority through workshop booking.
- [ ] 55. Verify queue code does not mutate Farm Cargo stage, timer, integrity or bound vehicle ID.
- [ ] 56. Impound an appointment vehicle and verify the deferred appointment fails closed.
- [ ] 57. Force authoritative workshop mutation failure and verify the exact debit is refunded.
- [ ] 58. Verify mutation failure clears only the failed appointment.
- [ ] 59. Verify a later appointment remains present after rollback of another entry.
- [ ] 60. Verify primary progress save is called after each successful workshop service.

## F. UX, regression and release truth

- [ ] 61. Verify garage summary shows workshop OPEN/CLOSED state and schedule together with appointment capacity.
- [ ] 62. Verify garage summary explains that each appointment has an exact ID and locked quote.
- [ ] 63. Verify garage summary explains payment happens only when service executes.
- [ ] 64. Verify after-hours garage interaction can add another appointment while the book is non-empty.
- [ ] 65. Verify a full book does not silently replace or overwrite an existing reservation.
- [ ] 66. Verify 0.1.45 no-precharge/exact-ID source regression check still passes.
- [ ] 67. Verify 0.1.46 packaged single-appointment evidence contract remains source-compatible.
- [ ] 68. Verify hard WORKSHOP HOLD still never enters the deferred appointment book.
- [ ] 69. Verify README embeds exactly one progress-card SVG and no text/Unicode progress meter.
- [ ] 70. Verify Roadmap embeds exactly one progress-mini SVG and remains 125/130 = 96.2%.
- [ ] 71. Verify source CI does not claim Unreal/Win64 packaged runtime evidence for 0.1.47.
- [ ] 72. Verify no Demo Release is created until the existing Win64/runtime/visual gates are genuinely satisfied.
