# GTT 0.0.93 Playtest — Persistent Native Structural Damage

This milestone extends the existing 0.0.92 packaged route. It does not replace the physical roadblock, post-spike escape, tire/condition persistence or paid workshop recovery checks; all of them must still pass before the structural phase begins.

## 1. Structural damage setup

Run the packaged candidate with `-GTTDemoSmokeScenario` and keep the runtime log. After `DEMO_SCENARIO_DAMAGE_RECOVERY result=PASS`, the 0.0.93 structural subsystem must select the same owned Native road fleet class (`Rattleback82` or `Mulebox1200`).

Expected structural staging evidence:
- two scripted left-side crash events at 110 km/h equivalent,
- one front crash event at 70 km/h equivalent,
- at least one detached-panel bit,
- left and front zone health below pristine,
- non-zero cooling stress,
- positive body-damage repair surcharge.

The scripted crash path is not a replacement for collision physics. It calls the production `ApplyNativeImpactDamage` model and exists so missions/cinematics and the deterministic evidence route can trigger the same zone/condition/tire/panel calculations without relying on random world geometry.

## 2. Exact SaveProgress / LoadProgress round trip

The active structural game mode must:
1. flush Native compatibility state,
2. execute the existing base `SaveProgress`,
3. store road structural records in primary save schema v5,
4. persist `FrontHealth`, `RearHealth`, `LeftHealth`, `RightHealth`, `CoolingStress` and `DetachedPanelMask` for the Native road fleet.

The evidence route then deliberately resets live structural state to pristine **after saving**. This is the anti-stale check: a PASS is impossible if the following load merely reuses the in-memory body state.

`LoadProgress` must deactivate any active Native takeover, restore the legacy/base vehicle snapshot, restore the exact structural record and reactivate Native Chaos. Required log:

`DEMO_SCENARIO_STRUCTURAL_PERSISTENCE ... result=PASS`

Acceptance:
- front/left health reload within 0.03 of saved values,
- cooling stress reload within 0.03,
- saved/reloaded panel masks match and are non-zero,
- structural repair surcharge remains positive after load.

## 3. Persistent detached-panel presentation

After reload, verify the damaged road vehicle still communicates the saved panel-loss state instead of silently becoming pristine. The persistent restore recreates detached debris proxies as physical components around the vehicle and keeps the exact panel bitmask for workshop/service logic.

This remains a prototype presentation system: final authored deformable body meshes/panel art are still a separate visual-production task. Do not treat the proxy debris proof as final art acceptance.

## 4. Paid workshop structural recovery

The evidence route parks the damaged Native road vehicle at the real `AGTTServiceTerminal`, invokes `Workshop`, and uses the real player economy.

Required outcome:
- cash decreases,
- paid amount is greater than the saved structural surcharge (base workshop cost + body surcharge),
- all four body zones return to >= 0.999,
- cooling stress returns to <= 0.01,
- detached panel mask returns to 0,
- `GetBodyDamageRepairSurcharge()` returns 0,
- the repaired state is saved again as pristine.

Required log:

`DEMO_SCENARIO_STRUCTURAL_REPAIR ... result=PASS`

Final phase log:

`DEMO_SCENARIO_STRUCTURAL_RECOVERY result=PASS ... route=structural-save-load-workshop`

## 5. Packaged evidence manifest

`DEMO_SCENARIO.json` must use:
- schema: `gtt.demo-scenario.v10`
- route: `structural-damage-save-load-workshop-recovery`
- `required_step_count = 30`
- all previous 26 core steps,
- `DAMAGE_PERSISTENCE`,
- `WORKSHOP_RECOVERY`,
- `STRUCTURAL_PERSISTENCE`,
- `STRUCTURAL_REPAIR`.

The Win64 workflow gives the packaged executable at least 105 seconds of runtime observation and evaluates the deterministic scenario before packaged gameplay smoke and the final technical demo gate.

## 6. Demo release decision

0.0.93 is **not** automatically a public demo candidate just because source/contract CI is green. A public Demo Release still requires all of the following at the same commit:
- real UE 5.8 Win64 package on the configured self-hosted runner,
- successful packaged-EXE smoke/runtime route with `gtt.demo-scenario.v10` PASS,
- green relevant GitHub Actions,
- no demo-critical blockers,
- rendered visual acceptance for world, vehicles, player/NPC presentation, weapons/combat, HUD, lighting and atmosphere.

If the self-hosted Unreal runner or visual acceptance is unavailable, report the missing verification explicitly and do not publish a Demo Release.
