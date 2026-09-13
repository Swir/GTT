# GTT 0.0.44 — Fieldmaster Native Gameplay Handoff

This milestone connects the accepted Native Chaos Fieldmaster pawn to the existing gameplay state and player input contract without claiming runtime activation before authored assets and a real UE 5.8 package exist.

## Native control handoff

1. Spawn the authored Rusty Fieldmaster 60 native pawn in an Unreal-capable build.
2. Verify the pawn refuses throttle and steering while `IsNativeFieldmasterReady()` is false.
3. With the required skeletal rig, sockets, Physics Asset, canonical wheels and powertrain present, verify the native acceptance gate succeeds.
4. Drive with the existing `VehicleThrottle` and `VehicleSteer` mappings on keyboard and gamepad.
5. Confirm steering direction, forward/reverse intent and input release do not produce stuck control values.

## Legacy gameplay state migration

1. Start from the existing legacy Rusty Fieldmaster 60 with non-default fuel, condition, tire integrity and upgrade levels.
2. Import its state through `ImportLegacyGameplayState`.
3. Confirm condition percent, fuel liters, ownership, engine upgrade, tire upgrade and tire integrity match the authoritative legacy values.
4. Verify an actor with another persistent vehicle ID is rejected rather than silently migrated as the Fieldmaster.
5. Exercise save/load and garage recall around the migration path and confirm the same persistent ID remains `RustyFieldmaster60`.

## Regression / fallback

- Without authored Native Chaos assets, the current legacy Fieldmaster remains the playable fallback.
- Native input must stay gated when acceptance fails; do not allow a half-configured pawn to move while legacy simulation is also active.
- Theft/wanted, garage ownership, tuning, fuel, damage and tire systems must retain their existing authoritative state until runtime migration is explicitly activated.
- Trailer attachment remains on the existing validated `rear_hitch`/fallback path.

## Win64 acceptance

Run the 0.0.43 Win64 evidence workflow on a real self-hosted Windows x64 Unreal Engine 5.8 runner. A public demo still requires package success, runtime smoke success and a rendered visual/gameplay review. Source sanity alone is not acceptance.

## Roadmap honesty

Keep the roadmap at 125/130 until the authored native Fieldmaster is actually compiled and driven in UE 5.8 and the dedicated Native Chaos drivetrain/suspension/wheel path is verified in runtime.
