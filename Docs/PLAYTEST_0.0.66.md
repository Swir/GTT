# GTT 0.0.66 Playtest — Native Road Fleet Takeover

## Goal
Verify the shared acceptance-gated takeover path for Rattleback 82 and Mulebox 1200, including state handoff, safe fallback and the existing Mulebox cargo/economy loop.

## Scenario A — acceptance gate
1. Run with no valid authored road-vehicle skeletal rig or Physics Asset.
2. Confirm native candidates stay hidden/non-colliding and log `NATIVE_ROAD_WAIT` rather than replacing the playable legacy vehicles.
3. Supply an authored Rattleback or Mulebox rig satisfying the canonical bones/sockets, wheel setup, powertrain and Physics Asset contract.
4. Confirm `NATIVE_ROAD_ACCEPTED vehicle=...` and, for an owned unoccupied legacy vehicle, `NATIVE_ROAD_TAKEOVER_ACTIVE vehicle=...`.

Expected: takeover is opt-in through real runtime acceptance, not a forced source-level assumption.

## Scenario B — garage state handoff
1. Own Rattleback 82, damage it, consume fuel and install at least one engine/tire upgrade.
2. Allow Native takeover.
3. Enter, drive and exit the native vehicle.
4. Save, load and inspect the legacy mirror/garage state.

Expected: condition ratio, fuel liters, ownership, tuning and tire integrity survive takeover and mirror sync. A 50% legacy vehicle must retain roughly half-condition power response; condition is a 0..1 ratio, not a 0..100 value.

## Scenario C — road vehicle handling
1. Compare a healthy/tuned Rattleback with a damaged/low-tire-integrity state.
2. Confirm engine tuning affects requested Native Chaos power and tire state/tuning affects steering authority.
3. Drain fuel to zero.

Expected: fuel burn is active while occupied, an empty tank cuts throttle and applies brake, and existing persistent vehicle state remains the source of gameplay consequences.

## Scenario D — Native Mulebox cargo loop
1. Own Mulebox 1200 and activate accepted Native takeover.
2. Start the legal farm cargo job and drive the native Mulebox to FEED DEPOT.
3. Load cargo and confirm `NATIVE_ROAD_CARGO vehicle=Mulebox1200 load=1.00`.
4. Drive above the existing cargo thresholds and confirm reduced power/high-speed steering authority.
5. Deliver to HILL FARM and verify `MULEBOX ROLE BONUS` is still paid.
6. Repeat with deliberate failure and confirm load returns to zero.

Expected: Native takeover does not break the real farm-job/economy loop introduced in 0.0.65.

## Scenario E — runtime fallback
1. During an accepted takeover, invalidate one required native runtime contract item in a development build (wheel setup, powertrain, rig/Physics Asset).
2. Wait for the runtime guard interval.
3. Confirm `NATIVE_ROAD_FALLBACK vehicle=...` and restoration of the synchronized legacy vehicle.

Expected: the player is never left with a known-invalid Native road vehicle.

## Demo gate
This is a source-level milestone and runtime acceptance design. Do not publish a Demo Release from this result alone. A real UE 5.8 Win64 package, packaged EXE smoke test, green relevant Actions and rendered visual acceptance are still mandatory.
