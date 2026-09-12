# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.19**.

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
- `LMB` — attack with equipped rural weapon
- `Q` — cycle weapon inventory
- `G` — physically drop current weapon
- `Space` — jump
- `E` — interact / pickup / enter / jobs / services / story contacts
- `F` — exit vehicle
- `R` — cycle radio
- `F5` — quick-save
- `F9` — quick-load

## 0.0.19 Main Story Arc 3 — Red Barn Reckoning
1. Complete Main Story Arc 2 / **Timber Ghosts** first. Return to the Arc 3 farm-office marker at Player Farm with zero police/ranger attention.
2. Start **RED BARN RECKONING** and follow the HUD road hint to the new west-side Red Barn compound.
3. Try starting the Red Barn confrontation with no collected rural weapon. Verify the encounter refuses to start and tells the player to arrive armed.
4. Return with at least one Rural Arsenal weapon and interact again. Verify four hostile locals spawn and immediately engage the player.
5. Fight using fists/melee tools or the Old Farm Shotgun. Verify the HUD shows `RED BARN FIGHT` and a live hostile counter.
6. Knock out all four hostiles. Verify the encounter advances to the evidence-search stage and temporary hostiles are cleaned up.
7. Interact with Red Barn again to take the payoff ledger. Verify this injects major real wanted heat and Arc 3 switches to an **ESCAPE POLICE** objective.
8. Lose the police using the existing pursuit cars/roadblocks/interception system. Verify the objective automatically advances to **COUNTY DROP** only when wanted reaches zero.
9. Arrive at County Drop without an owned usable Mulebox and verify delivery is rejected.
10. Bring the owned **Mulebox 1200** within the evidence-drop area at 35%+ condition, interact and verify the `$900` evidence-delivery payout.
11. Return to Player Farm. Verify the finale requires at least three owned vehicles and pays `$1100` when complete.
12. Quit/relaunch during Red Barn Fight, Escape Police and County Drop stages. Verify Arc 3 stage resumes; loading during the fight should rebuild/refresh the hostile encounter.

## 0.0.19 persistent Rural Arsenal
1. Pick up several rural weapons, including the Old Farm Shotgun, and consume some shells.
2. Cycle to a non-default equipped weapon.
3. Quit and relaunch the game without manually rebuilding the inventory.
4. Verify collected weapon types, equipped weapon and remaining shotgun shell count are restored.
5. Fire the shotgun once, relaunch again and verify the reduced shell count remains reduced.
6. Drop a weapon with `G`, relaunch and verify the dropped weapon is no longer restored to the player's saved loadout.
7. Get knocked out while carrying shotgun shells; verify the two-shell defeat penalty is persisted.

## 0.0.18 Rural Arsenal regression
1. Start at Player Farm and find the **Pitchfork** / **Rake** pickups around the farm/barn area. Use `E` and verify the HUD changes from Bare Hands to the picked-up weapon.
2. Find the remaining countryside items: Workshop Wrench, Wood Axe, Heavy Branch, Shovel, Cattle Chain and Old Farm Shotgun.
3. Pick up at least three melee weapons. Press `Q` repeatedly and verify the equipped item cycles through the inventory and the HUD inventory count stays coherent.
4. Press `G` with a non-bare-hand weapon equipped. Verify a physical pickup appears near the player and the item leaves the inventory; pick it up again.
5. Attack an ordinary villager with `LMB`. Verify the villager receives knockback, becomes hostile and the existing wanted system gains assault heat.
6. Continue the fight. Verify healthy villagers chase/retaliate, badly hurt non-brawl villagers try to flee and zero-health villagers become temporarily knocked out instead of being permanently removed.
7. Let a hostile villager hit the player repeatedly. Verify `RURAL ARSENAL | HP` decreases.
8. Allow player HP to reach zero. Verify the player wakes at Player Farm, receives the `$85` clinic/cleanup cost, wanted is cleared and the current weapon is holstered.
9. Find the **Old Farm Shotgun** pickup and verify it grants a limited shell count. Fire once and verify shell count drops and police heat rises sharply from firearm discharge.
10. Hit a vehicle with a melee weapon and shotgun. Verify both feed the existing vehicle-damage system, with the shotgun having the stronger effect.
11. Empty the shotgun and verify trying to fire produces the no-shells activity message instead of attacking.

## 0.0.18 Bent Axle Brawl regression
1. Clear wanted and visit **The Bent Axle** between 18:30 and 02:30.
2. Interact with the brawl entry point. Verify three hostile locals spawn and immediately engage the player.
3. Verify the HUD shows `BENT AXLE BRAWL`, standing opponent count and the two-minute timer.
4. Fight with fists or melee farm tools. Hits against registered brawl participants should not create ordinary melee-assault heat.
5. Fire the shotgun during the brawl and verify firearm-discharge heat still triggers police response.
6. Knock out all three opponents before time expires. Verify the activity pays `$260` and cleans up the temporary brawlers.
7. Repeat while wanted and during daytime; entry should be rejected.
8. Let the timer expire once and verify the brawl fails/cleans up without paying the purse.

## 0.0.17 vehicle dynamics regression
1. Drive Rusty Fieldmaster 60 and verify 5 short ratios, four-contact suspension telemetry and ~58 km/h target envelope.
2. Drive Rattleback 82 and verify road-biased grip, 5 ratios and ~128 km/h target envelope.
3. Drive Mulebox 1200 and verify 4 utility ratios, longer/heavier suspension and ~104 km/h target envelope.
4. Damage/overheat/tune vehicles and verify the same drivetrain responds to condition, engine upgrades and tire health.
5. Compare Fieldmaster vs Rattleback in Hill Farm mud; Fieldmaster should preserve more effective grip.
6. Regression: garage recall, body damage, spike strips, timber payout, mowing and physical recovery towing still function.

## Main story regression
### Arc 1
- Complete Borrowed Tractor -> County Ledger -> nighttime Backroad Deal -> police escape -> workshop delivery -> two-owned-vehicle farm finale.
- Verify Arc 1 rewards and story persistence.

### Arc 2 — Timber Ghosts
- Player Farm -> Warden Outpost -> Forest Cache -> clear real ranger alert -> Hill Farm with healthy owned tractor -> Player Farm finale.
- Verify `$500` evidence handoff, `$700` finale and shared-road `NEXT ROAD` guidance.
- Verify story save v2 resumes Arc 2 after relaunch.

## Roads / police / garage regression
- Wanted 3 adds pursuit vehicles; wanted 4–5 adds shared-road-node interception and roadblocks/spike strips.
- Traffic uses the shared countryside road graph including rural branches.
- Garage slot selectors recall Fieldmaster/Rattleback/Mulebox deterministically and charge `$15` only on successful recall.

## Rural work / nightlife regression
- Feed cargo: Player Farm -> Feed Depot -> Hill Farm, with timer/cargo integrity/fast bonus.
- Timber: North Wood Yard -> Workshop, with damage-sensitive payout.
- Mowing: tractor required; FIELD GATES 1–5 in order.
- Roadside recovery: physical tow constraint, snap/re-hook and condition-sensitive payout.
- Night Shift Favor: Bent Axle -> workshop parts -> stranded neighbor -> tavern, nighttime only.
- Fishing/poaching still drive fish economy and the independent game-warden response.

## Current limitations
- Combat uses source-driven traces and primitive placeholder presentation. Authored skeletal melee/firearm animations, hit reactions and final weapon meshes are not yet present.
- Rural Arsenal loadout/ammo now persists, but currently in a dedicated combat save slot rather than the primary sandbox SaveGame.
- Arc 3 persists in its own campaign slot; story/combat saves still need eventual consolidation into one sandbox save schema.
- Red Barn hostiles reuse the current citizen combat body/behavior rather than unique authored faction models/animations.
- 0.0.17 vehicle dynamics remain a source-driven four-contact suspension/drivetrain, **not yet a verified native `ChaosWheeledVehicleMovement` setup**.
- `ChaosVehiclesPlugin` is enabled/linked, but repository CI cannot compile/test a full UE 5.8 Win64 package.
- Towing uses a real Unreal physics constraint but not authored hitch sockets/trailer skeletal rigs.
- Mud uses gameplay volumes rather than landscape physical-material deformation.
- Shared road graph lacks lane metadata, speed limits and authored junction priorities.
- Vehicles/world/weapons remain primitive-mesh prototypes rather than final art.
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
