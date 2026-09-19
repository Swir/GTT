# GTT 0.1.57 — Scene Clearance & Lane Reopening Continuity playtest

Scope: source-built gameplay contract for post-recovery roadside scene handoff, controlled lane reopening, and save/load continuity while preserving existing dispatch, ranger and economy authorities.

Verification boundary: these cases are **source-contract verification**. They are **not a packaged Win64 runtime proof**. Unreal Engine 5.8 Win64 compile/cook/package, packaged EXE smoke, runtime evidence and rendered visual acceptance remain required before demo release.

Authority rule: the clearance phase may preserve and taper roadside presentation only. It owns **no economy authority**, repair authority, Wanted state, ranger/game-warden state, mission payout, or civilian-dispatch open/close authority. Roadmap completion is unchanged.

| # | Area | Scenario | Expected result |
|---:|---|---|---|
| 1 | Recovery handoff | Responder recovery succeeds | Phase becomes ClearingScene instead of destroying the responder immediately. |
| 2 | Recovery handoff | Responder recovery succeeds | Player receives no reward from county service. |
| 3 | Recovery handoff | Traffic pawn exits responder authority | Recovered civilian can resume its existing limp return-to-route behavior. |
| 4 | Recovery handoff | ClearingScene begins | Scene hold timer is zeroed before lane reopening. |
| 5 | Recovery handoff | ClearingScene begins | Nine-second clearance timer is initialized. |
| 6 | Presentation | ClearingScene begins | Service label changes to ROAD SERVICE - LANE REOPENING. |
| 7 | Presentation | ClearingScene begins | Front cone pair is hidden. |
| 8 | Presentation | ClearingScene begins | Rear cone pair stays visible. |
| 9 | Presentation | Responder is OnScene before recovery | All four safety cones remain available. |
| 10 | Presentation | Responder is EnRoute | Safety corridor presentation remains undeployed. |
| 11 | Traffic reopening | Active recovery corridor | Outer yield radius remains 1,800 cm. |
| 12 | Traffic reopening | ClearingScene corridor | Outer yield radius contracts to 980 cm. |
| 13 | Traffic reopening | ClearingScene corridor | Yield severity drops from 0.38 to 0.20. |
| 14 | Traffic reopening | Car is inside 320 cm pocket | No reopening pulse is applied. |
| 15 | Traffic reopening | Same car was pulsed under 4.5 s ago | No duplicate reopening pulse is applied. |
| 16 | Traffic reopening | Same car remains after cooldown | A new bounded reopening pulse may be applied. |
| 17 | Traffic reopening | Disabled civilian car is scanned | Safety layer skips it. |
| 18 | Traffic reopening | Player roadside assistance is active | Safety layer skips it. |
| 19 | Traffic reopening | Responder scene authority is active | Safety layer skips that car. |
| 20 | Traffic reopening | Ranger stop owns car | Safety layer skips ranger-controlled traffic. |
| 21 | Authority | ClearingScene runs | No cash is added or spent. |
| 22 | Authority | ClearingScene runs | No vehicle repair call is owned by the safety layer. |
| 23 | Authority | ClearingScene runs | No Wanted heat is added or cleared. |
| 24 | Authority | ClearingScene runs | No ranger fine/search/road-stop state is mutated. |
| 25 | Authority | ClearingScene runs | No civilian dispatch is opened or resolved by clearance code. |
| 26 | Authority | Player completed roadside assist before county takeover | Existing 0.1.53 payout path remains unchanged. |
| 27 | Authority | County recovery completed | Existing 0.1.55 recovery remains no-payout. |
| 28 | Continuity | Save during ClearingScene | Schema 2 stores ClearingScene phase. |
| 29 | Continuity | Save during ClearingScene | Remaining clearance seconds are stored. |
| 30 | Continuity | Save during ClearingScene | Scene location is stored. |
| 31 | Continuity | Reload schema 2 ClearingScene | Responder is reconstructed at the saved scene. |
| 32 | Continuity | Reload schema 2 ClearingScene | Responder resumes LANE REOPENING presentation. |
| 33 | Continuity | Reload schema 2 ClearingScene | Repair is not replayed. |
| 34 | Continuity | Reload schema 2 ClearingScene | Payout is not replayed. |
| 35 | Continuity | Reload schema 1 EnRoute | Legacy save remains accepted. |
| 36 | Continuity | Reload schema 1 OnScene | Legacy save remains accepted. |
| 37 | Continuity | Reload schema 1 with phase beyond OnScene | Checkpoint fails closed. |
| 38 | Continuity | Reload schema 2 with phase beyond ClearingScene | Checkpoint fails closed. |
| 39 | Continuity | Wrong save schema | Checkpoint is deleted/fails closed. |
| 40 | Continuity | Tracked incident already closed while ClearingScene runs | Clearance keeps progressing instead of cancelling immediately. |
| 41 | Continuity | New dispatch appears while old ClearingScene runs | Old scene clearance remains authoritative until completion. |
| 42 | Continuity | Responder actor disappears during ClearingScene | Subsystem recreates it from saved scene location. |
| 43 | Continuity | Responder recreation fails temporarily | Clearance timer does not falsely complete without a responder. |
| 44 | Continuity | Clearance timer reaches zero | Responder actor is destroyed. |
| 45 | Continuity | Clearance timer reaches zero | Responder sidecar is cleared. |
| 46 | Continuity | Clearance timer reaches zero | Phase returns to None. |
| 47 | Continuity | Dispatch later disappears after completed recovery | Stale tracked id is cleared without creating a new responder. |
| 48 | Lifecycle | Deinitialize during ClearingScene | Schema-2 checkpoint is saved. |
| 49 | Lifecycle | Deinitialize during EnRoute | Existing responder checkpoint behavior remains. |
| 50 | Lifecycle | Deinitialize during OnScene | Existing responder checkpoint behavior remains. |
| 51 | Lifecycle | Cancellation before recovery | Clearance timer and scene location are reset. |
| 52 | Lifecycle | Warden takes priority before recovery | Existing cancellation/yield path stays authoritative. |
| 53 | Lifecycle | Warden state overlaps after recovery | Clearance owns no ranger state and simply completes its bounded teardown. |
| 54 | Regression | Run 0.1.56 verifier | Safety-corridor source contract stays green. |
| 55 | Regression | Run 0.1.55 verifier | Responder handoff and no-payout contract stay green. |
| 56 | Regression | Run 0.1.54 verifier | Civilian dispatch authority stays green. |
| 57 | Regression | Healthy ambient traffic outside scene | Normal route driving is unchanged. |
| 58 | Regression | Traffic sees ordinary obstacle | Existing obstacle avoidance remains authoritative. |
| 59 | Regression | Traffic gets stuck after reopening | Existing stuck recovery remains available. |
| 60 | Regression | Service vehicle interaction | Responder remains non-enterable/service-owned. |
| 61 | Release gate | Source CI passes | Win64/demo readiness is still not claimed. |
| 62 | Roadmap | 0.1.57 source milestone lands | Roadmap remains 125/130 unless one of the five canonical blockers is independently proven. |
| 63 | Visual truth | Runtime basic-shape responder remains | This is not final authored vehicle-art acceptance. |
| 64 | Scope | Clearance phase ends | No persistent responder or traffic-yield state survives beyond the bounded scene. |

## Release truth

- Roadmap completion remains 125/130 (96.2%) unless one of the five canonical Native Chaos/trailer/Win64 blockers is independently proven complete.
- `ClearingScene` is a bounded presentation/lifecycle phase after the existing no-payout county recovery; it must never replay repair or player reward.
- The narrower reopening corridor is source gameplay behavior, not proof of final visual quality.
- Demo publication still requires the exact verified Win64 packaged candidate, packaged-EXE smoke, green relevant Actions and rendered visual acceptance.
