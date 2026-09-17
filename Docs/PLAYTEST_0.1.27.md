# GTT 0.1.27 Playtest — Demo Vertical Slice Integration

This checklist validates the legal-work slice from contract acceptance through cargo pickup and delivery, with the new world-space route guidance and stopped-vehicle handoff rule. Source sanity passing is **NOT a packaged-build verification**; Win64 runtime cases remain open until exercised on a qualifying Unreal Engine 5.8 Windows build.

1. Start a fresh prototype session and confirm no farm route beacon is visible while no legal cargo contract is active.
2. Inspect the unified contract board and confirm Farm Cargo still uses the existing fleet-readiness/preparation assessment rather than a new duplicate vehicle selector.
3. Accept Farm Cargo with a mission-ready loadout and confirm the existing legal-work Wanted/warden lockouts still apply.
4. Confirm the world-space route guidance appears only after the farm contract actually starts.
5. During `ReachPickup`, confirm the beacon targets the real Feed Depot pickup terminal, not the farm contract board.
6. Approach the Feed Depot from multiple directions and confirm the distance readout decreases toward the terminal.
7. Confirm the route beacon has no collision and cannot block the player, traffic or the cargo vehicle.
8. Confirm the beacon projects near the static ground surface on flat road, sloped terrain and the depot apron without following dynamic vehicles.
9. Confirm the compact label remains readable as the player drives around the marker and does not depend on a giant prototype wall label.
10. Load cargo with a legacy Mulebox and confirm the existing cargo-load handling effect remains active.
11. Load cargo with the Native Mulebox path when available and confirm the existing Chaos cargo-load factor is still used.
12. After pickup, confirm guidance moves to Hill Farm and displays destination distance, remaining delivery time and current cargo integrity.
13. Damage the delivery vehicle enough to reduce cargo integrity and confirm both the existing objective state and route beacon reflect the lower cargo condition.
14. Trigger a police Wanted state during the run and confirm Hill Farm handoff remains blocked by the existing legal-staff rule while the delivery timer continues.
15. Clear Wanted and return to Hill Farm; verify handoff becomes possible again without resetting cargo integrity or the timer.
16. Attempt a Hill Farm drive-by handoff above 3.0 km/h and confirm completion is rejected with a stop-the-vehicle message.
17. Stop at Hill Farm at or below 3.0 km/h and confirm a tier-1 route can complete normally.
18. For a tier-2/3 cargo chain, confirm the Hill Farm relay advances to `DeliverFinalStop` without paying the final contract early.
19. Confirm route guidance switches from Hill Farm to North Wood Yard after the relay is signed.
20. Attempt a North Wood Yard drive-by handoff above 3.0 km/h and confirm the final completion is rejected while the same delivery clock continues.
21. Stop at North Wood Yard and complete the extended chain; verify the existing payout, cargo integrity, route bonus and market multiplier logic still determine the reward.
22. Confirm successful delivery still records logistics reputation/history and immediately saves through the existing authority path.
23. Fail the cargo by timeout and confirm the beacon disappears when the farm director returns to Idle.
24. Fail the cargo through severe cargo damage and confirm the beacon disappears and the reserved-cargo failure consequence remains intact.
25. Finish a successful contract and confirm only one route beacon existed for the entire run; starting another contract should reuse the world-owned guidance actor.
26. Start another legal activity and confirm the farm route beacon does not appear for mowing, timber, recovery or road-courier jobs.
27. Trigger a ranger road stop while carrying legal cargo and confirm the 0.1.26 pull-over marker and the farm destination beacon remain separate presentation layers with no duplicated Wanted/economy authority.
28. During a ranger SEARCH, confirm the enforcement declutter behavior still works and the farm destination beacon does not alter COMPLY/SEARCH/FLEE state.
29. Verify source CI runs the previous contract-board preparation verifier before the new 0.1.27 vertical-slice verifier.
30. Verify source CI reruns the 0.1.26 ranger world-marker verifier to protect the previous presentation milestone.
31. On a qualifying Win64 + UE 5.8 runner, compile/cook/package the exact candidate and confirm these new C++ classes compile without source-contract-only assumptions.
32. In the packaged EXE, perform the complete board -> pickup -> traffic/NPC drive -> safe handoff -> payout/save loop and capture runtime/visual evidence before considering any demo release.

## Acceptance

0.1.27 source acceptance requires the dedicated sanity workflow to pass while `Docs/ROADMAP.md` remains mathematically consistent at **125/130 = 96.2%**. Demo acceptance additionally requires the already-defined Win64 compile/package, packaged runtime smoke, Native Chaos/trailer evidence and rendered visual approval gates; this checklist does not waive any of them.
