# GTT 0.1.22 — Ranger Road Stops, Search & Evasion Playtest

This milestone extends the existing wildlife-enforcement loop into vehicle-specific roadside stops. The goal is not a scripted cutscene: the same live ranger, vehicle, wanted, economy and contraband systems must produce different consequences depending on whether the player complies, exits, or flees.

## Runtime acceptance scenarios

1. **Alert-1 regression:** create wildlife alert 1 on foot and confirm the ranger still uses the existing chase/citation behavior with no vehicle road-stop order.
2. **Alert-2 vehicle order:** enter Rusty Fieldmaster, reach wildlife alert 2 and approach a ranger. Inside roughly 12 m, confirm `WARDEN ROAD STOP` appears once and gives a 7-second compliance window.
3. **Legacy fleet support:** repeat with a legacy `AGTTVehicleBase` road vehicle and verify the stop order uses its authoritative `GetSpeedKmh()` reading.
4. **Native Fieldmaster support:** repeat while controlling `AGTTFieldmasterNativePawn` and verify speed is read from live pawn velocity rather than a legacy mirror.
5. **Native road-fleet support:** repeat with Rattleback or Mulebox native pawn and verify the same stop/search rules apply.
6. **Moving citation guard:** drive through the normal citation radius while the road stop is active at more than 2.5 km/h. The moving vehicle must not instantly clear the wildlife incident through the old proximity citation path.
7. **Clean surrender:** stop below 2.5 km/h within the ranger's search radius and remain stationary. A visible `WARDEN SEARCH` message should begin the hold.
8. **Hold continuity:** move above 2.5 km/h before the 2.25-second search hold completes. The hold must reset rather than completing from non-contiguous stopped samples.
9. **Search completion:** remain stopped for the full hold. The existing ranger citation must resolve exactly once and clear wildlife heat.
10. **Fish seizure regression:** carry fish before complying. Confirm the authoritative citation still confiscates all fish and reports the existing fine/weight result.
11. **Rural contraband seizure:** carry poaching contraband before complying. Confirm the roadside search calls the existing rural-economy confiscation path, clears units/value and reports the lost fence value.
12. **No duplicate inventory:** after seizure, sell at the fence or save/reload and verify confiscated contraband cannot reappear from a parallel road-stop inventory.
13. **Grace expires while stopped:** stop outside the final search radius until the 7-second timer reaches zero. The player must not be marked as fleeing merely because the ranger still has to walk into search range.
14. **Slow roll tolerance:** after the grace window, remain below 8 km/h but above the 2.5 km/h compliance threshold. The search should not complete and police heat should not escalate until the player actually flees.
15. **Flee escalation:** after the grace window, accelerate to at least 8 km/h. Confirm `FLED WARDEN STOP` appears and 45 wanted heat is added through `UGTTWantedComponent`.
16. **One escalation per incident:** remain in flight through several ranger repath ticks. Wanted heat must not gain another +45 from the same stop order.
17. **Cross-agency behavior:** if the wildlife incident reaches alert 3, confirm the existing one-shot warden radio handoff can still operate independently without the road-stop controller creating a second police meter.
18. **Catch after flight:** allow the ranger to catch the player after an evasion. The normal citation/search path must still resolve the wildlife incident while police wanted state remains governed by the existing police system.
19. **Incident reset:** completely clear wildlife heat, create a new alert-2 incident and verify a fresh road-stop order/evasion decision can occur.
20. **Legal-work regression:** resolve the wildlife incident through compliance and verify legal farm/logistics work becomes available again under the existing job gating rules.
21. **Save/load regression:** quick-save after a completed compliant search, reload and verify fish/contraband seizure remains authoritative and no road-stop-only persistence is required.
22. **HUD/readability:** during a stop, confirm the new messages do not recreate a permanent debug-text wall and expire through the existing economy/message UI.
23. **Night poaching regression:** trigger the stop from the 0.1.21 night-risk loop and verify night wildlife multipliers/reinforcement still work unchanged.
24. **No false Demo claim:** confirm this source milestone does not mark any of the five Roadmap Win64/runtime hardware blockers complete without real UE 5.8 packaged evidence.

## Acceptance result

Source-contract CI can validate the state machine wiring, reuse of existing authority, README/roadmap locks and regression contracts. Full gameplay acceptance still requires a real Unreal Engine 5.8 run, and Demo Release remains blocked until the separate packaged Win64, runtime smoke and visual gates pass.
