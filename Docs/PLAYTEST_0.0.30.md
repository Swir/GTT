# GTT 0.0.30 — Chaos Migration Foundation Validation

This milestone prepares the native Chaos Vehicles conversion without pretending that the existing static-mesh pawns have already become fully simulated Chaos vehicles.

## Source/readiness validation

1. Confirm `ChaosVehiclesPlugin` remains enabled in `GTT.uproject`.
2. Confirm the `ChaosVehicles` module remains in `GTT.Build.cs`.
3. Confirm `GTTChaosVehicleSpec` exposes canonical Fieldmaster, Rattleback and Mulebox target specifications.
4. Confirm every specification uses the exact persistent vehicle ID already used by save/load.
5. Confirm all three profiles contain mass, torque/RPM, final drive, steering angle, front/rear wheel dimensions, suspension values, friction and explicit gear ratios.

## Existing gameplay regression

Until the native skeletal/physics migration is compiled and driven in UE 5.8, the current vehicle path must remain unchanged. Run the existing vehicle matrix:

- enter/exit Fieldmaster, Rattleback and Mulebox with keyboard and controller;
- verify fuel consumption, empty-tank shutdown and refueling;
- damage each vehicle and verify condition power loss, smoke, detached panels and repair;
- damage tires and verify grip loss plus workshop tire repair;
- apply engine/tire tuning and verify the existing drivetrain response persists after save/load;
- drive through mud and verify drag/tire wear;
- hit a police spike strip and verify shared tire damage;
- recall owned vehicles through numbered garage slots;
- tow the farm trailer with the Fieldmaster and run Heavy Timber Haul.

## Native Chaos acceptance matrix for the next stage

When the `AWheeledVehiclePawn` implementation and authored skeletal/physics assets land, repeat the matrix above plus:

- stationary wheel contact and suspension settling;
- full-lock steering without wheel/body collision;
- forward/reverse gear transitions;
- hill start and low-speed tractor torque;
- Rattleback high-speed stability and braking;
- Mulebox loaded handling;
- Fieldmaster mud and heavy-trailer pull;
- loss of grip after tire damage;
- no loss of persistent ownership/tuning/fuel/condition after quit/relaunch.

## Acceptance boundary

Repository CI validates the migration contract and catches accidental ID/profile/plugin drift. **Full Unreal Engine 5.8 compile/package/runtime testing is still required**, and this milestone makes **no packaged EXE verification** claim. The two Native Chaos roadmap tasks stay open until a true Chaos pawn, authored skeletal/physics assets and runtime acceptance are complete.
