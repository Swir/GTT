# GTT 0.0.40 — Native Chaos Setup Contract Playtest

## Purpose
Verify the new fleet-wide canonical Chaos wheel setup gate without pretending that authored skeletal/physics assets or packaged Win64 runtime acceptance already exist.

## Source acceptance
1. Confirm `UGTTChaosNativeSetupLibrary` maps all three persistent vehicle IDs:
   - `RustyFieldmaster60`
   - `Rattleback82`
   - `Mulebox1200`
2. Confirm each mapping builds exactly four `FChaosWheelSetup` entries in FL/FR/RL/RR order.
3. Confirm front/rear wheel classes are vehicle-specific and use the existing native wheel classes.
4. Confirm wheel bone names come from `FGTTChaosRigContract`, not duplicated literals.
5. Confirm mechanical simulation is required and enabled by the canonical setup helper.

## Bridge acceptance
1. Spawn or author a test vehicle with `UChaosWheeledVehicleMovementComponent`.
2. Deliberately use the wrong wheel class or bone name: bridge must remain `WAITING`, legacy dynamics must stay enabled, and the setup summary must identify the mismatch.
3. Restore the canonical four-wheel setup but leave a required skeletal bone/socket missing: bridge must still remain on legacy dynamics.
4. Supply both a valid rig contract and the canonical wheel setup: only then may the bridge report native-ready and disable legacy dynamics.
5. Verify throttle, reverse, steering, braking, fuel gating, condition power loss and tire/tuning grip scaling still route through the existing bridge when native-ready.

## Fleet matrix
Repeat the acceptance gate for Fieldmaster 60, Rattleback 82 and Mulebox 1200. A setup from one vehicle must not be accepted for another vehicle ID.

## Trailer regression
For Fieldmaster/Mulebox rigs, retain `rear_hitch` validation and confirm trailer attachment still uses the validated native socket only after rig acceptance.

## Demo/release gate
This milestone is source/CI acceptance only. Do **not** mark Native Chaos roadmap tasks complete and do **not** publish a Windows demo until a real UE 5.8 Win64 compile/package plus packaged-EXE runtime smoke test and visual approval have passed.
