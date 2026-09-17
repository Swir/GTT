# GTT 0.1.20 Playtest — Rendered visual evidence and exact-candidate release gate

## Purpose

Prove that the exact Win64 candidate considered for a public Demo Release is both technically valid and visually reviewable. Automated screenshot checks only prove that real, non-flat rendered frames exist; a human must still decide whether the candidate looks good enough to publish.

## A. Rendered capture run

1. Run the qualifying UE 5.8 Win64 candidate workflow on a self-hosted Windows x64 runner.
2. Confirm the normal NullRHI technical smoke still passes before the rendered pass starts.
3. Confirm the rendered pass launches the same packaged `GTT.exe` without `-nullrhi`.
4. Confirm the rendered pass uses 1280×720 and `-RenderOffscreen`.
5. Confirm the runtime log contains `DEMO_VISUAL_CAPTURE_PLAN scenes=5`.
6. Confirm `GTT_visual_world_gameplay.png` exists and shows actual world/gameplay, not a black/blank frame.
7. Confirm `GTT_visual_law_pressure.png` exists and clearly shows the current playable presentation during the law-pressure portion.
8. Confirm `GTT_visual_native_vehicle.png` exists and shows the Native Chaos vehicle phase.
9. Confirm `GTT_visual_loaded_trailer.png` exists during the loaded authored-trailer exercise.
10. Confirm `GTT_visual_hud_overview.png` exists after the route and shows a readable final HUD/world overview.
11. Confirm all screenshots include the intended game UI/HUD where applicable and are not editor screenshots.
12. Confirm the runtime log contains five `DEMO_VISUAL_CAPTURE_WRITTEN` markers and one `DEMO_VISUAL_CAPTURE_COMPLETE scenes=5`.
13. Confirm `VISUAL_RUNTIME_SMOKE.json` reports `null_rhi=false`, the exact Git SHA and at least 178 seconds alive.
14. Confirm `DEMO_VISUAL_EVIDENCE.json` is PASS, contains five scene records and still states `human_review_required=true`.

## B. Human visual acceptance

15. Download the candidate artifact **before** starting the publication workflow and inspect all five screenshots at full size.
16. Reject the candidate if the world is visibly broken, empty in a distracting way, overrun with debug text/placeholders, or lighting/exposure is clearly wrong.
17. Reject the candidate if vehicles, trailer, characters/weapons, traffic/NPC presentation or HUD/UI contain demo-critical visual defects.
18. Reject the candidate if the screenshots do not honestly represent the intended public demo slice.
19. If accepted, trigger the publication workflow with the exact candidate run id/SHA, `visual_review_passed=true`, and meaningful notes describing what was reviewed.
20. Confirm `DEMO_VISUAL_ACCEPTANCE.json` binds the review to the exact SHA, reviewer identity, visual-evidence manifest SHA256 and the five screenshot hashes.

## C. Publication integrity

21. Confirm the publication workflow verifies the referenced Actions run concluded `success`, belongs to `Win64 package evidence`, targets `main`, and has the same exact head SHA supplied for release.
22. Confirm it downloads the existing candidate artifact rather than recompiling/repackaging Unreal.
23. Confirm the candidate ZIP SHA256 matches its `.sha256` file before review acceptance is applied.
24. Confirm `evaluate_demo_candidate.ps1 -RequireVisual` passes only after the exact-candidate visual acceptance file exists.
25. Confirm a false visual-review input or short/empty notes block publication.
26. Confirm a mismatched SHA in any visual/technical manifest blocks publication.
27. Confirm a missing screenshot, flat/black screenshot, low-resolution image or duplicate/stuck-frame set blocks the visual evidence gate.
28. Confirm the release assets, if publication is enabled, include the original candidate ZIP + SHA256, `DEMO_VISUAL_ACCEPTANCE.json`, final `DEMO_TECHNICAL_GATE.json`, and the five reviewed screenshots.
29. Confirm the release is marked prerelease and release notes state the exact source SHA plus visual reviewer.
30. Confirm no release is created if any technical, rendered-evidence or human-review gate is not PASS.

## Expected status before a real qualified run

Source-only CI may verify this contract, but it must not create fake screenshots or claim visual approval. Until a qualifying UE 5.8 Win64 runner actually produces the candidate and a reviewer accepts those exact images, Roadmap stays 125/130 (96.2%) and Demo remains NOT READY.
