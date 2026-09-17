# GTT 0.1.16 — Automatic Drivetrain & Direction Safety Playtest

This pass targets the real Native Chaos drivetrain path introduced before demo acceptance. It must be run against an Unreal Engine 5.8 Win64 package before any runtime Roadmap checkbox is closed.

## Preconditions

- Use the exact commit/package SHA under test.
- `WIN64_PREFLIGHT.json`, `BUILD_ATTEMPT.json`, `RUNTIME_SMOKE.json` and `DEMO_SCENARIO.json` must refer to that same SHA.
- Native Fieldmaster takeover must be active with no `NATIVE_PHYSICS_FALLBACK` marker.
- Keep `NATIVE_CHAOS_RUNTIME.json` from the same packaged run.

## Runtime scenarios

1. **Forward launch** — enter Rusty Fieldmaster 60 from rest and hold forward throttle. The tractor must engage a forward gear and move through Native Chaos without legacy fallback.
2. **Automatic upshift freedom** — continue accelerating on a clear straight. The automatic transmission must be free to leave first gear when Chaos shift thresholds are reached; holding forward input must not continuously force gear 1.
3. **No per-frame target-gear spam** — while cruising in a forward gear, input may change throttle/steer/brake but should not repeatedly emit direction gear commands unless the transmission is neutral/opposite or a direction change is actually committed.
4. **Forward-to-reverse interlock** — while moving forward above 3.5 km/h, request reverse. Throttle must cut, braking must rise and the current forward gear must be retained while speed is reduced.
5. **Safe reverse commit** — keep reverse requested through the stop. Reverse may be committed only inside the 3.5 km/h release window; `NATIVE_DIRECTION_SHIFT_COMMIT` must not show a higher absolute speed.
6. **Reverse-to-forward interlock** — repeat the same test from reverse travel into a forward request. No high-speed sign flip or driveline snap is acceptable.
7. **Neutral/coast behavior** — release throttle at speed. Existing engine-brake behavior must remain active and low-speed stationary hold must still prevent unwanted creep.
8. **Condition damage** — repeat forward acceleration with degraded tractor condition. Effective throttle must remain condition-limited and the drivetrain must still shift safely.
9. **Low tire / poor terrain grip** — use degraded tires or a low-grip surface. Steering authority must remain reduced by the existing tire/terrain factors while direction changes continue to obey the shared drivetrain interlock.
10. **Trailer load** — attach the farm trailer with meaningful tow load, accelerate and brake. Automatic gear operation must remain stable and telemetry must retain trailer/tow-load evidence.
11. **Rattleback parity** — drive Rattleback 82 and verify the same final drivetrain authority does not pin the automatic transmission to first gear and still protects forward/reverse swaps.
12. **Mulebox parity** — repeat the shared-authority checks with Mulebox 1200, including a loaded CARGO state when available.
13. **Save/load takeover** — save while the Native Fieldmaster is valid, reload and re-enter. Takeover, automatic transmission configuration and direction safety must recover without a stale forced gear.
14. **Packaged telemetry** — `NATIVE_FIELDMASTER_RUNTIME_TELEMETRY` must contain `signed_speed_kmh`, `automatic_gears=YES`, a multi-gear `forward_gears` count, wheel contacts and complete suspension telemetry.
15. **Automatic gearbox evidence** — inspect `NATIVE_AUTOMATIC_GEARBOX_EVIDENCE` / `NATIVE_AUTOMATIC_GEAR_SHIFT` when player-controlled drivetrain authority is active. A higher forward gear is useful evidence but is not fabricated if the deterministic route never reaches its shift threshold.
16. **Runtime evaluator safety** — `NATIVE_CHAOS_RUNTIME.json` must report zero `unsafe_direction_shift_commits`, automatic-gear telemetry samples, configured forward-gear count, highest observed forward gear and the existing Native Physics/wheel/suspension acceptance fields.
17. **Demo gate honesty** — source sanity alone is not enough. Do not close Roadmap items or publish a demo until UE 5.8 Win64 package/runtime smoke and rendered visual acceptance have actually passed.

## Expected evidence

A qualifying packaged run should preserve the existing `gtt.native-chaos-runtime.v1` schema while adding backward-compatible fields for automatic transmission and direction safety. The evaluator must fail on an unsafe direction-shift commit, disabled automatic gears, or a single-forward-gear runtime configuration. Absence of a high forward-gear observation by itself is not a failure if the deterministic route never reaches the configured shift threshold.
