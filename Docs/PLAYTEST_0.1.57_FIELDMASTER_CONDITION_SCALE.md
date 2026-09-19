# GTT 0.1.57 — Fieldmaster Native Condition Scale Playtest Contract

This contract protects the Native Chaos Fieldmaster condition boundary while the real Win64 runtime gate remains open.

## Source acceptance

- A healthy migrated Fieldmaster is stored as normalized condition `0.0..1.0`.
- `ApplyMigrationSnapshot` accepts both the canonical `0.0..1.0` ratio and historical `0..100` ingress, then stores only the canonical ratio before any Native Chaos authority consumes it.
- Final Native Chaos drivetrain authority uses `0.08` as the 8% critical threshold.
- Healthy condition is not divided by `100` after the migration boundary or before top-speed/governor composition.
- Runtime impact damage continues clamping condition to `0.0..1.0`.
- Migration/import summaries convert the stored ratio to percent only for human-readable output.
- Engine and tire upgrade ingress is bounded to the supported `0..3` range before native runtime use.
- Native impact/mud/hitch methods have exactly one out-of-line definition across Fieldmaster translation units.
- `GTTFieldmasterNativeEnvironment.cpp` is the canonical environment/runtime implementation owner; the obsolete duplicate `GTTFieldmasterNativePawnRuntime.cpp` must remain absent so a Win64 link cannot fail on duplicate symbols.

## Future packaged Win64 witness

On the qualifying Unreal Engine 5.8 Windows runner, verify the same candidate SHA with:
1. owned healthy Fieldmaster takeover active after an ordinary legacy-state import;
2. a compatibility migration snapshot carrying `100` condition normalizes to `1.0` before runtime authority and survives the mirror/save roundtrip at full health;
3. throttle engages Native Chaos authority instead of permanent critical braking;
4. 100% health reports approximately `condition_pct=100.0`;
5. damage below 8% condition triggers the critical stop;
6. recovery above 8% allows drivetrain authority again and persists across mirror/save/load;
7. forward/reverse interlock, axle traction and suspension evidence remain intact;
8. the candidate compiles and links the Fieldmaster native pawn with one implementation surface for hit, mud, damage and rear-hitch behavior.

The roadmap checkbox remains open until that packaged runtime evidence and visual acceptance exist.
