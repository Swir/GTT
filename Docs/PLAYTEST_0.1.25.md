# GTT 0.1.25 playtest — Physical Shoulder Pull-Over & Warden Patrol Scene

## Setup

Use an Unreal Engine 5.8 development build or PIE session with the normal prototype world. Trigger wildlife enforcement with a drivable Fieldmaster, Rattleback/Mulebox or legacy vehicle. These cases describe runtime acceptance targets; source CI alone does not prove the packaged cases.

## Pull-over target and COMPLY

1. Trigger Wildlife Alert 2 while driving. Confirm a single WARDEN ROAD STOP begins and the COMPLY panel instructs the player to move to the shoulder marker.
2. Keep driving forward after the order. Confirm the pull-over target stays fixed instead of sliding forward with the player.
3. Stop in the traffic lane but outside the shoulder target. Confirm SEARCH does not begin.
4. Stop near the ranger but outside the shoulder target. Confirm proximity alone does not resolve the citation.
5. Enter the shoulder target above 2.5 km/h. Confirm the panel requests a lower speed and SEARCH still does not start.
6. Enter the target below 2.5 km/h. Confirm SEARCH begins only after spatial and speed compliance are both true.
7. Leave the target during the hold. Confirm search progress resets.
8. Re-enter and remain stopped for the full 2.25 s. Confirm the existing citation/search resolution executes once.
9. Approach the target from the wrong side of the road. Confirm the marker remains on the side selected when the incident started.
10. Allow the grace timer to approach expiry while outside the target. Confirm the one-shot COMPLY NOW reminder includes the remaining marker distance.
11. Let the timer expire while still moving at 8+ km/h. Confirm the existing +45 Wanted escalation occurs once and FLEE remains latched for the incident.
12. After FLEE, confirm a reinforcement ranger cannot immediately issue a replacement road stop.

## Stable roadside scene

13. During COMPLY, confirm the primary ranger stages beyond the pull-over point rather than continuously following the vehicle centerline.
14. Trigger a second ranger at high/night alert. Confirm support stages behind the primary contact on the same roadside scene.
15. While driving toward the marker, verify the traffic-control anchor does not move with the player.
16. Confirm same-direction civilian traffic progressively slows before the fixed stop anchor.
17. Confirm same-direction traffic physically holds inside the configured COMPLY safety radius.
18. Enter SEARCH and confirm the traffic hold radius remains wider than COMPLY.
19. Confirm oncoming/opposite-direction traffic is not frozen by the stop.
20. End the stop normally and confirm queued traffic resumes without a saved/persistent stop timer.
21. Confirm nearby civilian pedestrians still clear the live lane/observe from safety and return to schedules after the incident.
22. Confirm hostile/combat/knockout NPC priorities still override roadside observer behavior.

## Warden patrol vehicle

23. Start with no active road stop. Confirm the warden patrol support vehicle is hidden and non-colliding.
24. Begin COMPLY. Confirm one patrol support unit appears behind the shoulder target and faces along the incident road direction.
25. Confirm the support unit does not become player-interactable, stealable, garage-owned or part of vehicle save data.
26. Confirm its alternating warning beacons are active only while deployed.
27. Transition COMPLY → SEARCH. Confirm the unit remains at the same stable roadside parking transform.
28. Complete the search. Confirm the patrol unit hides and disables collision.
29. Trigger FLEE. Confirm the unit may remain briefly for the short FLEE presentation, then disappears while pursuit/Wanted consequences continue.
30. Trigger a fresh wildlife incident after reset. Confirm the existing single support actor is reused rather than spawning duplicates.

## Existing enforcement/economy regression

31. Complete a clean search with contraband. Confirm `ConfiscateContraband` removes the same authoritative rural inventory/value as 0.1.22–0.1.24.
32. Complete a clean search without contraband. Confirm the existing citation path resolves once.
33. Test native Fieldmaster road-stop compliance.
34. Test native Rattleback/Mulebox road-stop compliance.
35. Test a legacy `AGTTVehicleBase` road-stop compliance.
36. Save/load outside an active incident and confirm no transient COMPLY/SEARCH/FLEE or patrol-scene state is persisted.
37. Verify Wildlife Alert 3 police handoff still uses `UGTTWantedComponent`, with no second wanted meter introduced by the roadside scene.
38. Verify night Alert 2 reinforcement still works and cannot duplicate the primary search/seizure.

## Presentation / demo gate

39. At 1280×720 and 1920×1080, confirm the compact COMPLY panel remains readable and the new marker-distance instruction fits without overlapping the main HUD.
40. Confirm the primitive-built patrol unit is treated as an original code-authored prototype presentation, not as proof of final authored visual quality.
41. Confirm no Demo Release is created unless the exact candidate also passes the real UE 5.8 Win64 package, EXE smoke, Native Chaos, drivetrain, authored trailer and visual-acceptance gates.
42. Confirm source CI success alone does not close any of the five remaining Roadmap runtime/hardware checkboxes.

## Expected milestone result

The stop should now feel like a physical roadside interaction: the driver has a stable shoulder destination, ranger/traffic/civilian behavior references one fixed incident frame, and a visible warden support unit parks behind the contact. The build must still be treated as **not demo-ready** until real Win64 runtime and visual evidence exist.