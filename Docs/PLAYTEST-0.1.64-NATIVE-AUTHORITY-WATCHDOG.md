# GTT 0.1.64 — Native Chaos Authority Watchdog Playtest

## Scope

Validate that the Rusty Fieldmaster 60 has exactly one live driving authority after native takeover. The native Chaos pawn must remain visible/collidable with the dedicated `GTTFieldmasterChaosMovementComponent`, while the legacy mirror stays hidden, collision-disabled and tick-disabled. This milestone hardens gameplay reliability and evidence quality; it does **not close** a Native Chaos, authored-trailer or Win64 roadmap blocker without real packaged proof.

## Required setup

- Unreal Engine 5.8 Windows build of the exact candidate SHA.
- Owned Rusty Fieldmaster 60 plus the native Fieldmaster pawn.
- A PhysicsAsset and accepted native rig so takeover can activate normally.
- Runtime log capture enabled.
- For release evidence, a packaged Win64 candidate that already produced `BUILD_INFO.json` and a PASS `RUNTIME_SMOKE.json`.

## Acceptance scenarios

1. **Normal takeover:** enter the owned Fieldmaster. The native pawn must be visible/collidable, the legacy mirror must be hidden with collision/tick disabled, and driving input must move only the native Chaos vehicle.
2. **Authority evidence:** keep the vehicle active long enough to produce at least two `NATIVE_FIELDMASTER_RUNTIME_TELEMETRY` samples. Each must report `authority=NATIVE_CHAOS`, `takeover_integrity=PASS`, `movement_class=GTTFieldmasterChaosMovementComponent`, `legacy_mirror=QUIESCENT`, `native_collision=YES` and `physics_asset=YES`.
3. **Legacy visibility fault injection:** while takeover is active in a development build, deliberately re-show the legacy Fieldmaster. The watchdog must emit `NATIVE_FIELDMASTER_AUTHORITY_FAULT ... reason=LEGACY_MIRROR_VISIBLE action=RESTORE_LEGACY`, deactivate native takeover and restore the legacy actor instead of permitting two authoritative vehicles.
4. **Legacy collision fault injection:** re-enable legacy collision during takeover. The same fail-closed `RESTORE_LEGACY` response must occur before further authoritative telemetry is emitted.
5. **Legacy tick fault injection:** re-enable the legacy actor tick. The watchdog must fail closed rather than allow both gameplay paths to update simultaneously.
6. **Native collision fault injection:** disable native actor collision during takeover. The watchdog must report `NATIVE_COLLISION_DISABLED` and restore legacy authority.
7. **Movement/asset guard:** force an inactive/wrong movement path or remove the PhysicsAsset in a development-only test. Takeover evidence must not claim PASS; the watchdog must restore legacy authority.
8. **Recovery:** after correcting the injected fault, allow the existing retry path to re-run. Native takeover may reactivate only after the ordinary native readiness checks pass again.
9. **Driving regression:** after healthy takeover, repeat forward/reverse steering, braking, 0.1.59 loaded authority caps, 0.1.62 hill-haul control and 0.1.63 trailer-brake heat/fade/recovery. The watchdog must not alter healthy driving behavior.
10. **Save/garage regression:** save, load, exit, re-enter and recall the Fieldmaster. Persistent condition/fuel/tuning state must continue mirroring through the existing legacy persistence bridge.

## Packaged evidence

Run `Scripts/evaluate_native_authority_runtime.ps1` against the exact candidate package and runtime log. A PASS requires:

- Win64 `BUILD_INFO.json` with the expected candidate SHA;
- PASS packaged runtime smoke;
- at least two healthy Native Chaos authority telemetry samples;
- the exact dedicated Fieldmaster movement class;
- quiescent legacy mirror + active native collision + PhysicsAsset evidence;
- zero `NATIVE_FIELDMASTER_AUTHORITY_FAULT` entries.

The evaluator writes `NATIVE_AUTHORITY_RUNTIME.json`. This is additive evidence: the existing Native Chaos runtime gate still owns wheel contacts, suspension, movement, drivetrain and fallback rejection.

## Demo / roadmap truth

The roadmap remains **125 / 130 (96.2%)** after source acceptance. Demo stays **NOT READY** until the same candidate completes the real UE 5.8 Win64 package, packaged EXE smoke, Native Chaos/trailer runtime gates and human visual acceptance.
