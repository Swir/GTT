# GTT 0.1.52 — Workshop Priority/Pickup Packaged Evidence Playtest

This is a **72-case matrix** for the future qualifying Unreal Engine 5.8 Win64 candidate. The new deterministic route must exercise the production workshop queue rather than a duplicate repair/economy path. **No packaged Win64 proof is claimed** until the exact candidate actually emits the required PASS manifest and the rendered demo gate also passes.

| Area | Six cases to execute |
|---|---|
| 1. Route activation | scenario absent stays inert; demo-smoke alone stays inert; priority flag alone stays inert; both flags enable once; begin marker version=1; route starts only after historical workshop evidence window |
| 2. STANDARD booking | exact owned damaged vehicle selected; STANDARD accepted after closing; request-time quote positive; no pre-charge; exact `PersistentVehicleId` pinned; unrelated vehicle is not mutated |
| 3. URGENT promotion | exact-ID promotion succeeds; +20% uses deterministic rounded surcharge; quote is locked; `Priority=URGENT`; no cash is spent; second promotion is rejected safely |
| 4. Priority persistence | sidecar remains schema v1; urgent flag survives disk read; urgent locked quote survives disk read; exact ID survives; paid fields remain empty before checkout; legacy first-entry mirror remains valid |
| 5. Timed check-in | exact vehicle must be at workshop; check-in uses production queue; no charge at check-in; start/complete timestamps persist; service state becomes IN_SERVICE; leaving the bay would still use normal pause semantics |
| 6. x0.80 timing | workload-derived STANDARD duration is measured; URGENT is x0.80; minimum clamp remains 0.40h; maximum urgent clamp remains 1.20h; completion clock matches snapshot; timing does not change locked quote |
| 7. Checkout economy | checkout waits until completion time; one debit equals URGENT locked quote; no duplicate debit; failed mutation would use existing rollback; successful service repairs condition/tires; successful service refuels through production repair path |
| 8. READY_FOR_PICKUP | successful checkout persists `READY_FOR_PICKUP`; paid amount equals locked quote; paid date/time are valid; queue slot remains occupied; `IsVehicleAwaitingPickup` returns true; sidecar still contains exact ID |
| 9. Pickup exact-ID | wrong-ID release is rejected; wrong-ID rejection keeps pickup hold; exact repaired vehicle must remain at workshop; exact-ID release succeeds; release clears only that appointment; queue count reaches expected value |
| 10. Fleet/economy safety | pickup itself spends no cash; pickup itself performs no repair mutation; exact vehicle identity is unchanged; garage pickup hold source gate remains authoritative; hard TOW/IMMOBILE WORKSHOP HOLD remains separate; normal gameplay has no evidence-only acceleration |
| 11. Cross-system continuity | primary Farm Cargo active flag does not fabricate; cargo stage does not rewind; cargo bound vehicle ID does not transfer; cargo integrity does not improve; route timer does not rewind; baseline world/economy/vehicle state is restored after evidence |
| 12. Evidence/release gates | production priority marker count=1; production check-in marker count=1; production ready marker count=1; production pickup marker count=1; same-SHA JSON promotes technical gate 16→17; source CI never substitutes for Win64/runtime/visual acceptance |

## Expected evidence

A passing packaged candidate writes `WORKSHOP_PRIORITY_PICKUP_RUNTIME.json` with schema `gtt.workshop-priority-pickup-runtime.v1`, the exact candidate SHA, exact vehicle/decoy IDs, STANDARD and URGENT locked quotes, +20% surcharge, x0.80 service timing, one debit, persisted READY_FOR_PICKUP, wrong-ID rejection, exact pickup, no second charge, identity continuity and Farm Cargo authority continuity. `promote_demo_gate_workshop_priority_pickup.ps1` may promote only an already-PASS schema-16 gate for the same SHA to schema 17.

## Failure policy

Any missing marker, quote/timing mismatch, pre-charge, duplicate debit, lost pickup checkpoint, wrong-ID pickup success, second pickup charge, identity change, cargo-authority regression, schema/SHA mismatch or legacy bridge auto-release in the 0.1.52 window is a hard failure. A source-contract PASS is useful regression protection only; it is not a verified Windows demo.
