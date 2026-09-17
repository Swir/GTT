# GTT 0.1.30 Playtest — Persistent Farm Cargo Recovery

This playtest validates the **same active Farm Cargo contract and the same physical vehicle identity** across save/load and vehicle actor recovery. It does not replace the Win64 packaged runtime, Native Chaos or visual demo gates.

## Preconditions

- Use a fresh or known `GTT_Prototype_01` profile.
- Run inside normal gameplay unless a case explicitly requests packaged evidence mode.
- Keep the active cargo route visible in HUD/world guidance.
- Record vehicle persistent ID, route tier, depot stock, buyer demand, cash, logistics reputation, cargo history count, timer and integrity before checkpoints.

## A. Contract checkpoint — ReachPickup

- [ ] 01. Accept a Tier-1 Farm Cargo contract; save state shows active `ReachPickup` and no bound cargo vehicle ID.
- [ ] 02. Accept a Tier-2 extended contract; reserved depot units are debited exactly once.
- [ ] 03. Quick-load immediately after acceptance; objective returns to Feed Depot rather than Idle.
- [ ] 04. After that load, depot stock matches the pre-load post-reservation value; no second reservation is created.
- [ ] 05. Quick-load repeatedly at ReachPickup; stock and reservation count remain idempotent.
- [ ] 06. A pre-0.1.30 v8 profile with no active cargo fields loads with Farm Cargo Idle and no phantom vehicle binding.

## B. Loaded-vehicle checkpoint

- [ ] 07. Load a legacy Mulebox at Feed Depot and note its persistent ID.
- [ ] 08. Save immediately after pickup; stage is `DeliverCargo`, timer/integrity are non-default and bound ID matches the loaded Mulebox.
- [ ] 09. Quick-load; the same Mulebox persistent ID is rebound and Hill Farm accepts it when stopped in-zone.
- [ ] 10. Put a different owned vehicle beside Hill Farm before quick-load; it does not inherit the cargo.
- [ ] 11. After reload, attempting Hill Farm with only the wrong vehicle in-zone is refused.
- [ ] 12. Bringing the original loaded vehicle into the same zone allows handoff without reloading pallets.
- [ ] 13. Reload while cargo integrity is below 100%; restored integrity matches the saved value instead of resetting to 100%.
- [ ] 14. Reload with less than the full timer remaining; restored timer does not reset to the route maximum.
- [ ] 15. A saved police-incident flag remains part of the recovered run and cannot be cleared merely by quick-load.
- [ ] 16. Locked market multiplier and fleet prep multiplier survive load; payout cannot be improved by reloading after market/time changes.

## C. Native/legacy identity behavior

- [ ] 17. Native Mulebox load stores its stable persistent vehicle ID rather than actor name/pointer.
- [ ] 18. Native Mulebox save/load rebinds the exact matching native actor when available.
- [ ] 19. Legacy Mulebox save/load rebinds the exact matching legacy actor when available.
- [ ] 20. A legacy tractor used as the physical cargo vehicle remains the authority if the contract allowed it; nearby Mulebox does not steal the load.
- [ ] 21. A native road vehicle with a different persistent ID never satisfies the recovered authority.
- [ ] 22. Tier-3 restored Mulebox load reapplies the 1.20 cargo load factor.
- [ ] 23. Tier-1/2 restored Mulebox load reapplies the normal 1.00 cargo load factor.

## D. Missing actor / garage / tow recovery

- [ ] 24. Save with the loaded vehicle, remove/recreate that actor with the same persistent ID, then load; authority rebinds the replacement actor.
- [ ] 25. While the exact actor is temporarily absent, Hill Farm handoff is refused with a recovery message.
- [ ] 26. Spawning an unrelated vehicle closer than the missing cargo vehicle does not satisfy rebind.
- [ ] 27. Return/recreate the exact cargo vehicle after the failed handoff; the next authority tick or handoff attempt rebinds it.
- [ ] 28. Garage recall that teleports the same actor preserves authority without changing the bound ID.
- [ ] 29. Garage/tow flow that recreates the actor with the same persistent ID recovers authority without a second pickup.
- [ ] 30. A recovered actor must still be within 7.5 m and at/below 3.0 km/h for buyer handoff.

## E. Hill Farm relay checkpoint

- [ ] 31. Complete Hill Farm on a Tier-2/3 route; stage changes to `DeliverFinalStop` and a checkpoint is saved.
- [ ] 32. Quick-load after Hill Farm; objective resumes at North Wood Yard, not Feed Depot or Hill Farm.
- [ ] 33. The original bound vehicle ID survives the Hill Farm relay checkpoint.
- [ ] 34. Timer and integrity after Hill Farm reload match the saved relay values.
- [ ] 35. Hill Farm cannot be credited a second time after reloading the relay checkpoint.
- [ ] 36. Depot stock is not debited again after loading the relay checkpoint.
- [ ] 37. Buyer demand is not settled twice by repeated relay loads.

## F. Completion / failure cleanup

- [ ] 38. Complete a direct Tier-1 route; saved snapshot becomes Idle and bound vehicle ID is cleared.
- [ ] 39. Complete an extended route at North Wood Yard; saved snapshot becomes Idle and bound vehicle ID is cleared.
- [ ] 40. Reload after successful completion; no active cargo objective reappears.
- [ ] 41. Successful completion increases cargo history exactly once across save/load.
- [ ] 42. Successful completion pays cash exactly once across save/load.
- [ ] 43. Successful completion changes logistics reputation exactly once across save/load.
- [ ] 44. Let the delivery timer expire; failure snapshot becomes Idle and old vehicle ID is cleared.
- [ ] 45. Destroy cargo integrity to failure; reload does not resurrect the failed load.
- [ ] 46. After a failed/completed route, starting a new route binds only the newly loaded vehicle.

## G. Primary save non-regression

- [ ] 47. Trigger ranger citation save during ordinary play; logistics reputation/history remain unchanged except for intended ranger consequences.
- [ ] 48. Register/service an owned vehicle and save; current v8 logistics market fields remain intact.
- [ ] 49. Save/load preserves `bUnifiedWorldStateInitialized` and does not downgrade the primary slot to schema v3.
- [ ] 50. Existing combat/story/faction/rural-economy compatibility data still survives a normal save cycle.
- [ ] 51. A brand-new profile creates a v8-compatible snapshot with safe inactive-cargo defaults.
- [ ] 52. Existing owned legacy vehicle transform/fuel/tuning restoration still works.

## H. Packaged candidate regression

- [ ] 53. Existing 0.1.29 Farm Cargo packaged scenario still rejects deliberate wrong-vehicle Hill Farm handoff.
- [ ] 54. Existing `FARM_CARGO_RUNTIME.json` source evaluator contract remains valid.
- [ ] 55. Source sanity workflows for 0.1.28 and 0.1.29 remain green.
- [ ] 56. `Scripts/generate_progress_svg.py --check` remains green and reports the same authoritative Roadmap math.
- [ ] 57. A qualifying Win64 UE 5.8 build compiles the new persistence source files without UHT/UBT errors.
- [ ] 58. Packaged EXE quick-save/quick-load exercises the loaded route without crash or stale-pointer access.
- [ ] 59. Packaged EXE actor recreation/rebind produces stable-ID recovery evidence rather than nearest-vehicle substitution.
- [ ] 60. No demo release is created unless all existing technical and visual demo gates also pass on the exact candidate.

## Acceptance

Source milestone acceptance requires the new recovery sanity workflow plus all chained cargo regressions to pass. **Demo acceptance is separate** and still requires the protected Win64/runtime/Native Chaos/trailer/visual evidence gates in `Docs/ROADMAP.md`.
