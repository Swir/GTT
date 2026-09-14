# GTT 0.0.64 — Fleet Native Wheel-State & Traction Control

## Milestone goal
Move real Chaos `FWheelStatus` handling out of a Fieldmaster-only gameplay path and make wheel-state traction response part of the shared fleet bridge used by Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.

## What to verify in an Unreal-capable build
1. Enter each vehicle after its authored skeletal/physics rig passes the existing native bridge acceptance gates.
2. Confirm the bridge reports `RUNTIME 4/4` only after all four Chaos wheel states are valid.
3. Drive on a clean, flat road. Normal steering/throttle should remain unchanged when runtime risk is low.
4. Force one or more wheels to lose contact or induce wheel slip/skid. Confirm `NATIVE_FLEET_WHEEL_STATE_EVIDENCE` reports the correct persistent vehicle ID, contacts, slip/skid counts, suspension lengths and spring forces.
5. Sustain moderate slip. Throttle must be progressively limited; stronger risk may add a small brake assist rather than instantly freezing the vehicle.
6. Sustain severe wheel-state risk. Steering authority should be reduced, but never reversed or amplified.
7. Damage tires and repeat the same maneuver. Runtime risk should increase because the controller consumes the existing persistent tire integrity state.
8. Buy tire upgrades and repeat. The existing tire tuning level should modestly improve the intervention threshold/available throttle without bypassing severe protection.
9. Verify Rattleback 82 and Mulebox 1200 use the same controller through `UGTTChaosVehicleBridgeComponent`; no duplicate save/tuning state should appear.
10. Verify the Fieldmaster's specialized terrain/heavy-haul stability subsystem still remains available for tractor-specific rollover, trailer sway and hill-start behavior.

## Regression checks
- A vehicle without a complete native rig must stay on the proven legacy dynamics path.
- Incomplete/invalid Chaos wheel states must not be treated as verified runtime traction evidence.
- Fuel empty, engine off or zero condition still force full braking through the existing bridge safety path.
- Garage/save/workshop ownership, fuel, condition and tuning remain authoritative in `AGTTVehicleBase`.
- No roadmap Native Chaos checkbox is closed from source-level CI alone.

## Demo acceptance
This milestone is not sufficient for a public Windows demo. Demo still requires a real UE 5.8 Win64 package, successful packaged-EXE runtime smoke test, rendered visual acceptance and green relevant CI on the exact release commit.
