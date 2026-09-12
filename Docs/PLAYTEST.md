# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.17**.

## Requirements
- Unreal Engine 5.8
- Visual Studio 2022 with **Game development with C++**
- Windows 10/11 x64

## Launch
1. Clone the repository.
2. Generate Visual Studio project files for `GTT.uproject`.
3. Build the `GTTEditor` target.
4. Open `GTT.uproject` and press **Play**.

The project boots from Unreal's built-in Entry map and creates the greybox countryside at runtime.

## Controls
- `WASD` — walk / drive
- Mouse — camera
- `Space` — jump
- `E` — interact / enter / jobs / services / story contacts
- `F` — exit vehicle
- `R` — cycle radio
- `F5` — quick-save
- `F9` — quick-load

## 0.0.17 vehicle dynamics milestone
1. Drive the **Rusty Fieldmaster 60** on flat ground. Watch the new `VEHICLE DYNAMICS` HUD row and verify gear changes progress through the five short ratios instead of remaining a single direct-force response.
2. Drive over uneven greybox terrain/edges slowly and verify `CONTACT` varies between 0–4 and `SUSP` changes as the four suspension traces compress/release.
3. Accelerate the tractor on a long clear road. Verify power tapers near its ~58 km/h target envelope instead of accelerating indefinitely.
4. Repeat with **Rattleback 82**. Verify it uses five road-oriented ratios, accelerates/turns more aggressively and has a much higher ~128 km/h target envelope.
5. Repeat with **Mulebox 1200**. Verify its four utility ratios and longer/heavier suspension feel sit between tractor and car, with ~104 km/h target speed.
6. Damage a vehicle until condition drops materially. Verify acceleration weakens through the same drivetrain rather than only changing HUD/body state.
7. Overheat or stall a vehicle and verify drivetrain force drops to zero while steering/throttle input telemetry remains coherent after restart.
8. Upgrade engine level at the tuning terminal and verify effective acceleration improves. Upgrade/repair tires and verify grip improves after tire damage.
9. Drive each vehicle through Hill Farm mud. Verify grip drops and rolling resistance rises in the HUD/handling model while tire wear continues.
10. Compare Fieldmaster vs Rattleback in the same mud. The Fieldmaster should preserve noticeably more effective grip due to its dedicated 42% off-road recovery bias.
11. Tow the disabled recovery Mulebox through/near mud and verify the recovered vehicle still experiences physical terrain drag while the towing vehicle uses the new drivetrain.
12. Regression: garage recall, save/load, body damage, spike-strip tire loss, timber payout and mowing/recovery contracts must still function.

## 0.0.16 Main Story Arc 2 — Timber Ghosts
1. Complete Main Story Arc 1 and return to the **PLAYER FARM office**. Interact again to start **TIMBER GHOSTS**.
2. Follow the HUD road hint toward **WARDEN OUTPOST**. Verify the objective includes a `NEXT ROAD:` node and remaining route-node count.
3. Approach the warden while wanted or with existing ranger alert and verify the briefing is refused. Clear attention and accept the briefing.
4. Follow the shared-road hint into the forest and interact with the **FOREST CACHE**.
5. Verify taking the evidence creates a real game-warden alert through the existing ranger system and changes the objective to `CLEAR GAME WARDEN ALERT`.
6. Either evade until the ranger heat decays or accept the existing ranger citation. Verify the story advances only when wildlife alert reaches zero.
7. Go to **HILL FARM** and interact without a healthy owned tractor. The evidence handoff must be rejected.
8. Repair/own the Rusty Fieldmaster (40%+ condition) and retry. Verify the evidence chapter pays `$500` and advances to the final farm return.
9. Return to Player Farm and close Arc 2. Verify the final reward is `$700` and the HUD shows `ARCS 1-2 COMPLETE`.
10. Quit/relaunch during Arc 2 and verify `GTT_MainStory_01` v2 restores the current Arc 2 stage.
11. Load a 0.0.15 save whose story stage was `8`; verify it behaves as `ARC 1 COMPLETE` and allows starting Timber Ghosts rather than resetting campaign progress.

## 0.0.16 shared countryside road graph
1. Drive around the original village loop and verify the normal alternating civilian traffic remains active.
2. Observe additional traffic leaving the core loop toward rural destinations such as North Wood and Hill Farm.
3. Build wanted to level 4–5 while using East Road/forest/Hill Farm approaches. Verify police roadblocks still choose recognizable interception nodes.
4. Change direction between roadblock spawns and verify the existing predictive planner still chooses different likely escape nodes.
5. During story travel stages, compare HUD `NEXT ROAD:` hints with the same roads used by traffic/police.
6. Regression check: clear wanted and ensure pursuit/roadblock cleanup still works.

## 0.0.15 Main Story Arc 1 regression
1. Complete **BORROWED TRACTOR** and start the campaign at Player Farm.
2. Complete **COUNTY LEDGER**: North Wood pickup -> Village Shop delivery -> `$250`.
3. Meet the Bent Axle contact only during 18:30–02:30.
4. Collect the East Road crate and verify wanted heat / police escape stage.
5. Lose police, deliver to workshop -> `$600`.
6. Own two vehicles and finish Arc 1 at Player Farm -> `$350`.
7. Confirm the stage becomes `ARC 1 COMPLETE`, which is now the gateway into Arc 2.

## 0.0.14 police / garage regression
- Wanted 3 adds pursuit cars; wanted 4–5 adds route-aware roadblocks/spike strips.
- Explicit garage selectors recall Fieldmaster/Rattleback/Mulebox deterministically.
- Successful recall costs `$15`; occupied vehicles and active authority attention reject recall without charging.

## 0.0.13 recovery / mud regression
- Start roadside recovery at workshop, hook the disabled Mulebox with a working vehicle and physically tow it back.
- Excessive separation snaps the line and requires re-hooking.
- Recovery payout depends on time/condition.
- Hill Farm and forest mud apply physical drag and tire wear.

## Rural-work regression
- Feed cargo: Player Farm -> Feed Depot -> Hill Farm, with timer/cargo integrity/fast bonus.
- Timber: North Wood Yard -> Workshop, with damage-sensitive cargo payout.
- Mowing: tractor required; FIELD GATES 1–5 must be driven in order.
- Night Shift Favor: Bent Axle -> workshop parts -> stranded neighbor -> tavern, nighttime only.

## Existing systems to regression-test
- Borrowed Tractor mission and tractor ownership.
- Multi-vehicle garage/save/load and persistent tuning.
- Police arrest/fines and separate ranger citations.
- Fishing, poaching, fish buyer and legal work.
- Day/night, citizen schedules and nightlife.
- Four-station fictional radio.
- Vehicle body damage, detachable parts, heat/stalls and tire degradation.
- Workshop repair/refuel/tuning.

## Current limitations
- 0.0.17 replaces the old single-force player-driving path with a four-contact source-driven suspension/drivetrain model, but it is **not yet a verified native `ChaosWheeledVehicleMovement` setup** with authored wheel/suspension assets.
- `ChaosVehiclesPlugin` is enabled and the runtime module links `ChaosVehicles`, but the current repository runner cannot compile/test a full UE 5.8 Win64 Chaos vehicle package.
- Towing uses a real Unreal physics constraint but not authored hitch sockets/trailer skeletal rigs.
- Mud uses authored gameplay volumes rather than landscape physical-material sampling/deformation.
- Story stage still lives in a dedicated story save slot rather than the primary sandbox SaveGame schema.
- The shared road graph has topology and named nodes, but not lane metadata, speed limits, junction priority or nav-lane splines yet.
- Vehicles/world remain primitive-mesh prototypes rather than final art.
- Repository CI is structural sanity checking, not a full Unreal Win64 compile/package smoke test.

## Build a Windows package
```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1
```

Custom engine location:
```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
```

Shipping:
```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1 -Configuration Shipping
```

Output is written under `Releases/` by default and is ignored by Git.
