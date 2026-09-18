# GTT 0.1.45 Playtest — Persistent Workshop Repair Queue & Deferred Economy

Source-contract CI verifies wiring/invariants only. Runtime rows remain unverified until an actual Unreal Engine 5.8 build executes them.

## Test setup
- Use the ordinary village map with day/night cycle, garage office, workshop terminal and at least two owned Native road vehicles.
- Keep one damaged/mobile vehicle, one serviceable vehicle, one hard WORKSHOP HOLD case and an active Farm Cargo Mulebox scenario available.
- Regular workshop hours remain 06:30 inclusive through 20:00 exclusive. Hard holds retain the separate +35% after-hours emergency recovery path.

## A — Reservation eligibility and scheduling
- [ ] 01. 20:00 closed workshop allows an owned damaged/mobile Rattleback to be queued from the garage desk.
- [ ] 02. 02:00 closed workshop allows an owned damaged/mobile Mulebox to be queued.
- [ ] 03. 06:29 resolves same-day 06:30 opening.
- [ ] 04. 20:00 resolves next-day 06:30 opening.
- [ ] 05. 23:59 resolves next-day 06:30 opening.
- [ ] 06. 06:30 OPEN rejects deferred booking and uses normal service.
- [ ] 07. 19:59 OPEN rejects deferred booking.
- [ ] 08. Healthy Native road vehicle is not queued.
- [ ] 09. Unowned Native road vehicle is not queued.
- [ ] 10. Vehicle without persistent ID is not queued.
- [ ] 11. Native actor without active takeover is not queued.
- [ ] 12. Nearest eligible vehicle is selected inside the garage search radius.

## B — Locked quote and no-precharge economy
- [ ] 13. Booking locks a positive request-time repair quote.
- [ ] 14. Booking does not call SpendCash.
- [ ] 15. Booking does not change player cash.
- [ ] 16. Booking does not call ApplyNativeWorkshopService.
- [ ] 17. Booking does not repair condition.
- [ ] 18. Booking does not repair tires.
- [ ] 19. Booking does not restore body panels.
- [ ] 20. Booking does not refill fuel.
- [ ] 21. Same-target repeat preserves the original locked quote.
- [ ] 22. A second target cannot overwrite an existing reservation.
- [ ] 23. Queue status exposes exact target and locked quote.
- [ ] 24. Queue status exposes next opening and live game-hour ETA.

## C — Persistence and reload
- [ ] 25. GTT_WorkshopQueue_01 exists only after valid booking.
- [ ] 26. Sidecar schema is exactly v1.
- [ ] 27. Sidecar stores PersistentVehicleId.
- [ ] 28. Sidecar stores LockedQuote.
- [ ] 29. Sidecar stores requested day/hour.
- [ ] 30. Sidecar stores ready day/hour.
- [ ] 31. Reload restores the same target ID.
- [ ] 32. Reload restores the original locked quote without repricing.
- [ ] 33. Invalid/empty target sidecar clears without charge.
- [ ] 34. Zero/negative quote sidecar clears without charge.
- [ ] 35. Invalid ready-hour sidecar clears without charge.
- [ ] 36. Queue reload never rewrites unrelated primary-save progression.

## D — Opening-time exact-vehicle execution
- [ ] 37. Before ready time, no debit or vehicle mutation occurs.
- [ ] 38. Ready time while workshop is closed still waits.
- [ ] 39. Exact vehicle away from workshop waits unpaid.
- [ ] 40. Exact vehicle parked by a Workshop service terminal becomes eligible.
- [ ] 41. Execution requires exact PersistentVehicleId.
- [ ] 42. Same-model substitute cannot execute the reservation.
- [ ] 43. Duplicate exact IDs fail closed without charge.
- [ ] 44. Execution requires owned vehicle state.
- [ ] 45. Execution requires active Native takeover.
- [ ] 46. Vehicle that no longer needs repair clears stale reservation unpaid.
- [ ] 47. Successful execution debits exactly the locked quote once.
- [ ] 48. Successful execution uses ApplyNativeWorkshopService.
- [ ] 49. Successful execution writes normal primary SaveProgress.
- [ ] 50. Successful execution clears the queue sidecar.

## E — Failure and rollback
- [ ] 51. Insufficient cash leaves reservation pending.
- [ ] 52. Insufficient cash does not mutate vehicle.
- [ ] 53. Insufficient cash does not change locked quote.
- [ ] 54. Failed service mutation refunds exact debit.
- [ ] 55. Failed mutation clears unusable reservation after refund.
- [ ] 56. Restart after success does not resurrect reservation.
- [ ] 57. Exact-ID cancellation succeeds with no charge.
- [ ] 58. Wrong-ID cancellation is rejected.

## F — Hard hold and law separation
- [ ] 59. Existing TOW WORKSHOP HOLD cannot enter deferred queue.
- [ ] 60. Existing IMMOBILE WORKSHOP HOLD cannot enter deferred queue.
- [ ] 61. Queued vehicle that later becomes hard hold exits queue without debit.
- [ ] 62. Hard hold keeps established +35% after-hours emergency recovery.
- [ ] 63. Deferred queue never applies emergency surcharge.
- [ ] 64. Emergency hold path never consumes queued locked quote.
- [ ] 65. Primary-save impounded target fails closed.
- [ ] 66. Queue never writes ImpoundedVehicleId.

## G — Farm Cargo continuity
- [ ] 67. Active Farm Cargo queues only FarmCargoBoundVehicleId.
- [ ] 68. Different owned road vehicle is rejected while cargo authority is active.
- [ ] 69. Queue does not change FarmCargoStage.
- [ ] 70. Queue does not change FarmCargoTimeRemaining.
- [ ] 71. Queue does not heal FarmCargoIntegrity.
- [ ] 72. Queue does not reserve depot stock again.
- [ ] 73. Queue does not pay cargo revenue/reputation.
- [ ] 74. Successful repair keeps authority on the same Mulebox.

## H — UI, regressions and release honesty
- [ ] 75. Garage summary shows queue EMPTY before booking.
- [ ] 76. Garage summary shows 1 QUEUED after booking.
- [ ] 77. Garage summary shows target ID, locked quote and ETA.
- [ ] 78. Garage summary distinguishes queue from WORKSHOP HOLD.
- [ ] 79. Workshop schedule remains 06:30–20:00 and emergency +35% behavior remains intact.
- [ ] 80. Roadmap remains 125/130 = 96.2%, SVG-only presentation stays green, and no Demo Release is created without Win64/Chaos/trailer/visual gates.

## Acceptance
Source acceptance requires the dedicated 0.1.45 verifier, existing workshop/garage regressions and deterministic SWIR SVG validation to pass. Packaged Win64 behavior, Chaos runtime, authored trailer and rendered visual acceptance remain separate release gates and are not claimed by this source milestone.
