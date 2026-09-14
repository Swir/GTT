# GTT 0.0.56 — Native Heavy-Haul Load & Sway Dynamics

## Goal
Connect the Native Fieldmaster stability layer to the real articulated farm-trailer state so cargo mass, hitch stretch, trailer damage, axle loss and trailer sway materially affect driving during Heavy Timber Haul.

## What changed
- Added a canonical trailer `GetTowLoadFactor()` derived from the existing live trailer/cargo state rather than a duplicate heavy-haul stat.
- Loaded cargo contributes the largest base tow load; hitch stretch, trailer damage, missing wheels and shifted/damaged cargo progressively increase the load factor.
- Native Fieldmaster stability now discovers the actually attached native trailer and measures yaw divergence plus trailer-vs-tractor lateral relative velocity as sway evidence.
- Heavy loads lower the intervention speed threshold from normal road-speed protection to a more conservative towing threshold.
- Sustained trailer sway cuts throttle and applies load/sway-proportional braking after a short grace period; severe sustained sway can also neutralize steering to stop the driver amplifying a fishtail.
- Existing tire upgrades still reduce intervention slightly, so tuning progression remains meaningful while never bypassing severe safety intervention.
- `NATIVE_STABILITY_EVIDENCE` now records `tow_load`, `sway_risk`, sway duration and trailer attachment state.

## Runtime playtest
1. Launch UE 5.8 with an accepted authored Native Fieldmaster rig and activate native takeover.
2. Drive without a trailer and confirm `tow_load=0`, `sway_risk=0` and baseline 0.0.55 handling remains unchanged.
3. Attach the Heavy Timber Haul trailer empty. Confirm a modest non-zero tow load appears without spurious braking on straight road.
4. Load timber at North Wood Yard. Confirm tow load rises substantially and intervention begins at a lower unsafe-speed threshold than when unloaded.
5. On a safe wide test road, induce a controlled trailer weave. Confirm yaw/lateral motion raises `sway_risk`; a brief wiggle stays inside the grace period, while sustained sway cuts throttle and adds braking.
6. Damage trailer condition and repeat. Confirm the same maneuver produces higher tow load and earlier/stronger intervention.
7. Break one axle/wheel in an editor-safe setup and repeat at low speed. Confirm tow load reflects the axle penalty and handling becomes appropriately conservative.
8. Damage cargo integrity and verify shifted cargo contributes additional load without resetting mission stage, payout state or trailer damage.
9. Repair roadside and confirm restored axle/trailer condition lowers tow load again while previously lost cargo integrity remains meaningful.
10. Detach the trailer. Confirm trailer influence immediately disappears and the Native Fieldmaster returns to normal contact/attitude stability behavior.

## Acceptance limits
- Repository sanity proves source wiring only; it does not prove real Chaos tire forces, authored suspension response or packaged Win64 behavior.
- This milestone does not close the Native Chaos roadmap checkbox until UE 5.8 runtime evidence confirms the authored drivetrain/suspension/wheel setup behaves correctly under load.
- Demo remains blocked until a verified Win64 package, packaged EXE runtime smoke, rendered visual acceptance and green relevant Actions exist.
