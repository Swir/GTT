# GTT 0.1.28 playtest — exact cargo authority & packaged vertical slice

These checks cover both manual Unreal playtesting and the new packaged evidence contract. Source CI validates wiring only; this document is **NOT a packaged-build verification** by itself.

1. Accept Farm Cargo from the unified contract board while Wanted and wildlife alert are clear.
2. Confirm a stock-backed reservation is created and saved immediately on contract acceptance.
3. Approach Feed Depot on foot with no working vehicle in range; pickup must fail without creating a persistent load lock.
4. Park a healthy legacy cargo-capable vehicle within the pickup radius and load it; the exact loaded vehicle must be locked.
5. Load the Native Mulebox while controlling it; the exact native actor must be locked.
6. Confirm the authority emits one `LOAD_LOCK result=PASS` with a persistent vehicle ID.
7. Confirm cargo load factor affects the Mulebox after pickup exactly as before 0.1.28.
8. Exit the loaded vehicle and walk near another healthy vehicle; the load lock must remain on the original actor.
9. Park a decoy vehicle at Hill Farm while the exact loaded vehicle is more than **750 cm** away; handoff must be rejected.
10. Repeat with a decoy of the same vehicle model; handoff must still be rejected.
11. Confirm rejection reports the locked vehicle ID and a distance greater than **750 cm**.
12. Move the exact loaded vehicle just outside 750 cm; handoff remains blocked.
13. Move the exact loaded vehicle inside 750 cm; distance authority may pass.
14. Drive the exact loaded vehicle through the yard above **3.0 km/h**; handoff must be rejected.
15. Stop a decoy while the exact loaded vehicle is still moving; handoff must remain rejected based on the loaded actor.
16. Stop the exact loaded vehicle at or below 3.0 km/h inside Hill Farm; handoff can proceed.
17. Confirm the accepted Hill Farm handoff log carries the same vehicle ID as the Feed Depot load lock.
18. On a tier-1 order, confirm Hill Farm completion clears the cargo lock after payout.
19. On a tier-2+ order, confirm Hill Farm becomes a relay and the exact load lock survives for North Wood Yard.
20. On a tier-2+ order, park a decoy at North Wood Yard with the loaded vehicle away; final handoff must be rejected.
21. Stop the exact loaded vehicle inside North Wood Yard and complete the final handoff.
22. Confirm every accepted handoff is <=3.0 km/h and <=750 cm in the runtime log.
23. Trigger Wanted during the delivery and confirm the existing police handoff block still takes precedence.
24. Clear Wanted before the timer expires and confirm the same exact loaded vehicle can complete the job.
25. Damage the loaded vehicle below the existing cargo-loss threshold and confirm cargo integrity degradation still uses the original job system.
26. Destroy cargo integrity and confirm failure clears physical cargo state without awarding cash.
27. Let the delivery timer expire and confirm the existing logistics failure/backlog consequence remains intact.
28. Complete a clean run and confirm cash rises by the normal computed reward rather than a test-injected value.
29. Confirm Cargo completed-runs/reputation history advances through `RecordCargoSuccess` rather than through the evidence subsystem.
30. Confirm successful completion calls the existing save path and survives reload.
31. Launch packaged Win64 with `-GTTFarmCargoScenario`; require `WORLD`, `CONTRACT`, `LOAD_LOCK`, `WRONG_VEHICLE_REJECT`, `HILL_HANDOFF`, `PAYOUT_REPUTATION`, `SAVE` and `COMPLETE` PASS markers.
32. For an extended route, require `FINAL_HANDOFF` before scenario completion.
33. Confirm `FARM_CARGO_SMOKE.json` reports schema `gtt.farm-cargo-smoke.v1`, PASS and the exact build SHA.
34. Confirm `FARM_CARGO_RUNTIME.json` reports schema `gtt.farm-cargo-runtime.v1`, PASS and the same build SHA.
35. Tamper the smoke SHA and confirm the evaluator rejects the evidence.
36. Remove the wrong-vehicle rejection marker and confirm the evaluator rejects the evidence.
37. Raise an accepted handoff speed above 3.0 km/h in fixture evidence and confirm evaluation fails.
38. Raise an accepted handoff distance above 750 cm in fixture evidence and confirm evaluation fails.
39. Set cash delta or cargo-run delta to zero in fixture evidence and confirm evaluation fails.
40. Confirm `evaluate_demo_candidate.ps1` refuses a technical candidate when `FARM_CARGO_RUNTIME.json` is missing.
41. Confirm the Win64 workflow uploads Farm Cargo smoke/runtime manifests and dedicated log in the technical candidate bundle.
42. Confirm failure diagnostics preserve the Farm Cargo log/manifests when the later gate fails.
43. Run `python Scripts/generate_progress_svg.py --check`; committed SVGs must exactly match the Roadmap source of truth.
44. Parse `assets/readme/progress-card.svg`, `progress-mini.svg` and `progress-template.svg` as XML; all must have valid viewBoxes and bounded finite geometry.
45. Confirm `progress-card.svg` says 125 / 130 roadmap items and 96.2%, with PRE-ALPHA status and separate release-readiness wording.
46. Confirm `progress-mini.svg` uses the same 125 / 130 and 96.2% values beside the authoritative Roadmap dashboard.
47. Confirm the template is labelled TEMPLATE / NOT PROJECT DATA and is not embedded as live progress.
48. Confirm README still has `<!-- SWIR-README-STANDARD:v2 -->`, Search Keywords and the explicit no-public-demo statement.
49. Confirm `Docs/ROADMAP.md` still has `<!-- SWIR-ROADMAP-STANDARD:v1 -->`, badges, text bar and table unchanged at 125/130 (96.2%).
50. Confirm all five Native Chaos / trailer / Win64 Roadmap blockers remain unchecked unless a real qualifying UE 5.8 Win64 run supplies their evidence.
