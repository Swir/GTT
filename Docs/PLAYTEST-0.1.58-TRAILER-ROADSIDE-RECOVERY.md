# GTT 0.1.58 — Trailer Roadside Recovery & Suspension Playtest

## Scope

This milestone turns the heavy farm trailer's existing repair primitive into a player-facing, timed roadside service and fixes the axle setup so the wheel constraints actually have bounded vertical suspension travel. It does **not** claim authored skeletal trailer wheel assets, final hitch sockets, Native Chaos acceptance, a packaged Win64 runtime pass or public demo readiness.

## Preconditions

- Start a Heavy Haul contract with an owned usable Fieldmaster.
- Keep enough cash for at least one roadside repair.
- Damage the trailer by a meaningful impact, hitch overstress or wheel/axle failure.
- For loaded cases, pick up timber at North Wood Yard first.

## Acceptance matrix

1. **Roadworthy trailer** — interacting with an undamaged trailer reports that no field repair is needed and does not debit cash.
2. **Repair prompt** — a damaged trailer exposes an interaction prompt with the current dollar quote and deterministic service duration.
3. **Locked quote** — starting repair snapshots the displayed quote and duration; no cash is charged at request time.
4. **Timed work** — the player must remain within 500 cm while the trailer stays at or below 1.5 km/h and, if attached, hitch load stays at or below the safe service threshold.
5. **Move-away cancellation** — leaving the 500 cm service radius cancels the repair with no charge and no repair mutation.
6. **Movement cancellation** — moving the trailer during service cancels with no charge.
7. **Hitch-tension cancellation** — pulling against the hitch during service cancels with no charge.
8. **Impact cancellation** — a meaningful collision during service cancels with no charge.
9. **Manual cancellation** — interacting with the same trailer again while servicing cancels with no charge.
10. **Insufficient funds at start** — service does not begin and cash/vehicle state remain unchanged.
11. **Funds spent during service** — completion fails closed if the locked quote is no longer affordable; no repair is applied.
12. **Successful checkout** — the locked quote is debited once, then the production repair restores lost wheels and improves structure.
13. **Repair rollback** — if the repair target becomes invalid after checkout starts, the same quote is refunded.
14. **Cargo preservation** — roadside service does not restore lost cargo integrity; damaged timber stays damaged.
15. **Workshop distinction** — roadside repair caps structural recovery below perfect condition; it is not a free substitute for full workshop service.
16. **Escalating repeated repairs** — each completed field repair increases the next quote using the existing heavy-haul escalation model.
17. **Contract time consequence** — each completed trailer field repair removes 22 seconds from the Heavy Haul delivery clock exactly once.
18. **No cancellation penalty** — cancelled/failed repair attempts do not increment the completed-repair count or remove 22 seconds.
19. **Heavy-haul objective HUD** — while damaged, the objective exposes field-repair availability/quote; while servicing, it exposes remaining service time.
20. **Re-hitch continuity** — a repair that temporarily releases an attached trailer restores the hitch constraint and attempts to reconnect the same tow vehicle.
21. **Wheel restoration** — a lost left or right wheel is physically reset to its axle home transform and the breakable wheel constraint is rebuilt.
22. **Hitch reuse** — a trailer whose hitch previously broke can be repaired and later attached again through the normal hitch API.
23. **Empty suspension** — unloaded wheel constraints visibly move within bounded vertical travel rather than being Z-locked.
24. **Loaded suspension** — loading timber increases body mass and retunes the wheel spring/damping pair for the heavier state.
25. **No lateral axle slop** — wheel X/Y travel remains locked while Z travel is bounded.
26. **Wheel spin preserved** — axle twist remains free so wheels can rotate while suspension travel is active.
27. **Breakability preserved** — wheel linear/angular break thresholds remain enabled after suspension configuration and roadside restoration.
28. **Damage presentation** — fender/tailgate/reflector/cargo-shift presentation still reflects authoritative trailer/cargo damage.
29. **Cargo handling regression** — instability, excessive speed and a lost axle still reduce cargo integrity under load.
30. **Hitch failure regression** — excessive tow separation still routes through the existing authoritative detach path and damages the trailer.
31. **Native hitch regression** — native Fieldmaster attachment still requires runtime readiness and the validated rear-hitch transform.
32. **Legacy hitch regression** — legacy Fieldmaster fallback still attaches through the physical root component path.
33. **Contract completion regression** — cargo/trailer/tow-vehicle condition still influence final heavy-haul payout.
34. **Contract failure regression** — delivery-window expiry still fails/reset the contract.
35. **Reset regression** — starting/resetting a contract cancels any in-flight service, restores base trailer state and clears completed repair escalation.
36. **Economy authority** — only `UGTTPlayerEconomyComponent` performs the roadside debit/refund/message flow.
37. **No duplicate repair authority** — Heavy Haul delegates repair start to `AGTTFarmTrailer`; it does not independently charge or mutate trailer damage.
38. **Roadmap truth** — checklist remains exactly 125/130 (96.2%); none of the five authored/Win64/Native Chaos evidence gates close.
39. **SWIR presentation** — README/roadmap continue to use repository SVG progress assets only; no live ASCII/Unicode meter returns.
40. **Release honesty** — no demo Release is created without a real UE 5.8 Win64 package, packaged-EXE smoke, green relevant Actions and visual acceptance.

## Runtime evidence still required

A qualifying Windows runner must later prove the same service flow inside the packaged executable together with wheel suspension behavior, trailer re-hitching, heavy-haul timing/economy and visual quality. Source-contract PASS alone is not packaged runtime evidence.
