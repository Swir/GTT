# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.14**.

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
- `E` — interact / enter / jobs / services
- `F` — exit vehicle
- `R` — cycle radio
- `F5` — quick-save
- `F9` — quick-load

## 0.0.14 road-node police interception
1. Build wanted to level 3 and verify pursuit cars still join normally.
2. Raise wanted to level 4 while driving around the village loop or East Road.
3. Verify newly spawned roadblocks appear on recognizable road approaches rather than arbitrary nearby terrain.
4. Change direction before a new roadblock is requested and verify the planner favors a different node aligned with the new escape direction.
5. Reach wanted level 5 and verify two roadblocks can use different interception nodes instead of stacking on the same location.
6. Verify high-tier pursuit cars can also enter from a predicted route node rather than always spawning radially behind/around the player.
7. Clear wanted and verify roadblocks/pursuit units despawn through the existing response cleanup.

## 0.0.14 explicit garage slots
1. Own/register at least the Rusty Fieldmaster, Rattleback and Mulebox.
2. Visit Player Farm and verify four numbered physical garage selectors exist near the four bays.
3. Verify slot labels resolve the current fleet deterministically: Fieldmaster first, Rattleback second, Mulebox third and unused slots show `EMPTY`.
4. Interact with slot 2 and verify the Rattleback is recalled directly rather than cycling another owned vehicle first.
5. Verify a successful recall costs `$15` and the economy message records the garage service charge.
6. Try recalling an occupied vehicle and verify the request is rejected without charging the fee.
7. Try recalling while police wanted or game-warden alert is active and verify garage recall is locked.
8. Use the original large garage desk with a nearby unowned persistent vehicle and verify vehicle registration still works separately from recall.
9. Save/load and repeat recalls to confirm slot resolution remains stable from persistent vehicle IDs.

## 0.0.13 roadside recovery
1. Go to the workshop and interact with **RECOVERY DESK / DROP BAY** with zero police wanted.
2. Follow the contract to the disabled Mulebox on East Road.
3. Park a working vehicle within roughly 9 m of the disabled van and get out.
4. Interact with **RECOVERY HOOK**. The tow constraint should connect the parked vehicle and disabled van.
5. Enter the tow vehicle and drive toward the workshop. Verify the disabled van physically follows rather than teleporting.
6. Make a violent pull or create excessive separation. The tow line should snap and the contract should switch to re-hook state instead of silently completing.
7. Reattach and tow the van into the workshop recovery bay, then interact with the recovery desk.
8. Verify payout is added to the economy, fast completion can add a bonus, and poorer recovered-vehicle condition reduces reward.
9. Let the 240-second timer expire once and verify the contract fails and resets the target.
10. Try accepting while wanted and verify the legal contract is rejected.

## 0.0.13 mud/off-road handling
1. Drive tractor, old car and van through the Hill Farm field mud zone and the forest-track mud zone.
2. Verify moving vehicles experience obvious velocity-proportional resistance while inside the zones.
3. Compare a slow crawl with a fast pass: faster movement should produce more noticeable drag.
4. Repeatedly drive through mud and verify tire integrity decreases through the same shared tire-damage model used by collisions and spike strips.
5. Verify leaving the mud restores the normal source-only physics fallback behavior.

## 0.0.12 feed-cargo regression
1. Start **FEED CARGO CONTRACT** at Player Farm.
2. Drive any working vehicle to Feed Depot, park beside the pickup and exit.
3. Interact with **LOAD FEED PALLETS**. The nearby parked vehicle must satisfy the vehicle requirement.
4. Drive to Hill Farm, park inside the delivery yard, exit and interact with **DELIVER FEED CARGO**.
5. Verify timer, cargo integrity, fast bonus and payout still work.

## 0.0.12 legal timber haul
1. Clear police wanted and ranger alert.
2. Travel to **NORTH WOOD YARD** and take **TIMBER CONTRACT**.
3. Park a working vehicle beside **LOAD LOGS** and interact.
4. Drive the loaded vehicle across the map to the workshop.
5. Damage the vehicle and/or tires during one run and verify timber integrity/payout decreases.
6. Park beside **TIMBER UNLOAD** and interact.
7. Repeat quickly with a healthy vehicle and verify the fast-delivery bonus can be earned.

## 0.0.12 tractor field mowing
1. Bring the Rusty Fieldmaster tractor near the Hill Farm field office.
2. Interact with **MOWING CONTRACT**. Starting without a nearby tractor must be rejected.
3. Enter the tractor and drive through **FIELD GATE 1** through **FIELD GATE 5** in order.
4. Check that each overlap advances the HUD objective automatically without leaving the tractor.
5. Driving through a later gate out of order must not advance progress.
6. Finish all five passes before the timer expires and verify the base reward plus optional efficient-route bonus.

## 0.0.12 Night Shift Favor side mission
1. Visit **THE BENT AXLE TAVERN** between 18:30 and 02:30 with zero wanted level.
2. Interact with **NIGHT SHIFT FAVOR**.
3. Follow the HUD objective to the workshop and collect **EMERGENCY PARTS**.
4. Travel to **STRANDED NEIGHBOR / EAST ROAD** and interact with **HELP NEIGHBOR**.
5. Return to The Bent Axle and interact again.
6. Verify the side mission pays `$450` and saves resulting progress/economy.
7. Try starting outside nightlife hours and while wanted; both starts must be rejected.

## Existing systems to regression-test
- Four-station fictional radio and `R` cycling.
- Nightlife crowd and random village encounters.
- Police pursuit cars at wanted 3+ and roadblocks/spike strips at wanted 4–5.
- Borrowed Tractor mission and tractor ownership.
- Multi-vehicle garage, save/load and persistent tuning.
- Police arrest/fines and separate ranger/game-warden response.
- Fishing, poaching, fish buyer and legal feed/timber cargo.
- Day/night, citizen schedules and village traffic.
- Vehicle body damage, breakable parts, engine heat/stalls and tire degradation.
- Workshop repair/refuel and tuning.

## Current limitations
- Player, traffic and pursuit vehicles still use the source-only physics fallback; dedicated Chaos wheel/suspension drivetrain tuning is not yet implemented.
- Police interception currently uses a fixed source-defined road-node graph matched to the runtime greybox world; it is not yet a shared authored road graph/nav asset.
- Garage slot selection is physical world UI rather than a full UMG fleet-management screen.
- 0.0.13 towing uses a real Unreal physics constraint between primitive vehicle roots, but not yet authored hitch sockets, trailer skeletal rigs or dedicated Chaos Vehicle suspension.
- Mud is represented as authored gameplay volumes with drag/tire wear rather than landscape physical-material sampling or deformable terrain.
- Rural cargo is gameplay state rather than visible strapped log/pallet assets.
- Mowing gates model route completion; visible cut-grass deformation is not yet implemented.
- Night Shift Favor is session mission-state; its resulting cash is saved, but the side-mission stage itself is not yet in SaveGame.
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
