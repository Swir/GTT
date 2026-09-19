# GTT 0.1.57 — Fieldmaster Native Condition Scale Playtest Contract

This contract protects the Native Chaos Fieldmaster condition boundary while the real Win64 runtime gate remains open.

## Source acceptance

- A healthy migrated Fieldmaster uses normalized condition `0.0..1.0`.
- Final Native Chaos drivetrain authority uses `0.08` as the 8% critical threshold.
- Healthy condition is not divided by `100` before top-speed/governor composition.
- Runtime impact damage continues clamping condition to `0.0..1.0`.
- Source telemetry converts the ratio to percent only for human-readable logging.

## Future packaged Win64 witness

On the qualifying Unreal Engine 5.8 Windows runner, verify the same candidate SHA with:
1. owned healthy Fieldmaster takeover active;
2. throttle engages Native Chaos authority instead of permanent critical braking;
3. 100% health reports approximately `condition_pct=100.0`;
4. damage below 8% condition triggers the critical stop;
5. recovery above 8% allows drivetrain authority again;
6. forward/reverse interlock, axle traction and suspension evidence remain intact.

The roadmap checkbox remains open until that packaged runtime evidence and visual acceptance exist.
