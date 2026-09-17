# GTT 0.1.24 — Enforcement presentation and civilian roadside playtest

This milestone extends the 0.1.23 world-authoritative warden stop. It does **not** replace citation, seizure, Wanted, wildlife-alert, traffic or save-game authorities.

## Preconditions

- Start a development build in the rural world with ambient traffic and civilian NPCs enabled.
- Keep a legal save plus a poaching/contraband save for COMPLY, SEARCH and FLEE outcomes.
- Exercise at least one legacy road vehicle plus Native Fieldmaster and one Native road vehicle across repeated runs.

## Compact HUD enforcement presentation

1. **Panel appears once** — trigger Wildlife Alert 2 while driving into warden road-stop range. Confirm one compact centered `WARDEN STOP | COMPLY` panel appears; no second debug block is added.
2. **COMPLY moving instruction** — stay above 2.5 km/h during the grace window. Confirm the panel instructs the player to pull over / slow below the compliance limit.
3. **COMPLY hold instruction** — slow below the limit while the ranger is still approaching. Confirm the instruction changes to holding position rather than continuing to demand speed reduction.
4. **COMPLY progress** — watch the single progress bar advance as the seven-second window is consumed; confirm it remains clamped inside the panel.
5. **SEARCH transition** — stop inside search range. Confirm the same panel changes to SEARCH instead of spawning another UI card.
6. **SEARCH progress** — hold still and confirm the bar reflects the 2.25 s compliance/search hold.
7. **Break SEARCH** — move above the compliance threshold before completion. Confirm SEARCH progress resets through the existing ranger authority and the panel returns to COMPLY behavior.
8. **FLEE transition** — exceed the flee threshold after grace expiry. Confirm the panel turns to FLEE / police escalation feedback.
9. **FLEE timeout** — continue pursuit after the short flee display window. Confirm the panel disappears even though the incident consequence remains latched.
10. **Resolved stop cleanup** — comply through citation/search. Confirm the panel clears immediately after the road-stop subsystem ends.
11. **No panel while walking normally** — with wildlife alert but no active vehicle stop, confirm normal HUD stays unchanged.
12. **Small-window safety** — use a narrower window/resolution. Confirm the panel width clamps to the canvas rather than overflowing the viewport.

## Civilian roadside behavior

13. **Pedestrian in lane** — place a civilian close to the stopped vehicle's road corridor. Confirm the civilian walks laterally toward a safe roadside point instead of wandering through the contact.
14. **Stable safe side** — observe the same civilian for several ticks. Confirm the chosen side does not flip every update while the road frame remains stable.
15. **Nearby safe observer** — place a civilian already outside the protected lane but within awareness distance. Confirm they can pause and face the stop instead of stepping back into traffic.
16. **Far civilian exclusion** — observe civilians beyond roughly 18 m. Confirm their schedule/wander behavior is not promoted or redirected by the stop.
17. **Longitudinal exclusion** — place a civilian near the road but well ahead/behind the contact. Confirm the bounded longitudinal window prevents unrelated village pedestrians from reacting.
18. **Schedule recovery** — resolve the stop and confirm reacting civilians resume normal home/work/social wandering without a persistent enforcement timer.
19. **Night schedule recovery** — repeat at night and confirm the smaller normal night wander radius returns after the stop.
20. **Combat priority** — start a brawl with a civilian near an active stop. Confirm combat behavior owns movement and is not cancelled by roadside observation.
21. **Hostile priority** — place a hostile archetype near the stop. Confirm faction combat/chase behavior is not turned into a passive witness reaction.
22. **Knockout priority** — knock out a civilian in the zone. Confirm road-stop response does not re-enable or move the knocked-out actor.
23. **Performance promotion** — with world performance budgeting active, confirm a reacting civilian receives urgent cadence and does not visibly skip across the protected lane.
24. **Performance recovery** — clear the stop and confirm civilians become eligible for their normal simulation tier again.

## Lane-aware traffic and queue regressions

25. **Same-direction approach** — approach the active stop with an ambient car behind the player. Confirm the existing progressive slowdown and hold still occurs.
26. **Opposite lane pass-through** — observe an oncoming ambient car travelling against the player's road direction. Confirm it is not frozen solely because it approaches the same world point.
27. **Same-direction queue** — place multiple cars behind the player. Confirm the lead car yields and followers queue through existing obstacle avoidance.
28. **SEARCH safety radius** — enter SEARCH and confirm same-direction lead traffic respects the wider existing 8 m protected radius.
29. **Crash priority** — crash a yielding car. Confirm collision/disabled response remains higher priority than warden-stop traffic control.
30. **No stuck jump** — keep same-direction traffic held longer than `StuckRecoverySeconds`. Confirm yielding cars do not impulse-jump the queue.
31. **FLEE traffic release** — flee after grace expiry. Confirm traffic control ends and cars resume route behavior while ranger/police pursuit continues.
32. **Cross-road exclusion** — verify an unrelated vehicle outside the existing corridor is not stopped.

## Authority / persistence regressions

33. **No duplicate Wanted state** — complete and flee separate stops. Confirm +45 evasion heat remains owned by the existing Wanted component and is applied once per incident.
34. **No duplicate seizure** — comply with rural contraband. Confirm seizure still executes exactly once through the rural economy subsystem.
35. **Two wardens** — spawn reinforcement. Confirm only the primary owns COMPLY/SEARCH while the second stages behind it; HUD still shows one incident.
36. **Save/load after resolution** — save after a completed search and reload. Confirm normal persistent citation/economy state remains, but no transient HUD/civilian/traffic stop reappears.
37. **Save/load during live stop** — if supported by the development build, save during COMPLY/SEARCH and reload. Confirm no new serialized road-stop presentation state was introduced.
38. **Vehicle matrix** — repeat COMPLY/SEARCH/FLEE in legacy car/van, Native Fieldmaster and Rattleback/Mulebox. Confirm the presentation and scene response are vehicle-family independent.

## Demo evidence rule

Passing this source/playtest milestone is **not** a Windows demo acceptance. Do not close any remaining Roadmap runtime checkbox or publish a Demo Release until the exact candidate has a verified UE 5.8 Win64 compile/cook/package, packaged-EXE smoke, required Native Chaos/trailer manifests, green relevant Actions and rendered visual acceptance.
