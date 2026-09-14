# GTT 0.0.68 Playtest — Native Road Traffic Incidents & Wanted Escalation

## Scope

This milestone connects Native Chaos road-vehicle crashes to the existing civilian traffic and wanted/police loop. It does not add a parallel crime system: the hidden driver pawn's existing `UGTTWantedComponent` remains authoritative.

## Preconditions

- Use an authored Native Rattleback 82 or Mulebox 1200 that passes the existing takeover gates.
- Own the corresponding garage vehicle and enter it through Native takeover.
- Ensure civilian traffic is active on the shared village/countryside road graph.
- For the loaded-van comparison, start the legal farm cargo job and load Mulebox 1200.

## A. World-object collision regression

1. Hit a rigid world object above the existing Native impact threshold.
2. Confirm 0.0.67 still emits `NATIVE_ROAD_IMPACT_DAMAGE` and persists condition/tire damage.
3. Confirm no `NATIVE_ROAD_CRIME_ESCALATION` is emitted when no civilian traffic vehicle is within the bounded incident radius.

**Pass:** ordinary self-inflicted world collisions do not create false traffic crimes.

## B. Civilian traffic crash

1. Drive Native Rattleback into a civilian traffic car at a controlled speed above 16 km/h.
2. Confirm the Native pawn impact counter increments.
3. Confirm `NATIVE_ROAD_TRAFFIC_INCIDENT` identifies the player vehicle, nearby traffic victim, impact speed and victim damage.
4. Verify the civilian traffic car loses condition through its existing shared `AGTTVehicleBase` damage model.
5. Repeat above the severe-impact threshold and verify victim tire damage is applied as well.

**Pass:** player crashes now have consequences for the traffic vehicle, not only the player's own car.

## C. Wanted/police escalation

1. Repeat a traffic collision while the player is driving the Native road vehicle.
2. Confirm the hidden driver pawn's existing `UGTTWantedComponent` receives heat.
3. Confirm `NATIVE_ROAD_CRIME_ESCALATION` reports heat added, total heat and resulting wanted level.
4. Continue causing incidents until existing wanted thresholds escalate police response.
5. Stop committing incidents and verify the existing wanted decay still behaves normally.

**Pass:** traffic crashes feed the established police/wanted loop; there is no duplicate Native-only wanted state.

## D. Incident deduplication

1. Produce one collision and remain in contact with the victim briefly.
2. Confirm the incident subsystem processes each new Native impact count once rather than adding heat every scan tick.
3. Separate the vehicles and collide again.
4. Confirm the next real impact produces a second incident.

**Pass:** heat and victim damage are event-based, not timer-spammed.

## E. Mulebox cargo consequence

1. Start the legal farm cargo contract and load Mulebox 1200.
2. Record wanted heat before a controlled traffic collision.
3. Perform the same collision unloaded and loaded where practical.
4. Confirm loaded Mulebox includes the small cargo-related heat multiplier on top of the existing 0.0.67 cargo-inertia self-damage behavior.
5. Continue the legal job and confirm cargo/reward state still uses the existing farm-job path.

**Pass:** carrying cargo increases the consequence of reckless road driving without creating a second cargo or economy state.

## F. Regression / safety

- Rattleback/Mulebox native takeover still requires authored rig, canonical wheels, powertrain and Physics Asset.
- Fieldmaster-specific trailer/heavy-haul systems are unchanged.
- Traffic route driving, horn/avoidance and stuck recovery remain operational after receiving damage.
- Garage/save/workshop persistence remains driven by the existing legacy mirror.
- Roadmap runtime-only checkboxes remain open without genuine Unreal/Win64 runtime evidence.

## Demo gate

**Do not create a Demo Release from source-level Project sanity alone.** The demo remains blocked until a real UE 5.8 Win64 package is produced, the packaged `GTT.exe` passes runtime smoke testing, rendered visual acceptance passes, relevant GitHub Actions are green, and no demo-critical blockers remain.
