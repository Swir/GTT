# GTT 0.1.13 — Dedicated Native Chaos Tractor Movement

This milestone moves the Rusty Fieldmaster 60 from a generic `AWheeledVehiclePawn` movement path to a dedicated `UGTTFieldmasterChaosMovementComponent` while deliberately preserving the legacy vehicle as a persistence/fallback mirror until a real Unreal/Win64 packaged runtime test proves the takeover safe.

## Source/CI acceptance

1. Confirm `AGTTFieldmasterNativePawn` replaces `AWheeledVehiclePawn::VehicleMovementComponentName` with `UGTTFieldmasterChaosMovementComponent` through `FObjectInitializer::SetDefaultSubobjectClass`.
2. Confirm the dedicated movement component owns canonical Fieldmaster wheel setup, powertrain configuration and validation.
3. Confirm throttle, steering, braking and forward/reverse gear commands are issued by the dedicated movement component rather than directly by the pawn.
4. Confirm `GTT.Build.cs` still depends on `ChaosVehicles` and no protected third-party vehicle code/assets were introduced.
5. Run `python Scripts/verify_fieldmaster_dedicated_chaos_movement.py`.
6. Run the full `Project sanity` workflow and require all earlier native takeover, wheel, drivetrain, terrain, damage, trailer and packaged-demo contract checks to remain green.

## Unreal Editor runtime playtest

These are required before the roadmap checkbox can be closed; source sanity alone is not runtime evidence.

1. Open the project in the repository-supported Unreal Engine build and load the normal sandbox world.
2. Obtain ownership of the Rusty Fieldmaster 60 so the native takeover path is eligible.
3. Verify the legacy tractor becomes hidden/disabled only after the dedicated native pawn reports a valid rig, canonical wheels, canonical powertrain and Physics Asset.
4. Enter the tractor and drive forward, reverse and through full steering travel. Confirm the pawn remains possessed and no duplicate legacy collision body is active.
5. Release throttle on level ground. Confirm the dedicated movement's idle brake settles the tractor instead of leaving a stale throttle/gear command.
6. Deplete fuel to zero while moving. Confirm effective throttle becomes zero and full brake is requested; refuel and confirm drive authority returns without respawning the vehicle.
7. Reduce vehicle condition. Confirm the same requested throttle produces less effective Chaos throttle while steering remains controllable.
8. Damage tires and drive through authored mud. Confirm steering authority is reduced by tire integrity and native terrain grip rather than by a second unrelated steering model.
9. Exit/re-enter, quick-save, quick-load and garage-recall the tractor. Confirm commands reset safely and the legacy persistence mirror retains transform, fuel, condition, ownership and tuning state.
10. Attach the farm trailer and repeat forward/reverse/turning tests. Confirm the native rear hitch path and existing trailer safety logic remain functional.
11. Strike a world prop at low and then higher speed. Confirm existing native collision damage still feeds the shared condition/tire systems and therefore the dedicated movement response.
12. Drive through at least one legal farm/heavy-haul activity and one wanted/police situation to confirm gameplay systems still recognize the native takeover actor.

## Dedicated Native Chaos Tractor Movement acceptance evidence

The roadmap item **Dedicated native Chaos wheeled tractor movement** remains open until all of the following are captured from a real Unreal runtime:

- successful project compile for the supported Windows target,
- native Fieldmaster spawning with the dedicated movement subclass,
- wheel contacts and suspension active on all four wheels,
- forward/reverse/brake/steer behavior observed in runtime,
- no duplicate legacy collision/physics actor during takeover,
- save/load and garage recall survive the takeover,
- no demo-critical errors in the Unreal log.

## Win64 packaged demo gate

A source-only GitHub Actions pass is **not** a packaged build. Do not publish a demo from this milestone unless the existing Windows pipeline also produces a real packaged Win64 artifact, launches that packaged executable in the runtime smoke path, records the required evidence, and the visual acceptance pass confirms the current world/HUD/vehicles/NPCs are presentation-ready. If the runner or Unreal installation is unavailable, leave the Windows/demo roadmap tasks open.
