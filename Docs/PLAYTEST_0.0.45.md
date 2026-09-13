# GTT 0.0.45 — Fieldmaster Native Runtime Takeover

This milestone completes the source-level gameplay lifecycle needed for an authored Rusty Fieldmaster 60 Native Chaos pawn to replace the legacy physics pawn without breaking the existing sandbox state. It still does **not** claim runtime acceptance until a real skeletal/Physics Asset build is compiled and driven in UE 5.8.

## Takeover acceptance

1. Start with the current legacy Rusty Fieldmaster 60 before ownership is unlocked. The native pawn must remain hidden, collision-disabled and unable to take over.
2. Verify an incomplete skeletal rig, missing required sockets, missing Physics Asset, invalid wheel setup or invalid powertrain leaves `IsNativeFieldmasterReady()` false and the legacy tractor fully usable.
3. Complete Borrowed Tractor / otherwise mark the legacy Fieldmaster owned.
4. With a fully authored accepted native pawn present, wait for the takeover retry. Confirm the native pawn moves to the exact legacy transform, becomes visible/collidable, and the legacy actor becomes hidden/non-colliding with its tick disabled.
5. Confirm there is never a frame in which both drivetrains are intentionally active for player control.

## Enter / drive / exit

1. Interact with the active native Fieldmaster and verify the existing player pawn is attached to `driver_seat`, hidden and collision-disabled before possession transfers.
2. Verify the existing `VehicleThrottle`, `VehicleSteer`, `ExitVehicle`, `RadioNext`, `QuickSave` and `QuickLoad` mappings remain usable.
3. Verify forward and reverse gear intent, steering release, braking when throttle is released and zero-throttle behavior at an empty fuel tank.
4. Exit and verify the player reappears at the authored `driver_exit` socket (fallback offset only if the socket unexpectedly cannot be resolved).
5. Verify camera boom/view remains usable at 720p and 1080p.

## Existing gameplay state

1. Before takeover, set non-default condition, fuel, engine upgrade, tire upgrade and tire integrity on the legacy Fieldmaster.
2. Confirm takeover imports all values and ownership under persistent ID `RustyFieldmaster60`.
3. Drive long enough to confirm native fuel use decreases the migrated fuel state.
4. Confirm wanted/economy/radio lookups still resolve through the hidden player pawn while the native vehicle is possessed.
5. Quick-save while driving. The compatibility mirror must hold the native transform/state so existing garage/save serialization remains valid.
6. Quick-load: native possession must exit safely, legacy state must become loadable, and takeover may reactivate only after the restored legacy vehicle is again owned and native-ready.

## Garage recall

1. Use the normal Fieldmaster garage slot while native takeover is active.
2. The slot label may identify the active native Fieldmaster while retaining the existing persistent ordering.
3. Recall must move the **native** pawn to the bay and synchronize its legacy compatibility mirror.
4. Recall remains blocked while the Fieldmaster is occupied, while police are looking for the player, or while the game warden alert is active.
5. Rattleback and Mulebox garage recall must behave exactly as before.

## Fallback regression

- Remove/disable the authored native Fieldmaster actor and verify the legacy Rusty Fieldmaster remains the complete playable fallback.
- No native actor may become visible merely because the C++ class exists.
- Borrowed Tractor theft/wanted flow remains on legacy until ownership is obtained; native takeover intentionally starts only after ownership so the existing theft mission semantics are not bypassed.
- Heavy haul, trailer hitch, tuning, repair, service, police, ranger, NPC, jobs and story missions must remain operational.

## DEMO / Win64 gate

Source sanity is not enough. Before a demo Release, run the 0.0.43 Win64 evidence pipeline on an actual self-hosted Windows x64 Unreal Engine 5.8 runner, obtain a packaged `GTT.exe`, pass runtime smoke, then perform a normal rendered visual/gameplay review. Keep the Native Chaos roadmap items open until that evidence exists.
