# GTT 0.0.70 Playtest — Native Road Body Damage & Workshop

## Preconditions
- Use an authored Native Rattleback 82 or Mulebox 1200 that passes the existing Native takeover gates.
- Own the matching garage vehicle and confirm the Native road pawn is active.
- Keep `LogGTT` visible so damage-zone and workshop evidence can be checked.

## A. Directional body zones
1. Strike a solid world object front-first above 20 km/h.
2. Confirm `NATIVE_ROAD_DAMAGE_ZONE` reports `zone=FRONT` and front health decreases.
3. Repeat with rear-first and side impacts.
4. Confirm REAR/LEFT/RIGHT health changes independently instead of all zones being reduced equally.

**Pass:** impact location determines the damaged body zone while shared condition still decreases through the existing vehicle state.

## B. Detachable panels
1. Repeatedly hit one body zone hard enough to drive its zone health below the detachment threshold.
2. Confirm exactly one `NATIVE_ROAD_PANEL_DETACH` event for that zone.
3. Confirm a physical debris piece separates from the Native vehicle and receives collision impulse.
4. Continue hitting the same zone and confirm it does not create duplicate detachments.

**Pass:** severe localized damage has a visible/physical consequence rather than only a numeric condition loss.

## C. Front cooling and power consequence
1. Damage the FRONT zone below roughly half health without fully breaking the vehicle.
2. Drive with sustained throttle.
3. Observe `NATIVE_ROAD_DAMAGE_DYNAMICS`: `cooling=` should climb and `power_limit=` should fall.
4. Release throttle or drive gently and confirm cooling stress decays.
5. At very high cooling stress, continue pushing and confirm overall condition slowly worsens.

**Pass:** a smashed front end changes how long the player can push the vehicle and can turn reckless driving into a breakdown.

## D. Side damage and steering pull
1. Damage only LEFT or RIGHT significantly.
2. Drive straight and make controlled steering inputs.
3. Confirm `steering_limit` is lower and `steering_bias` is non-zero toward the damaged-side imbalance.
4. Damage the opposite side to a similar level and confirm the asymmetric bias reduces.

**Pass:** side damage affects steering rather than behaving like generic HP loss.

## E. Rear damage + wheel-state controller
1. Damage the REAR zone heavily while keeping tires usable.
2. Drive through a corner or loose surface where Chaos wheel-state risk is already non-zero.
3. Confirm `NATIVE_ROAD_WHEEL_STATE_EVIDENCE` reports `rear_body=` and total risk is higher than the equivalent undamaged setup.
4. Repeat with Mulebox cargo loaded and verify the vehicle remains controllable but more demanding.

**Pass:** rear structural damage composes with the existing wheel-state/traction layer instead of replacing it.

## F. Workshop + economy
1. Park the damaged Native Rattleback/Mulebox inside workshop range and exit the vehicle.
2. Interact with the workshop.
3. Confirm the charged price is base service plus a bounded body surcharge.
4. Confirm `NATIVE_ROAD_WORKSHOP_RESTORE` appears.
5. Verify condition, tire integrity, fuel, four body zones and cooling stress are restored; detached debris is reset/hidden.
6. Drive again and confirm power/steering/traction penalties are cleared.
7. Save/load or cycle takeover and confirm the compatibility mirror does not overwrite the freshly repaired Native snapshot with stale values.

**Pass:** workshop spending fixes the same Native road vehicle the player was driving and synchronizes the legacy save/garage mirror.

## G. Regression
- Traffic collision incidents, nearby crash response and hit-and-run escalation from 0.0.68/0.0.69 still trigger.
- World-object collisions still do not falsely create traffic-crime heat.
- Fuel depletion, condition breakdown, tire tuning, cargo load and Native takeover/fallback behavior remain intact.
- Fieldmaster workshop service remains unchanged.
- Roadmap remains 125/130 until genuine UE/Win64 runtime evidence exists.

## Demo gate
Do not publish a Demo Release from Project sanity alone. Require a real UE 5.8 Win64 package, successful packaged-EXE runtime smoke test, rendered visual acceptance, green relevant CI and no demo-critical blockers.
