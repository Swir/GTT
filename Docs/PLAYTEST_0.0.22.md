# GTT 0.0.22 Playtest — Rural Economy & Law

## Backlot Fence / poaching loop
1. Poach at the forest spot until a hare/boar/deer result succeeds.
2. Verify no immediate cash payout occurs; the message should report persistent contraband and an estimated fence value.
3. While ranger alert is active, try the Backlot Fence near the Rust Dogs road and verify sale is refused.
4. Clear ranger/police attention and use the fence again. Verify the stash is converted into cash and then becomes empty.
5. Save/relaunch between poaching and fencing and verify the stash survives through `GTT_RuralEconomy_01`.

## Farm Mutual insurance
1. Use the insurance terminal near the village shop with at least `$260` and verify the policy activates.
2. Damage an owned vehicle, park it near the office and interact again.
3. Verify a `$55` deductible is charged, body condition improves and tires are repaired, while fuel is not refilled.
4. Try a claim with a nearly pristine vehicle and verify it is rejected.

## Arrest -> impound
1. Enter an owned vehicle and obtain wanted heat.
2. Allow police to arrest the player while using that vehicle.
3. Verify the normal arrest fine still occurs and the nearby owned vehicle is moved to County Impound.
4. Use the impound terminal after wanted is clear and pay the release fee.
5. Repeat once with insurance active and verify the release fee is substantially lower.
6. Save/relaunch with a held vehicle and verify it remains registered as impounded.

## Road-law metadata / speeding
1. Drive through village/police roads and verify the authored limits are lower than East Road.
2. Hold speed more than roughly 18 km/h above the nearest road-node limit for at least three seconds.
3. Verify a cash speeding citation appears and repeated citations have a cooldown.
4. Exceed the limit by roughly 45 km/h or more and verify police heat is also added.
5. Compare forest tracks (one lane / low limit) with East Road (two lanes / higher limit).

## Regression
- Garage ownership/recall still works.
- Ranger citation still confiscates fish and clears wildlife alert.
- Police arrest still teleports/releases the player and clears wanted.
- Existing legal jobs, faction encounters, Heavy Haul and main story remain reachable.

## Known limitation
This repository CI does not perform a full Unreal Engine 5.8 Win64 compile/package/smoke test. Road-law enforcement currently samples the nearest authored road node rather than a spline-level lane segment.
