# GTT 0.1.56 — Responder Safety Corridor Playtest

> Verification boundary: this matrix supports **source-contract verification** and future manual/package testing. It is **not a packaged Win64 runtime proof**. The safety corridor is **cooldown-limited**, preserves **warden priority**, owns **no economy authority**, and **roadmap completion is unchanged**.

| # | Area | Scenario | Expected result |
|---:|---|---|---|
| 1 | Dispatch | Severe incident remains unresolved through player-first grace | 0.1.55 responder is dispatched |
| 2 | Arrival | Responder reaches arrival radius | Responder parks and enters on-scene state |
| 3 | Visual | Responder is still en route | Safety cones remain hidden |
| 4 | Visual | Responder parks at scene | Four safety cones become visible |
| 5 | Visual | Restored on-scene responder loads | Cones are visible immediately |
| 6 | Visual | Responder is destroyed after recovery | Attached cones disappear with responder |
| 7 | Visual | On-scene service label | Label reads ROAD SERVICE - SAFE CORRIDOR |
| 8 | Visual | En-route service label | Label remains ROAD SERVICE |
| 9 | Corridor | Ambient car enters 1,800 cm outer radius | Car receives existing nearby-incident yield pulse |
| 10 | Corridor | Ambient car is outside 1,800 cm | No safety pulse is sent |
| 11 | Corridor | Ambient car is inside 320 cm recovery pocket | No extra pulse is sent |
| 12 | Corridor | Same car remains in corridor under 4.5 s | No duplicate pulse is sent |
| 13 | Corridor | Same car remains after cooldown expires | A new bounded pulse may be sent |
| 14 | Corridor | Multiple ambient cars enter together | Each eligible car is handled independently |
| 15 | Corridor | Traffic actor is destroyed after yield | Weak history entry is pruned safely |
| 16 | Corridor | No responder exists | Corridor reports inactive and yields zero cars |
| 17 | Authority | Incident vehicle is disabled | Safety subsystem does not pulse the disabled authority vehicle |
| 18 | Authority | Player roadside assistance is active | Safety subsystem skips that vehicle |
| 19 | Authority | County responder scene authority is active | Safety subsystem skips that vehicle |
| 20 | Authority | Ranger stop owns a traffic car | Safety subsystem skips ranger-controlled traffic |
| 21 | Authority | Warden takes overlapping scene | 0.1.55 responder yields/cancels; corridor ends naturally |
| 22 | Authority | Corridor scans scene | It never opens or resolves civilian dispatch |
| 23 | Economy | Corridor activates | No cash is added or spent |
| 24 | Economy | Corridor ends | No payout, fee or fine is generated |
| 25 | Repair | Ambient car yields | Safety subsystem does not repair or damage it |
| 26 | Wanted | Corridor activates near player | Wanted level is not changed |
| 27 | Player assist | Player helps during grace | Existing 0.1.53 payout authority remains player-owned |
| 28 | Responder | Responder takes scene after grace | Existing 0.1.55 recovery remains no-payout |
| 29 | Traffic | Yield pulse reaches traffic pawn | Existing ReactToNearbyIncident path owns stop/steer response |
| 30 | Traffic | Yield pulse is applied | Existing obstacle/stuck logic remains available afterward |
| 31 | Traffic | Yielding car also sees obstacle | Existing obstacle avoidance remains authoritative |
| 32 | Traffic | Yielding car reaches route point later | Route progression still continues after pulse expires |
| 33 | Performance | Corridor has no eligible cars | No per-car mutation occurs |
| 34 | Performance | Corridor has many traffic cars | Scan remains bounded to normal world traffic iterator cadence |
| 35 | Performance | Tick runs below 0.75 s accumulated time | No corridor scan occurs yet |
| 36 | Performance | Scan interval elapses | One bounded scan is executed |
| 37 | Recovery | Responder completes civilian recovery | 0.1.55 removes scene authority and responder actor |
| 38 | Recovery | Responder actor is gone | Safety corridor deactivates and per-scene cooldown history is cleared on next scan |
| 39 | Save/load | Responder checkpoint restores EnRoute | Cones stay hidden until physical arrival |
| 40 | Save/load | Responder checkpoint restores OnScene | Cones deploy and corridor resumes from live actor state |
| 41 | Cleanup | Dispatch changes to another incident | Old responder/corridor ownership and yield cooldown history are cleared |
| 42 | Cleanup | World deinitializes | No persistent actor pointer is introduced by safety subsystem |
| 43 | Regression | Run 0.1.55 verifier | Responder handoff contract stays green |
| 44 | Regression | Run 0.1.54 verifier | Authoritative dispatch contract stays green |
| 45 | Regression | Inspect safety source for repair/economy calls | None are present |
| 46 | Regression | Inspect responder ownership | Service vehicle remains illegal to take / non-enterable |
| 47 | Release gate | Source CI passes | Win64/demo readiness is still not claimed |
| 48 | Roadmap | Milestone lands without closing Native Chaos/Win64 gates | Roadmap remains 125/130 (96.2%) |
| 49 | Lifecycle | A new responder replaces a previous physical scene | Old per-car cooldown history is reset before the new scene starts yielding |
| 50 | Lifecycle | Active responder disappears after recovery/cancel | Corridor logs closure and clears all per-scene cooldown history |
| 51 | Lifecycle | Many incidents complete while the same traffic cars survive | Completed scenes do not accumulate stale cooldown entries across incidents |
| 52 | Lifecycle | Same ambient car reaches a fresh scene inside 4.5 s of an old scene | New responder can yield it immediately; old scene cooldown never suppresses fresh safety authority |
