# GTT 0.0.20 Heavy Haul Playtest

This checklist validates the articulated trailer and heavy-timber contract without claiming a packaged Win64 build.

## Setup
- Unreal Engine 5.8 editor build of the repository.
- Existing save with the Rusty Fieldmaster 60 owned, fueled and above 40% condition.
- No active police wanted level or game-warden alert.

## Heavy timber contract
1. Go to Player Farm and interact with **HEAVY HAUL CONTRACT / TRACTOR + TRAILER**.
2. Verify the contract rejects a player who has no owned usable Fieldmaster near the farm.
3. Park the owned Fieldmaster beside the trailer and use **TRAILER HITCH**.
4. Drive toward North Wood Yard and verify the trailer articulates behind the tractor instead of being teleported or rigidly attached.
5. Create an intentionally violent separation once; the hitch should break and the contract should switch to re-hitch state.
6. Re-hitch and continue to North Wood Yard.
7. Enter the heavy-load zone and use **HEAVY TIMBER LOAD**. The visible cargo block should appear and trailer physical mass should increase from 980 kg to 1680 kg.
8. Drive the loaded trailer toward Hill Farm. Compare careful driving with a fast/tilted run; excessive speed or roll/pitch should reduce cargo/trailer integrity.
9. Deliver through **HILL FARM HEAVY BAY** while the trailer remains attached.
10. Verify payout depends on cargo integrity, trailer integrity and tow-vehicle condition, with an additional fast bonus for an efficient run.
11. Verify completion saves normal sandbox progress.

## Regression checks
- Existing roadside recovery still uses its separate tow-line constraint and can re-hook after a snap.
- Feed cargo, legal timber haul and mowing contracts still start and complete independently.
- Mud still affects tractor dynamics and tires while towing.
- Tractor condition/tuning/fuel still affect the tow vehicle through the shared dynamics layer.
- Police/ranger attention still blocks legal heavy-haul work.

## Current limitation
The trailer uses Unreal rigid-body physics plus `UPhysicsConstraintComponent` articulation with primitive greybox geometry. This is a real physical trailer gameplay step, but not yet an authored skeletal trailer with native Chaos wheel assets. Repository CI remains structural and does not verify a packaged UE 5.8 Win64 executable.
