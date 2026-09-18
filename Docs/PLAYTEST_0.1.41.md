# GTT 0.1.41 — Garage / Workshop Recovery Integration Playtest

## Scope

This milestone closes the gameplay gap between **paid roadside tow** and **actual player-authorized workshop service**. Tow remains a separate transport transaction. Once the exact owned native vehicle is physically at the workshop, the player can purchase one full service quote that restores mechanical condition, tires, fuel and native body damage while preserving the same persistent vehicle identity and any active Farm Cargo load binding.

Source-contract CI is not packaged-runtime evidence. Native Chaos, Win64 packaging, packaged EXE smoke and demo visual acceptance remain separate open gates.

## Acceptance matrix

| # | Scenario | Expected result |
|---:|---|---|
| 1 | Owned damaged Rattleback at workshop | Service quote available. |
| 2 | Owned damaged Mulebox at workshop | Service quote available. |
| 3 | Healthy full vehicle at workshop | No paid service offered. |
| 4 | Vehicle outside workshop radius | Service unavailable. |
| 5 | Vehicle moving above workshop speed limit | Service unavailable. |
| 6 | Player too far from workshop vehicle | Service unavailable. |
| 7 | Unowned native vehicle at workshop | Service unavailable. |
| 8 | Vehicle with missing persistent ID | Service unavailable. |
| 9 | Native takeover inactive | Service unavailable. |
| 10 | Wanted level 1 | Voluntary workshop service blocked. |
| 11 | Wanted level 2+ | Voluntary workshop service blocked. |
| 12 | Wanted clears | Service becomes available without vehicle recreation. |
| 13 | Quote with mechanical damage | Mechanical labor contributes to price. |
| 14 | Quote with tire wear | Tire parts contribute to price. |
| 15 | Quote with missing fuel | Fuel contributes to price. |
| 16 | Quote with body damage | Body surcharge contributes to price. |
| 17 | Quote with detached panels | Detachment surcharge contributes to price. |
| 18 | Mixed severe damage | Quote remains bounded by production estimate. |
| 19 | Press H near eligible vehicle | Exactly one purchase attempt occurs. |
| 20 | Insufficient cash | No debit and no vehicle mutation. |
| 21 | Exact cash balance | Service may complete and balance reaches expected value. |
| 22 | Sufficient cash | Exactly one `SpendCash` transaction occurs. |
| 23 | Service succeeds | Condition reaches full. |
| 24 | Service succeeds | Tires reach full integrity. |
| 25 | Service succeeds | Fuel reaches native capacity. |
| 26 | Service succeeds | Native body zones restore. |
| 27 | Service succeeds | Detached panel count/mask clears. |
| 28 | Service succeeds | `NeedsNativeWorkshopService()` becomes false. |
| 29 | Service succeeds | Persistent vehicle ID is unchanged. |
| 30 | Service succeeds | Fleet persistence mirror is flushed. |
| 31 | Service post-check fails | Migration snapshot rolls back. |
| 32 | Service post-check fails | Body snapshot and detached mask roll back. |
| 33 | Service post-check fails | Cargo load factor rolls back. |
| 34 | Service post-check fails | Exact charge is refunded. |
| 35 | Service post-check fails | Failure is logged as FAIL, not PASS. |
| 36 | Roadside tow completes | Vehicle appears inside workshop service radius. |
| 37 | Roadside tow completes | Tow damage remains before service purchase. |
| 38 | Roadside tow completes | Tow charge remains separate from workshop quote. |
| 39 | Tow then service | Tow cannot silently auto-charge workshop service. |
| 40 | Emergency patch then workshop | Workshop can later perform full service. |
| 41 | Emergency patch then workshop | Body damage left by patch is repaired only after paid workshop service. |
| 42 | Police impound | Existing police safety-service path remains separate. |
| 43 | Police impound | H purchase path does not execute during active Wanted. |
| 44 | Active Farm Cargo + tow | Exact loaded Mulebox remains the same vehicle at workshop. |
| 45 | Active Farm Cargo + service | Persistent cargo vehicle ID does not change. |
| 46 | Active Farm Cargo + service | Cargo load factor is preserved. |
| 47 | Active Farm Cargo + service | No cargo payout occurs from workshop code. |
| 48 | Active Farm Cargo + service | No job completion occurs from workshop code. |
| 49 | Active Farm Cargo + service | Delivery timer is not reset by workshop code. |
| 50 | Active Farm Cargo + service | Cargo integrity is not repaired by workshop code. |
| 51 | Decoy Mulebox near workshop | Closest owned eligible exact actor is selected only by physical proximity. |
| 52 | Decoy is unowned | Decoy is ignored. |
| 53 | Vehicle identity mutates unexpectedly during service | Post-check fails and transaction rolls back/refunds. |
| 54 | Cargo factor mutates unexpectedly during service | Post-check fails and transaction rolls back/refunds. |
| 55 | H pressed repeatedly after success | Healthy vehicle prevents repeat billing. |
| 56 | Service prompt | Shows exact persistent ID and current full-service quote. |
| 57 | Service prompt | Explicitly states tow charge is separate. |
| 58 | Prompt cooldown | No per-frame message spam. |
| 59 | Save after successful service | Legacy persistence mirror contains restored service state. |
| 60 | Reload after normal primary save | Serviced condition/fuel/tire state remains restored. |
| 61 | Source verifier | Rejects missing ownership/exact-ID/Wanted guards. |
| 62 | Source verifier | Rejects missing rollback/refund verification. |
| 63 | Source verifier | Rejects cargo-authority mutation from workshop source. |
| 64 | SWIR presentation verifier | Roadmap remains 125/130 = 96.2%, SVG-only. |
| 65 | Qualifying Win64 candidate (future gate) | Compile/cook/package must succeed. |
| 66 | Packaged candidate (future gate) | H workshop service must be exercised in runtime smoke. |
| 67 | Packaged candidate (future gate) | Tow → workshop → paid service must preserve exact vehicle ID. |
| 68 | Demo candidate (future gate) | Visual workshop area/HUD must pass human acceptance before Release. |

## Manual play route

1. Start with an owned Rattleback or Mulebox and enough cash.
2. Damage condition/tires/body or drain fuel until workshop service is needed.
3. If recovery is required, order a normal paid tow and wait for arrival.
4. Confirm tow places the same `PersistentVehicleId` at the workshop and does not repair ordinary damage.
5. Stand near the vehicle. Confirm the workshop prompt shows the exact vehicle ID and one full-service quote.
6. Press **H**. Verify one debit only, then check condition, tires, fuel, body zones and detached panels.
7. Repeat with insufficient cash; verify no vehicle state changes.
8. Repeat during Wanted; verify the voluntary workshop is blocked.
9. Repeat with active Farm Cargo. Confirm exact-vehicle authority and cargo load survive service and the route still must finish through its real terminals.
10. Save after service, reload normally and verify the serviced fleet state persists through the existing save path.

## Demo gate

0.1.41 is a gameplay/source milestone only. It does **not** close any of the five remaining roadmap gates and must not create a GitHub demo Release without a qualifying UE 5.8 Win64 build/package, packaged-EXE smoke, Native Chaos/runtime evidence and visual acceptance of the exact candidate.
