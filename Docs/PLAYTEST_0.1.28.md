# GTT 0.1.28 Playtest — Farm Cargo Vehicle Authority & Progress Evidence

This playtest validates the source-integrated 0.1.28 milestone. It is **NOT a packaged-build verification**. Runtime/visual acceptance still requires an actual Unreal Engine 5.8 Win64 package and the exact candidate evidence chain.

## Farm Cargo physical-vehicle authority

1. Accept a Farm Cargo contract from the legal contract board and confirm the job reaches Feed Depot normally.
2. Load a legacy Mulebox at Feed Depot and confirm the message states that the physical cargo vehicle is locked to the contract.
3. Load the native Chaos Mulebox and confirm the same loaded-vehicle lock is created only after native readiness/takeover is active.
4. Load cargo while controlling a valid legacy work vehicle and confirm that controlled vehicle is chosen before other nearby vehicles.
5. Load cargo on foot with two legacy work vehicles nearby and confirm the nearest eligible vehicle inside the 700 cm depot search radius is bound.
6. Keep a damaged-but-working vehicle (>0 condition) beside the depot and confirm it can still be bound.
7. Put a broken/zero-condition vehicle closest to the depot and confirm it cannot become the bound cargo vehicle.
8. Hide an inactive legacy vehicle near the depot and confirm it cannot become the bound cargo vehicle.
9. After pickup, inspect runtime logs and confirm `FARM_CARGO_AUTHORITY event=BIND result=PASS` includes the persistent vehicle ID.
10. Drive the same loaded vehicle to Hill Farm and stop inside the 750 cm handoff zone at or below 3.0 km/h; handoff should be accepted.
11. Approach Hill Farm in the same loaded vehicle above 3.0 km/h; handoff must be refused and show the actual cargo-vehicle speed.
12. Stop a different vehicle beside Hill Farm while leaving the loaded vehicle elsewhere; handoff must be refused.
13. Park a different vehicle closer to the Hill terminal than the loaded vehicle; the different vehicle must never satisfy the contract.
14. Exit the correct loaded vehicle and walk to the Hill terminal while it remains safely parked inside the zone; handoff should still use the bound physical vehicle.
15. Leave the correct loaded vehicle more than 750 cm from Hill Farm and walk to the terminal; handoff must be refused with distance guidance.
16. For a direct Tier-1 route, complete Hill Farm with the same loaded vehicle and confirm the cargo authority clears after the contract reaches Idle.
17. Start a fresh contract after a completed direct route and confirm no stale vehicle identity leaks into the new load.
18. For Tier-2+ cargo, complete the Hill Farm relay and confirm the same cargo vehicle remains bound while the stage advances to North Wood Yard.
19. During an extended route, switch to another vehicle after Hill Farm and attempt North Wood Yard delivery; final handoff must be refused.
20. Bring the original same loaded vehicle to North Wood Yard, stop inside the zone at or below 3.0 km/h and confirm final handoff succeeds.
21. Confirm extended-route completion clears the cargo authority only after the final stop succeeds.
22. Let the delivery timer expire after pickup and confirm the contract fails; the authority should clear when the farm director returns to Idle.
23. Trigger cargo destruction/failure through severe vehicle condition and confirm the bound identity cannot be reused to claim a later handoff.
24. Trigger Wanted/police blocking at a buyer while using the correct loaded vehicle; the existing legal handoff lockout must still win and the delivery clock must continue.
25. Clear Wanted and retry with the same vehicle without reloading cargo; the normal contract should resume if its timer has not expired.
26. Confirm cargo integrity still reacts to the condition of the vehicle that the farm director loaded; 0.1.28 must not create a second cargo-condition value.
27. Confirm payout still comes from the existing FarmJobDirector/economy path rather than `UGTTFarmCargoAuthoritySubsystem`.
28. Confirm logistics reputation/history still updates through the existing logistics subsystem.
29. Confirm contract stock reservation/settlement still uses the existing logistics market and no duplicate stock ledger exists.
30. Confirm save handling remains in the existing game-mode/director path and the authority subsystem does not write an independent save slot.
31. Verify route beacon guidance still tracks Feed Depot → Hill Farm → North Wood Yard without owning contract state.
32. Verify road traffic, NPCs, day/night, vehicle damage and ranger/police systems continue running while Farm Cargo is active.
33. Run `python Scripts/generate_progress_svg.py --check`; it must report 125 / 130 tasks and 96.2% from `Docs/ROADMAP.md`.
34. Open README and confirm the 1200-wide progress card renders without clipping and the textual fallback says 125 / 130 tasks complete (96.2%).
35. Open `Docs/ROADMAP.md` and confirm the compact progress SVG uses `../assets/readme/progress-mini.svg` while the protected badges/table/text bar remain unchanged.
36. Inspect both generated SVGs and confirm release readiness is shown separately as NOT READY rather than being represented by the 96.2% roadmap completion figure.
37. Validate `progress-template.svg` as XML and confirm it is visibly labelled TEMPLATE / NOT PROJECT DATA with N/A defaults.
38. Run the dedicated 0.1.28 GitHub Actions sanity workflow and the previous 0.1.27 verifier; both must pass.
39. On a qualifying Windows x64 + UE 5.8 runner, compile/cook/package the exact candidate before claiming any runtime result.
40. In that packaged Win64 candidate, execute the Farm Cargo vertical slice with the same loaded vehicle through every required handoff; only real runtime evidence can close runtime/demo gates.
41. Perform the packaged-EXE smoke test and existing Native Chaos/drivetrain/trailer evidence chain on the exact candidate.
42. Perform rendered visual acceptance at gameplay resolution and confirm route guidance, vehicles, NPCs, HUD and roadside systems are presentation-worthy before any public demo release.

## Expected milestone result

0.1.28 prevents a second or opportunistic vehicle from completing a Farm Cargo load that it never picked up. The contract director remains the authority for stage, cargo integrity, payout, logistics history and save state. Roadmap completion remains **125 / 130 (96.2%)** and demo readiness remains **NOT READY** until the real Win64/runtime/visual gates pass.
