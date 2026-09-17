# GTT 0.1.23 — Ranger roadside positioning and traffic reactions playtest

This milestone extends the 0.1.22 road-stop loop. It does **not** replace the existing citation, seizure, Wanted or Wildlife Alert authorities.

## Preconditions

- Start a development build in the authored rural world with ambient traffic enabled.
- Have a legacy road vehicle, Native Fieldmaster and Native Rattleback/Mulebox available across repeated runs.
- Keep a legal save plus a save with rural contraband/fish so both clean and seizure outcomes can be exercised.

## Roadside enforcement

1. **Primary shoulder stage** — reach Wildlife Alert 2 in a vehicle and let the first ranger enter the 12 m order radius. Confirm the ranger approaches a point beside/slightly behind the vehicle instead of targeting the lane center.
2. **Heading reversal** — trigger another stop while travelling the opposite road direction. Confirm the shoulder frame follows current vehicle travel/forward direction rather than a world-axis assumption.
3. **Stopped before order** — enter the order radius already below 2.5 km/h. Confirm COMPLY transitions to SEARCH only after the ranger is inside the existing search radius.
4. **Moving target** — remain below flee speed but continue rolling during the grace period. Confirm the staging point follows the vehicle without oscillating from one shoulder to the other.
5. **Late reminder** — stay above the compliance speed until roughly the final 2.25 s. Confirm `COMPLY NOW` appears once, not every ranger update.
6. **Search hold** — stop correctly and hold for 2.25 s. Confirm the existing citation plus fish/contraband seizure resolves once and the stop authority clears.
7. **Break hold** — begin SEARCH, move above 2.5 km/h, then stop again. Confirm search progress resets before it can resolve.
8. **Exit vehicle** — exit during the stop. Confirm the transient vehicle-stop state clears and the normal on-foot ranger enforcement path can continue.

## Ranger reinforcement

9. **Second ranger** — at a level that produces reinforcement, let two wardens reach the same stopped vehicle. Confirm only one owns the contact and the other stages farther behind the same shoulder.
10. **No duplicate seizure** — complete the stop with two wardens present. Confirm the citation/seizure executes once.
11. **Owner removed** — destroy/despawn the primary ranger during a test stop. Confirm the world state does not remain a permanent traffic block; a subsequent incident can acquire a fresh stop.

## Traffic reaction

12. **Approach slowdown** — observe an ambient traffic car approaching an active stop from within about 22 m. Confirm it progressively slows before reaching the ranger/player contact.
13. **COMPLY hold** — while the driver is still in COMPLY, confirm the lead traffic car physically brakes/holds around the 6.5 m safety radius instead of pushing through.
14. **SEARCH hold** — enter SEARCH and confirm the protected traffic hold grows to about 8 m.
15. **Queue formation** — place several ambient cars on the same route. Confirm the lead car yields and following cars queue through their existing obstacle avoidance rather than overlapping the stop.
16. **Opposite direction** — observe traffic that has already passed and is moving away. Confirm it is not pulled backward or stopped by the road-stop system.
17. **Cross-road exclusion** — observe a car outside the road corridor near the same world position. Confirm the lateral corridor filter avoids an unrelated permanent stop.
18. **Collision priority** — crash an ambient car while it is in the response zone. Confirm crash/disabled response remains authoritative over road-stop yielding.
19. **Performance promotion** — with world performance budgeting active, confirm a car currently yielding is updated at critical cadence and does not visibly skip through the hold zone.
20. **No stuck jump** — keep the stop active longer than the traffic `StuckRecoverySeconds`. Confirm a deliberately yielding car does not use the normal stuck-recovery impulse to jump the queue.
21. **Immediate recovery** — resolve the search. Confirm traffic resumes its normal route without waiting for a saved/persistent road-stop timer.

## Flee and consequence regressions

22. **Flee after grace** — remain at or above 8 km/h after the seven-second grace. Confirm +45 Wanted heat is added once and the FLEE message appears.
23. **No post-flee citation erase** — after triggering flee, drive close past the ranger on the next updates. Confirm the old proximity citation does not immediately resolve/erase the fled-stop consequence.
24. **Traffic after flee** — trigger flee and watch nearby ambient traffic. Confirm the stop's traffic hold ends; normal pursuit/police systems own the moving incident.
25. **Incident reset** — clear the Wildlife Alert, then create a new poaching incident. Confirm a new road stop can be acquired and the one-shot evasion state is re-armed.

## Vehicle and persistence matrix

26. **Legacy car/van** — repeat comply/search and flee paths in the legacy vehicle family.
27. **Native Fieldmaster** — repeat in Native Fieldmaster and verify roadside staging does not interfere with Native Chaos drivetrain control.
28. **Native road fleet** — repeat in Rattleback or Mulebox and verify the stop uses the same shared authority.
29. **Save/load outside stop** — save after a resolved stop and reload. Confirm citation/contraband consequences persist through their existing owners while no transient traffic-control zone reappears.
30. **Save/load during stop** — save during COMPLY/SEARCH if the build allows it and reload. Confirm no duplicate persistent road-stop inventory/wanted state was introduced; enforcement may reacquire from live Wildlife Alert state.

## Demo evidence rule

Passing this source/playtest milestone is **not** a Windows demo acceptance. Do not close any remaining Roadmap runtime checkbox or publish a Demo Release until the exact candidate has a verified UE 5.8 Win64 compile/cook/package, packaged-EXE smoke, required Native Chaos/trailer manifests, green relevant Actions and rendered visual acceptance.
