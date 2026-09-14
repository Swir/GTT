# GTT 0.0.77 Playtest — Native Command Composition & Suspension Runtime Evidence

## Scope

Verify that Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200 resolve drivetrain, axle and suspension safety through one final Native Chaos movement-command path rather than independent subsystems racing to overwrite throttle/brake/steering.

## Preconditions

1. Use an accepted Native takeover vehicle with its authored Physics Asset, wheel setup and powertrain accepted by the existing runtime gates.
2. Keep Output Log visible and filter for `NATIVE_COMMAND_COMPOSITION`, `NATIVE_AXLE_TRACTION` and `NATIVE_DIRECTION`.
3. Repeat the scenarios on Fieldmaster, Rattleback and Mulebox.

## Scenario A — normal four-wheel driving

1. Drive on flat road with healthy tires and four wheels grounded.
2. Confirm `NATIVE_COMMAND_COMPOSITION_EVIDENCE` reports `suspension_ready=YES` once all four Chaos wheel records expose valid suspension samples.
3. Confirm final throttle/steering remain close to upstream requests and no axle torque cut is active.
4. Confirm brake remains zero unless another existing system legitimately requests braking.

**Pass:** the composer is transparent during healthy driving.

## Scenario B — direction interlock plus axle slip

1. Accelerate above the 0.0.75 direction-shift release speed.
2. Request the opposite direction while simultaneously crossing mud/ditch geometry that creates wheel slip or unload.
3. Confirm direction interlock cuts final throttle.
4. Confirm axle torque cut cannot restore throttle.
5. Confirm final brake is at least the stronger of drivetrain-interlock brake and axle brake assist.
6. Confirm `NATIVE_COMMAND_COMPOSITION_LIMIT` reports the resolved values.

**Pass:** two simultaneous safety systems compose conservatively instead of last-writer-wins behavior.

## Scenario C — upstream governor/damage brake preservation

1. Use Fieldmaster condition/tire degradation or overspeed governor to request upstream braking.
2. Add moderate axle slip that requests a smaller brake assist.
3. Confirm final brake never drops below the existing stronger upstream brake.
4. Repeat on damaged Rattleback/Mulebox while their existing body/tire systems are active.

**Pass:** command composition preserves the strictest brake request and existing gameplay consequences.

## Scenario D — steering authority composition

1. Traverse a side slope/curb so left/right suspension load proxies diverge while at least two wheels remain grounded.
2. Confirm axle imbalance reduces final steering authority but never increases it beyond the upstream command.
3. Trigger direction interlock at the same time and verify steering remains bounded by the stricter reduction.
4. Return to balanced four-wheel contact and confirm normal steering returns naturally.

**Pass:** traction/imbalance can only remove steering authority, never create artificial turn force.

## Scenario E — authored suspension runtime evidence

1. On each Native vehicle, confirm all four `FWheelStatus` records become valid during runtime.
2. Drive over bumps/ditches and verify `suspension_samples=4` plus changing min/max normalized suspension travel in `NATIVE_AXLE_TRACTION_EVIDENCE`.
3. Intentionally break/misconfigure an authored wheel/suspension setup in a test branch or editor copy.
4. Confirm `suspension_ready=NO` rather than falsely claiming a complete Native suspension contract.

**Pass:** suspension readiness is derived from live Chaos wheel records, not a static source declaration.

## Scenario F — regression across game loops

- Fieldmaster: heavy-haul, mud, governor, rollover, load-transfer and breakdown remain active.
- Rattleback/Mulebox: body damage, detached panels, traffic incidents, police escalation, workshop and roadside recovery remain active.
- Mulebox cargo penalties remain upstream of final command safety.
- Workshop tire repair improves subsequent axle-risk behavior as before.

**Pass:** 0.0.77 consolidates command ownership without flattening vehicle roles or bypassing economy/damage systems.

## Release honesty

- Run `python Scripts/verify_native_command_composition.py` and the complete Project sanity workflow.
- Roadmap stays **125/130 (96.2%)** unless genuine Unreal runtime/build evidence closes an existing checkbox.
- This milestone does not prove a UE 5.8 Win64 package, packaged `GTT.exe` launch, runtime smoke test or rendered visual acceptance.
- Do not publish the first Demo Release until all technical and visual gates in the project release policy are genuinely satisfied.
