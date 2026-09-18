# GTT 0.1.41 — Garage / Workshop Recovery Integration Playtest

## Scope

This milestone connects the existing roadside tow, garage fleet and **existing `AGTTServiceTerminal` workshop** into one authoritative recovery loop. No parallel workshop subsystem is introduced. Tow remains a separate transport transaction; once the exact owned native road vehicle is physically beside the workshop terminal, the player uses the normal **E / Interact** flow to buy fuel-only service or a full repair/refuel/body service.

Source-contract CI is not packaged-runtime evidence. Native Chaos, Win64 packaging, packaged EXE smoke and demo visual acceptance remain separate open gates.

## Acceptance matrix — 68 scenarios

| # | Scenario | Expected result |
|---:|---|---|
| 1 | Owned damaged Rattleback beside existing workshop | Exact repair quote is shown. |
| 2 | Owned damaged Mulebox beside existing workshop | Exact repair quote is shown. |
| 3 | Healthy/full vehicle beside workshop | No paid service is applied. |
| 4 | Vehicle outside terminal radius | Service unavailable. |
| 5 | Unowned native road vehicle nearby | Ignored by native workshop selection. |
| 6 | Vehicle with empty persistent ID | Rejected from native paid service. |
| 7 | Native takeover inactive | Native paid service unavailable. |
| 8 | Wanted level 1 | Voluntary workshop service blocked. |
| 9 | Wanted level 2+ | Voluntary workshop service blocked. |
| 10 | Wanted clears | Existing workshop becomes usable again. |
| 11 | Mechanical-only damage | Existing breakdown repair estimate feeds quote. |
| 12 | Tire wear | Tire parts contribute to quote. |
| 13 | Missing fuel + mechanical damage | Fuel contributes to full-service quote. |
| 14 | Body damage | Body surcharge contributes to quote. |
| 15 | Detached panel | Structural/body surcharge contributes to quote. |
| 16 | Fuel-only visit | Exact per-litre fuel quote is used instead of full-service fee. |
| 17 | Fuel-only exact cash | One debit only. |
| 18 | Full-service exact cash | One debit only. |
| 19 | Insufficient repair cash | No service mutation. |
| 20 | Insufficient fuel cash | No refuel mutation. |
| 21 | Press E on existing terminal | `PurchaseNativeRoadWorkshopService` owns native transaction. |
| 22 | Full service succeeds | Condition reaches full. |
| 23 | Full service succeeds | Tires reach full integrity. |
| 24 | Full service succeeds | Fuel reaches native capacity. |
| 25 | Full service succeeds | Native body zones restore. |
| 26 | Full service succeeds | Detached panel count/mask clears. |
| 27 | Full service succeeds | `NeedsNativeWorkshopService()` becomes false. |
| 28 | Full service succeeds | Persistent vehicle ID remains unchanged. |
| 29 | Full service succeeds | Native cargo load factor remains unchanged. |
| 30 | Full service succeeds | Persistence mirror is flushed. |
| 31 | Full service succeeds | Existing GameMode primary save path is called. |
| 32 | Post-service identity mismatch | Migration/body state rolls back. |
| 33 | Post-service cargo mismatch | Migration/body state rolls back. |
| 34 | Post-service restoration mismatch | Migration/body state rolls back. |
| 35 | Full-service verification failure | Exact service charge is refunded. |
| 36 | Fuel verification failure | Fuel state rolls back and exact charge refunds. |
| 37 | Roadside tow completes | Existing tow destination remains WORKSHOP. |
| 38 | Roadside tow completes | Tow preserves ordinary damage. |
| 39 | Roadside tow completes | Tow preserves exact persistent vehicle ID. |
| 40 | Roadside tow completes | Tow remains `serviced=NO`. |
| 41 | Tow then E-interact | Workshop quote is separate from locked tow quote. |
| 42 | Tow then full service | Same towed actor is repaired, not a substitute. |
| 43 | Emergency patch then workshop | Full service can later restore remaining damage. |
| 44 | Emergency patch then workshop | Body damage left by patch is repaired only after paid service. |
| 45 | Police impound | Existing mandatory police safety-service path remains separate. |
| 46 | Active Wanted | Voluntary terminal service cannot bypass police recovery. |
| 47 | Active Farm Cargo + tow | Exact loaded Mulebox remains authoritative at workshop. |
| 48 | Active Farm Cargo + service | Persistent cargo vehicle ID is unchanged. |
| 49 | Active Farm Cargo + service | Native cargo load factor is unchanged. |
| 50 | Active Farm Cargo + service | Workshop source does not call Farm Cargo payout. |
| 51 | Active Farm Cargo + service | Workshop source does not complete a job. |
| 52 | Active Farm Cargo + service | Workshop source does not reset delivery timer. |
| 53 | Active Farm Cargo + service | Workshop source does not repair cargo integrity. |
| 54 | Decoy unowned Mulebox near terminal | Decoy is ignored. |
| 55 | Two owned vehicles in radius | Nearest active native road vehicle is selected deterministically. |
| 56 | Interaction prompt | Shows display name + exact persistent ID. |
| 57 | Interaction prompt | Shows exact fuel quote for fuel-only state. |
| 58 | Interaction prompt | Shows exact full-service quote for mechanical damage. |
| 59 | Service repeated after success | Healthy state prevents repeat billing. |
| 60 | Save after service | Existing GameMode save records synchronized mirror state. |
| 61 | Reload after normal primary save | Service result persists through existing fleet save. |
| 62 | Existing garage dashboard | Repair/tow estimates remain readable. |
| 63 | Existing garage recall | Recall still preserves damage; no free repair introduced. |
| 64 | Source verifier | Rejects any reintroduction of a parallel workshop subsystem. |
| 65 | Source verifier | Rejects missing rollback/refund/exact-ID/cargo guards. |
| 66 | SWIR presentation | Roadmap remains 125/130 = 96.2% with SVG-only meter. |
| 67 | Qualifying Win64 candidate (future) | Compile/cook/package + runtime workshop loop must pass. |
| 68 | Demo candidate (future) | Exact candidate workshop area/UI must pass rendered visual acceptance. |

## Manual play route

1. Start with an owned Rattleback or Mulebox and enough cash.
2. Damage condition/tires/body or drain fuel until service is needed.
3. If recovery is required, order a normal paid tow and wait for arrival.
4. Confirm tow places the same `PersistentVehicleId` at the workshop and does **not** repair ordinary damage.
5. Walk to the existing workshop terminal and use normal **E / Interact**. Confirm the prompt names the exact vehicle ID and quote.
6. Buy service. Verify exactly one debit, then check condition, tires, fuel, body zones and detached panels.
7. Repeat with fuel-only state; verify per-litre billing and no mechanical mutation.
8. Repeat with insufficient cash and Wanted; verify no vehicle mutation or hidden charge.
9. Repeat with active Farm Cargo; exact-vehicle authority and cargo load must survive while the real route still owns timer/integrity/payout.
10. Save after service, reload normally and verify fleet service state persists.

## Demo gate

0.1.41 is a gameplay/source milestone only. It does **not** close any of the five remaining roadmap gates and must not create a GitHub demo Release without a qualifying UE 5.8 Win64 build/package, packaged-EXE smoke, Native Chaos/runtime evidence and visual acceptance of the exact candidate.
