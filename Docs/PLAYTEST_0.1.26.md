# GTT 0.1.26 Playtest — World-Space Pull-Over Guidance & Patrol Lighting

> This checklist is for the first qualifying Unreal/Win64 runtime. Source CI can verify wiring and invariants, but it cannot substitute for rendered or packaged-game acceptance.

## Pull-over marker

1. Trigger a Wildlife Alert 2+ road stop while driving the Fieldmaster and confirm a single world-space `PULL OVER` marker appears on the selected shoulder.
2. Repeat with Rattleback and confirm the target appears at the same authoritative stop location reported by the COMPLY HUD.
3. Repeat with Mulebox and verify vehicle family does not change the marker authority or acceptance radius.
4. Begin COMPLY while moving and confirm the target remains fixed instead of chasing the player vehicle.
5. Stop short of the marker and confirm SEARCH does not begin even below 2.5 km/h.
6. Enter the marker above 2.5 km/h and confirm SEARCH does not begin until speed compliance is also met.
7. Reach the marker below 2.5 km/h and confirm the existing hold/search flow begins.
8. Drive away during SEARCH and confirm search progress is interrupted by the existing spatial-compliance rule.
9. Confirm marker geometry has no collision and cannot snag the tractor, old car, van, pedestrians or ambient traffic.
10. Confirm FLEE removes the world marker while the existing police escalation remains active.
11. Confirm clearing the wildlife incident removes the marker without leaving visible components behind.

## Grounding and readability

12. Trigger a stop on flat asphalt and confirm the ground disc sits just above the road surface without z-fighting.
13. Trigger a stop on a raised/uneven authored roadside and confirm world-static grounding follows the surface rather than vehicle pivot height.
14. Confirm the grounding trace ignores dynamic vehicles and does not jump onto the stopped vehicle body during SEARCH.
15. Approach during daylight and confirm the marker is visible without overpowering the road scene.
16. Approach at night and confirm the pulsing shoulder light clearly identifies the requested stop location.
17. During SEARCH, confirm the floating `PULL OVER` label and chevron disappear and the ground cue dims.
18. Confirm the compact COMPLY/SEARCH HUD remains readable while the marker is visible; no duplicate wall of instructional text should appear.
19. Confirm the marker orientation follows the frozen road heading instead of world north.
20. Check a camera pass from front/rear/side and verify the code-built marker does not resemble protected third-party branding or signage.

## Patrol lighting

21. Start COMPLY and confirm the original warden support vehicle deploys once behind the shoulder target.
22. Confirm alternating visible roof beacons and amber point lights stay synchronized.
23. Confirm the patrol beacons remain visible at night but do not create excessive bloom or obscure the road.
24. Enter SEARCH and confirm the forward scene lamp activates.
25. Confirm the SEARCH lamp illuminates the inspection area without changing gameplay authority or compliance state.
26. Leave SEARCH back to COMPLY conditions and confirm the scene lamp turns off while roof beacons continue.
27. End the stop and confirm beacon meshes, beacon lights and search lamp are all hidden with the patrol actor.
28. Confirm the hidden patrol actor remains non-colliding outside an active roadside scene.
29. Trigger repeated incidents and verify the director still owns one support vehicle and one pull-over marker rather than accumulating actors.

## World integration and regression

30. Approach the stop behind ambient same-direction traffic and confirm existing progressive slow/queue behavior still uses the fixed lane anchor.
31. Pass the scene in opposite-direction traffic and confirm that lane is not frozen by the marker or patrol lighting.
32. Observe nearby civilians and confirm existing safe-side movement/recovery continues without targeting the marker actor itself.
33. Trigger a hostile NPC/combat interruption and confirm road-stop presentation does not override higher-priority combat/knockout behavior.
34. Trigger night-poaching reinforcement and confirm two ranger logic does not create a second marker or second roadside authority.
35. Comply with a contraband search and confirm confiscation/citation remains owned by the existing economy/ranger path.
36. Flee and confirm the existing one-shot +45 Wanted heat contract remains unchanged.
37. Save/load outside an incident and confirm no transient marker/patrol presentation state is persisted as player-owned content.
38. Exercise day/night transition during an active stop and look for lighting leaks, stale marker state or duplicate presentation.

## Demo gate

39. Run the qualifying UE 5.8 Win64 compile/cook/package pipeline and record whether the new actor/component APIs compile without adaptation.
40. Run packaged-EXE smoke and visually inspect COMPLY, SEARCH and FLEE presentation in the exact packaged candidate.
41. Capture rendered evidence only from the exact candidate SHA; source sanity output is not visual acceptance.
42. Do not create a Demo Release unless Win64 package/runtime, Native Chaos/trailer evidence, visual quality and the existing technical gate all pass.