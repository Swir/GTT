# GTT 0.1.55 — Civilian Incident Responder Handoff & Rural Emergency Scene playtest

Scope: source-built gameplay contract for severe civilian incident escalation into a physical county road-service response while preserving the authoritative 0.1.54 dispatch, existing 0.1.53 player roadside payout, and ranger/game-warden traffic-control priority.

Verification boundary: these cases are **source-contract verification**. They are **not a packaged Win64 runtime proof**. A qualifying Unreal Engine 5.8 Win64 compile/cook/package, packaged EXE smoke test, runtime evidence and visual acceptance remain required before demo release.

Authority rule: the 0.1.54 civilian dispatch remains the only incident owner. The responder layer may stage a physical service vehicle and ask `AGTTTrafficCarPawn` to perform a no-payout recovery, but it may not open/resolve incidents, award/spend cash, change Wanted, or mutate ranger state.

Player-first grace: a severe unresolved incident gives the player an 18-second chance to begin the existing paid six-second roadside assistance before county road service commits. Once the responder is physically on scene, the responder owns that recovery scene and there is **no player reward** for that incident handoff.

| # | Area | Scenario | Expected result |
|---:|---|---|---|
| 1 | Eligibility | Dispatch severity below 0.72 | No county responder is requested. |
| 2 | Eligibility | Severity is exactly 0.72 | Severe-incident responder timer is eligible to start. |
| 3 | Eligibility | Severe dispatch opens | Responder tracks the exact authoritative dispatch incident id. |
| 4 | Eligibility | Different dispatch supersedes active one | Old responder state is discarded and new incident gets fresh grace. |
| 5 | Grace | Severe incident remains unresolved for 5 seconds | No responder yet; player keeps first opportunity. |
| 6 | Grace | Severe incident reaches 17.9 seconds | No responder yet. |
| 7 | Grace | Severe incident reaches 18 seconds | County road-service responder can be dispatched. |
| 8 | Grace | Player starts normal assist before 18 seconds | Responder commitment is cancelled/reset. |
| 9 | Grace | Player finishes normal assist | Existing 0.1.53 repair/payout closes the incident; responder never pays. |
| 10 | Grace | Player begins then leaves scene | Existing assist cancels; responder grace restarts rather than stealing progress. |
| 11 | Physical responder | Dispatch commits after grace | One `AGTTRoadsideResponderVehicle` is spawned for the tracked incident. |
| 12 | Physical responder | Service vehicle spawns | It uses project/runtime basic geometry, not protected third-party vehicle assets. |
| 13 | Physical responder | Service vehicle is en route | Physics force/torque moves it toward the saved incident scene. |
| 14 | Physical responder | Service vehicle is far away | It remains EN_ROUTE and does not take scene authority early. |
| 15 | Physical responder | Service vehicle reaches arrival radius | Phase becomes ON_SCENE. |
| 16 | Physical responder | Vehicle reaches scene | Cyan ROAD SERVICE presentation and alternating beacons remain visible. |
| 17 | Physical responder | Player interacts with service vehicle | Service-owned responder cannot be entered or stolen. |
| 18 | Physical responder | Responder is destroyed unexpectedly en route | Subsystem can recreate it for the same committed incident. |
| 19 | Handoff | Responder parks on scene | Exact tracked traffic vehicle receives responder scene authority. |
| 20 | Handoff | Scene authority starts | Civilian interaction reports recovery in progress instead of starting player assist. |
| 21 | Handoff | Scene authority active | Player roadside payout path is unavailable for that incident. |
| 22 | Handoff | Seven-second scene hold elapses | Traffic pawn executes responder recovery. |
| 23 | Handoff | Responder recovery succeeds | Vehicle becomes mobile and keeps a temporary limp interval. |
| 24 | Handoff | Responder recovery succeeds | Responder subsystem issues no player cash/reward. |
| 25 | Handoff | Recovery mutation is insufficient | Scene stays owned and retries instead of faking success. |
| 26 | Handoff | Vehicle becomes mobile | Authoritative 0.1.54 dispatch observes recovery and closes normally. |
| 27 | Handoff | Responder completes | Physical responder is destroyed after handoff. |
| 28 | Handoff | New collision happens later | New incident can start cleanly; old responder authority is not sticky. |
| 29 | Economy | Responder dispatch begins | No cash is added or spent. |
| 30 | Economy | Responder reaches scene | No cash is added or spent. |
| 31 | Economy | Responder recovery completes | No player reward is issued. |
| 32 | Economy | Player wins grace race and completes assist | Existing severity-scaled $65–$110 payout remains unchanged. |
| 33 | Economy | Save/load occurs during responder flow | Persistence does not change cash. |
| 34 | Wanted | Responder flow runs | No Wanted heat is added or cleared by responder subsystem. |
| 35 | Warden traffic control | Warden control overlaps during grace | Grace pauses/resets and responder does not take scene. |
| 36 | Warden traffic control | Warden control begins while responder en route | Responder yields/cancels and ranger authority remains untouched. |
| 37 | Warden traffic control | Warden control begins while responder on scene | Responder scene authority is cleared before further recovery. |
| 38 | Warden traffic control | Warden scene later clears | Severe incident may restart player-first grace. |
| 39 | Warden traffic control | Ranger search/fine state exists | Civilian responder never mutates ranger state. |
| 40 | Persistence | Grace is partly elapsed then game saves | `GTT_CivilianResponder_01` stores exact incident id and bounded grace elapsed. |
| 41 | Persistence | Save while responder en route | Sidecar stores EN_ROUTE phase without actor pointer. |
| 42 | Persistence | Save while responder on scene | Sidecar stores ON_SCENE and bounded hold remaining. |
| 43 | Persistence | Load before authoritative dispatch is ready | Responder waits instead of inventing an incident actor. |
| 44 | Persistence | Matching dispatch appears after load | Saved responder state reconnects only to that exact incident id. |
| 45 | Persistence | Loaded phase was EN_ROUTE | Physical responder is recreated en route. |
| 46 | Persistence | Loaded phase was ON_SCENE | Physical responder is recreated at scene and traffic authority is restored. |
| 47 | Persistence | Sidecar schema is invalid | Sidecar is rejected/deleted. |
| 48 | Persistence | Authoritative dispatch disappears | Responder state expires and sidecar is removed. |
| 49 | Persistence | Dispatch id changes after load | Stale responder state cannot attach to the new incident. |
| 50 | Persistence | Recovery completed | Sidecar is cleared so completed handoff cannot resurrect. |
| 51 | Traffic regression | Healthy ambient traffic drives normal route | Responder feature does not change ordinary route driving. |
| 52 | Traffic regression | Nearby crash warning occurs | Existing bounded non-recursive incident reaction remains. |
| 53 | Traffic regression | Player assist is active | Traffic remains simulation-critical as before. |
| 54 | Traffic regression | Responder scene authority is active | Traffic also remains simulation-critical during handoff. |
| 55 | Traffic regression | Ambient traffic is healthy | Base entry/theft path remains unavailable. |
| 56 | Dispatch regression | 0.1.54 ROADSIDE SOS dispatch is active | Existing dispatch marker/lifecycle remains authoritative. |
| 57 | Dispatch regression | Responder finishes recovery | Dispatch closes because tracked vehicle recovered, not because responder edits dispatch state. |
| 58 | Verification | 0.1.55 verifier runs | Checks physical responder, authority split, persistence and no-payout contract. |
| 59 | Verification | Historical 0.1.54/0.1.53 verifiers rerun | Dispatch and player payout invariants remain green. |
| 60 | Verification | Roadmap review | Roadmap completion is unchanged; source responder code closes no Native Chaos/trailer/Win64 gate. |

## Release truth

- Roadmap completion is unchanged by this milestone unless an existing canonical checkbox is independently proven complete.
- The responder vehicle and beacons are runtime/project presentation for gameplay, not proof of final authored vehicle art or visual demo acceptance.
- Passing this source-contract matrix does not prove Unreal Engine 5.8 compilation or packaged behavior.
- Demo publication still requires the exact Win64 packaged candidate, runtime smoke/evidence, green release gates and rendered visual acceptance.
