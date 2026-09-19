# GTT 0.1.54 — Civilian Incident Dispatch & Recovery Continuity playtest

Scope: source-built gameplay contract for disabled civilian incident dispatch, visible roadside marking, save/load rebind continuity, and ranger traffic-control priority.

Verification boundary: these cases are **source-contract verification**. They are **not a packaged Win64 runtime proof**. A qualifying Unreal Engine 5.8 Win64 compile/cook/package, packaged EXE smoke test, runtime evidence, and visual acceptance remain required before demo release.

Persistence rule: `GTT_CivilianIncident_01` persists a stable dispatch id, scene location, severity, and whether work had been in progress. It deliberately **does not persist an actor pointer**, helper identity, repair mutation, payout, or the six-second timer. On reload, an interrupted assist returns to Active and must rebind to a compatible disabled traffic actor near the stored location.

Economy rule: the dispatch subsystem may surface player messages, but it owns no money. The existing `AGTTTrafficCarPawn::CompleteRoadsideAssistance()` repair-then-`AddCash` path remains the single payout authority.

| # | Area | Scenario | Expected result |
|---:|---|---|---|
| 1 | Dispatch creation | First disabled traffic car appears | A single civilian incident dispatch opens with a non-empty stable incident id. |
| 2 | Dispatch creation | Condition remains above disable threshold | No civilian roadside dispatch is created for a merely slowed/crashed-but-mobile car. |
| 3 | Dispatch creation | Disabled car severity is low | Dispatch records the real clamped incident severity instead of inventing a fixed value. |
| 4 | Dispatch creation | A second less-severe disabled car appears | Existing dispatch remains authoritative; the lower-severity incident does not replace it. |
| 5 | Dispatch creation | A second incident exceeds active severity by at least 0.20 | New severe incident may supersede the active dispatch and gets a new incident id. |
| 6 | Dispatch creation | Tracked car is the only disabled car | Repeated scans do not duplicate or re-open the same dispatch. |
| 7 | Dispatch creation | Disabled car already completed roadside assistance | Scanner ignores it and cannot issue a second paid helper job. |
| 8 | Dispatch creation | Dispatch opens | Sidecar `GTT_CivilianIncident_01` is written with schema 1 and dispatch facts only. |
| 9 | Presentation | New dispatch reaches player | Existing activity-message HUD channel announces ROADSIDE DISPATCH without a parallel HUD economy system. |
| 10 | Presentation | Tracked car is live | A world-space ROADSIDE SOS text marker is attached above the exact traffic vehicle. |
| 11 | Presentation | Player approaches marked vehicle | Presentation snapshot reports true distance in metres from viewer to saved/live scene. |
| 12 | Presentation | No compatible actor is currently bound | Snapshot says REACQUIRING SCENE instead of pretending the actor exists. |
| 13 | Presentation | Assist is active | Snapshot says ROADSIDE ASSIST and tells player to remain within 5 m for 6 seconds. |
| 14 | Presentation | Warden traffic control owns scene | Snapshot exposes WARDEN TRAFFIC CONTROL and the priority instruction. |
| 15 | Presentation | Dispatch closes | Dynamic world marker is destroyed; no orphan SOS marker remains. |
| 16 | Presentation | Subsystem is deinitialized | Dynamic marker is destroyed and active checkpoint is saved before teardown. |
| 17 | Assistance lifecycle | Player interacts with disabled marked vehicle | Existing `AGTTTrafficCarPawn` `BeginRoadsideAssistance` path remains the service authority. |
| 18 | Assistance lifecycle | Assist starts normally | Observer transitions dispatch Active -> AssistanceInProgress and saves that lifecycle. |
| 19 | Assistance lifecycle | Player leaves the 500 cm radius | Existing traffic pawn cancels assist; dispatch returns to Active on the next scan. |
| 20 | Assistance lifecycle | Player returns and restarts | Same dispatch can be serviced again; no reward is paid before completion. |
| 21 | Assistance lifecycle | Six-second assist completes | Existing traffic pawn repairs before paying; dispatch only observes the completed authority. |
| 22 | Assistance lifecycle | Successful repair makes car mobile | Dispatch closes after the same car reports completion/recovery. |
| 23 | Assistance lifecycle | Field repair is insufficient | Existing traffic pawn cancels; dispatch remains open and no close/reward is faked. |
| 24 | Assistance lifecycle | Civilian car is interacted with while healthy | Ambient traffic remains non-stealable; no new entry/theft path is added. |
| 25 | Economy | Dispatch opens | No cash is spent or awarded by the dispatch subsystem. |
| 26 | Economy | Dispatch is saved/loaded | No cash mutation occurs on persistence or rebind. |
| 27 | Economy | Assist is cancelled by distance | No cash mutation occurs. |
| 28 | Economy | Warden priority cancels assist | No cash mutation occurs. |
| 29 | Economy | Existing assist succeeds | Only `AGTTTrafficCarPawn` existing `AddCash` roadside payout authority can reward player. |
| 30 | Economy | Dispatch closes after success | Close message explicitly does not issue a duplicate reward. |
| 31 | Save/load | Save during active dispatch | Sidecar stores incident id, location, severity and lifecycle flag, but no actor pointer. |
| 32 | Save/load | Save during AssistanceInProgress | `bAssistanceWasInProgress` records history; helper identity and timer are not persisted. |
| 33 | Save/load | Load an in-progress checkpoint | Lifecycle safely resumes as Active, requiring a fresh 6-second assist. |
| 34 | Save/load | Load valid checkpoint before traffic actors exist | Dispatch remains unbound and searches instead of creating a fake vehicle. |
| 35 | Save/load | Compatible disabled car appears within 950 cm | Nearest compatible actor rebinds to the stable saved incident id. |
| 36 | Save/load | Candidate is too far from saved location | It is not rebound to the saved incident. |
| 37 | Save/load | Candidate severity is implausibly below saved severity | It is rejected by the compatibility guard. |
| 38 | Save/load | No compatible actor appears for 180 seconds | Saved dispatch expires and the sidecar is deleted cleanly. |
| 39 | Save/load | Corrupt/wrong-schema checkpoint loads | Checkpoint is rejected/deleted rather than treated as project truth. |
| 40 | Save/load | Resolved dispatch persists | Sidecar is deleted so the resolved incident cannot resurrect on next load. |
| 41 | Warden authority | Road stop is inactive | Civilian assist lifecycle is not blocked by ranger subsystem. |
| 42 | Warden authority | Road stop active but farther than 1400 cm | Civilian assist remains independent. |
| 43 | Warden authority | Road stop active within 1400 cm and civilian assist starts | Subsystem cancels voluntary assist within the scan interval before its 6-second completion. |
| 44 | Warden authority | Warden cancellation occurs | Player sees priority message; dispatch remains Active and no repair/payout happens. |
| 45 | Warden authority | Traffic-control scene later clears | Player can restart normal civilian assist using existing interaction path. |
| 46 | Warden authority | Ranger road-stop traffic response is active | No ranger state, search state, fine, or road-stop presentation is mutated by civilian dispatch. |
| 47 | Regression | Nearby traffic reacts to collision | 0.1.53 non-recursive incident fan-out remains in `AGTTTrafficCarPawn`. |
| 48 | Regression | Traffic car is disabled/assisting | Existing critical simulation-budget conditions remain unchanged. |
| 49 | Regression | Obstacle/stuck recovery runs on healthy traffic | Dispatch subsystem does not alter normal route driving or avoidance. |
| 50 | Regression | Owned-vehicle roadside recovery exists simultaneously | Civilian sidecar uses a separate `GTT_CivilianIncident_01` slot and does not replace owned-vehicle recovery. |
| 51 | Verification | Source verifier runs | Checks wiring, sidecar, ranger authority, one-way economy and persistence contracts deterministically. |
| 52 | Verification | Milestone CI runs on branch/PR | Runs 0.1.54 verifier plus 0.1.53 roadside regression verifier. |
| 53 | Verification | Source contract is green | Result is still not a packaged Win64 runtime proof; UE 5.8 package/runtime/visual gates remain required. |
| 54 | Verification | Roadmap review | No roadmap checkbox or percentage changes solely because source dispatch code/tests exist. |

## Release truth

- Roadmap completion is unchanged by this milestone unless an existing canonical checkbox is independently proven complete.
- The dynamic `ROADSIDE SOS` marker is a gameplay aid created from the same tracked live vehicle; it is not a screenshot or authored-art claim.
- A restored incident can be shown as `REACQUIRING SCENE` until a compatible disabled vehicle is found. The game must never imply that a saved actor pointer survived the load.
- Warden traffic control can pause/cancel the voluntary helper timer near the stop scene, but the civilian dispatch does not mutate ranger search/fine/road-stop state.
- Passing this matrix and its verifier is source evidence only, not a packaged Win64 runtime proof.
