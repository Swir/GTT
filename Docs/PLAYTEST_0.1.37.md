# GTT 0.1.37 — Roadside Dispatch HUD & Cargo Continuity Playtest

> Source milestone playtest. Passing this document does **not** replace the required packaged Win64 Unreal runtime/demo gates.

## Goal

Prove that roadside patch/tow presentation is driven by the same locked dispatch authority introduced in 0.1.36. The HUD must show the request-time quote, live ETA and exact persistent target vehicle, while Farm Cargo keeps the load pinned to that same vehicle.

## Desktop / gamepad acceptance

1. Damage the native Mulebox until recovery is recommended; pre-dispatch HUD shows patch/tow estimates.
2. Press **Y**: pending line changes to `PATCH DISPATCH` and explicitly labels the price as `LOCKED`.
3. Confirm the displayed patch price equals the request-time roadside quote.
4. Confirm ETA decreases while the request is pending.
5. Confirm the displayed vehicle ID equals `GetPersistentVehicleId()` for the requested Mulebox.
6. Press **Y** again before arrival; dispatch cancels and no cash is deducted.
7. Request patch again; verify a fresh dispatch contract appears.
8. During pending patch, change damage state; locked HUD quote must not change.
9. Complete patch; pending contract line disappears.
10. Re-damage until tow is recommended; press **T**.
11. Pending line changes to `TOW DISPATCH`, showing locked quote, ETA and target ID.
12. Press **T** again before arrival; tow cancels without charge.
13. Request tow again and allow completion; exactly one tow fee is charged.
14. Press opposite service key while one service is pending; active service must not silently switch.
15. Trigger Wanted while voluntary service is pending; voluntary dispatch must not become a police impound charge.
16. Verify police impound presentation remains separate from voluntary patch/tow cancellation UX.
17. Repeat patch and tow flows using D-pad Left / D-pad Up.

## Farm Cargo continuity

18. Accept a Tier-2+ Farm Cargo contract and pick up at Feed Depot using the native Mulebox.
19. Confirm cargo authority binds the exact Mulebox persistent ID.
20. Damage the loaded Mulebox until recovery is recommended.
21. Start emergency patch; cargo recovery panel shows locked quote, ETA and exact dispatch vehicle ID.
22. Confirm `CARGO PIN OK` only when dispatch target equals cargo authority vehicle ID.
23. Cancel patch before arrival; cargo remains bound and contract timer keeps running.
24. Start patch again and allow completion; cargo ID remains unchanged.
25. Verify emergency patch does not restore full workshop condition/body state.
26. Damage again and start tow; panel shows the tow request-time locked quote rather than a recomputed estimate.
27. Move/alter vehicle state while tow is pending; displayed locked quote remains stable.
28. Allow tow to complete; cargo remains attached to the same persistent vehicle identity.
29. Bring a decoy van to Hill Farm while exact cargo Mulebox is outside handoff range; delivery is rejected.
30. Recover exact Mulebox and complete Hill Farm handoff.
31. Complete North Wood Yard final stop.
32. Verify one payout, one cargo-run completion increment and one reputation update.
33. Save/load around a pending/recovered cargo state; no duplicate stock reservation or payout is created.
34. After final completion, cargo authority is cleared and no stale dispatch HUD survives.

## Presentation / accessibility regression

35. At 1280×720 the recovery panel does not cover the primary objective text.
36. At 1920×1080 locked quote, ETA and vehicle ID remain readable.
37. Long persistent IDs remain inside the widened panel or degrade legibly without overlapping other HUD regions.
38. Generic native-road recovery line and Farm Cargo recovery panel display the same authoritative quote/ETA for the same pending service.
39. Pre-dispatch estimates remain clearly different in wording from an accepted `LOCKED` dispatch contract.
40. Repeated cancellation/re-request does not leave stale ETA or stale target ID in the HUD.

## Repository / CI regression

41. `python Scripts/verify_roadside_dispatch_contract.py` passes.
42. `python Scripts/verify_roadside_dispatch_hud.py` passes.
43. `python Scripts/generate_progress_svg.py --check` passes.
44. `Docs/ROADMAP.md` remains 125/130 (96.2%) unless a real unchecked runtime/art gate is independently verified.
45. Roadmap uses `../assets/readme/progress-mini.svg` and contains no legacy ASCII/Unicode meter.
46. README retains one `assets/readme/progress-card.svg`, README standard v2 and Search Keywords.
47. Relevant GitHub Actions are green on the exact candidate commit before merge.
48. After merge, exact-main Actions are rechecked; source CI is never reported as packaged Win64 verification.

## Demo gate

A demo release remains blocked until the exact candidate has a verified UE Win64 compile/cook/package, packaged EXE smoke, required Native Chaos/trailer runtime evidence, visual acceptance, green relevant Actions and no demo-critical blockers.
