# GTT 0.0.96 Playtest — Player-Selectable Recovery & Damage HUD

## Goal
Verify that a Native road-vehicle breakdown creates a real player decision, that the HUD communicates it cleanly, and that tow transport plus workshop repair are two separate paid actions using the same authoritative damage state.

## Manual gameplay route
1. Own a Native Chaos Rattleback 82 or Mulebox 1200 and drive until significant damage, tire failure, fuel starvation or another recovery-eligible breakdown occurs.
2. Confirm the lower-left vehicle panel shows real condition/tire/body state plus a concise `RECOVERY` row with recommendation, tow quote and repair estimate. It must not become a debug telemetry wall.
3. At wanted 0, do nothing for at least eight seconds. The car must remain with the player: no automatic civilian tow is allowed.
4. If the assessment permits limp-home driving, attempt to reach the workshop manually and confirm no tow fee is charged.
5. Repeat with an immobilized vehicle. Press `T` or gamepad D-Pad Up for the manual tow decision. Confirm the quoted tow is explicitly requested and paid, then confirm the vehicle is delivered near the workshop with its damage still present.
6. Interact with the workshop separately. Confirm a second, separately quoted repair payment is charged and condition/tires/body state are restored.
7. Repeat while wanted 1: civilian tow must be blocked. Repeat while wanted 2+: police impound may take custody automatically and retain its mandatory safety-service/fine behavior.

## Deterministic packaged evidence
The `Win64 recovery-choice evidence` workflow packages version 0.0.96 on a real self-hosted UE 5.8 runner and keeps the executable alive long enough for the earlier wanted/spike/save-load/structural route plus the new recovery sequence. Before the voluntary-recovery phase it deterministically clears any residual wanted state from the earlier wanted-4 scenario and zeros Native vehicle velocity, so the test cannot accidentally pass through police impound or fail because of residual momentum. Runtime must emit all of:
- `DEMO_SCENARIO_RECOVERY_OFFER ... result=PASS`
- `DEMO_SCENARIO_RECOVERY_CHOICE ... auto_tow=NO` after at least 8 seconds
- `DEMO_SCENARIO_PLAYER_TOW ... requested=YES ... damage_preserved=YES serviced=NO`
- `DEMO_SCENARIO_SEPARATE_REPAIR ... result=PASS`
- `DEMO_SCENARIO_RECOVERY_CHOICE_COMPLETE result=PASS ... route=stranded-offer-manual-tow-paid-repair`

`Scripts/evaluate_recovery_choice.ps1` must create `RECOVERY_CHOICE.json` with schema `gtt.recovery-choice.v1` and reject price/cash-delta mismatches, auto-tow behavior, tow-side repair, incomplete workshop restoration or SHA drift. `evaluate_demo_candidate.ps1` must require this manifest for version 0.0.96.

## Regression checks
- The existing 33-step `gtt.demo-scenario.v11` structural limp-home route remains mandatory.
- Tow must preserve damage; workshop must still calculate and charge the real repair estimate.
- Police impound semantics remain distinct from voluntary roadside assistance.
- Legacy HUD objective/wanted/ranger/radio/combat presentation must remain contextual.
- `Docs/ROADMAP.md` must retain `SWIR-ROADMAP-STANDARD:v1` and remain 125/130 = 96.2% unless a genuine acceptance checkbox is completed.

## Demo-release decision
Do **not** publish a demo from source checks alone. Release still requires a verified UE 5.8 Win64 package, successful packaged EXE runtime smoke including this recovery route, green relevant Actions, no demo-critical blocker, and rendered visual acceptance proving the world, vehicles, characters, HUD and lighting look presentation-ready.