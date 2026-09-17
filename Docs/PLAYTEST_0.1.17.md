# GTT 0.1.17 — Deterministic Native Gear/Reverse Runtime Scenario

This milestone turns the packaged Fieldmaster drivetrain from passive telemetry into an active, deterministic runtime exercise. It is intentionally scheduled after the existing core demo smoke scenario so the two automated control passes cannot fight over the same Native Chaos pawn.

## Preconditions

- Use the exact UE 5.8 Win64 package SHA under test.
- `BUILD_INFO.json`, `RUNTIME_SMOKE.json`, `NATIVE_CHAOS_RUNTIME.json` and the runtime log must all come from that same package/run.
- The core `-GTTDemoSmokeScenario` pass must remain alive for at least 125 seconds.
- Native Fieldmaster takeover must be available and `NATIVE_CHAOS_RUNTIME.json` must already be `PASS`.
- Do not close any Roadmap runtime checkbox from source sanity alone.

## Runtime scenarios

1. **Late-start isolation** — confirm `NATIVE_DRIVETRAIN_SCENARIO_BEGIN` is present, but the drivetrain exercise does not begin before 76 seconds. The original 0–75 second demo controls must retain exclusive ownership of their phase.
2. **Automatic forward shift** — the evidence pass requests first forward gear once, then holds forward throttle without repeatedly re-sending gear 1. A real automatic upshift to gear 2 or higher must be observed at measurable forward speed.
3. **Forward-speed floor** — automatic-shift evidence must be captured at at least 6 km/h so a transient/idle gear value cannot satisfy the gate.
4. **Reverse interlock** — after the forward run, reverse intent is staged while the tractor is still travelling above the 3.5 km/h release threshold. The current forward gear is held while braking.
5. **Safe reverse commit** — reverse target gear may be written only after absolute longitudinal speed falls into the 3.5 km/h safe window (3.75 km/h evaluator tolerance).
6. **Measured reverse motion** — after the safe commit, the Fieldmaster must actually travel backwards in a reverse gear and reach at least 5 km/h reverse speed.
7. **Reverse braking** — throttle must be removed and deterministic braking applied before returning to a forward gear.
8. **Safe forward return** — forward target gear may be restored only after absolute speed returns to the same safe shift window.
9. **Measured forward return** — after the reverse phase the tractor must genuinely move forward again in a positive gear, proving the gearbox did not remain stuck in reverse/neutral.
10. **Three transition writes only** — inspect source/runtime behavior: one initial forward selection, one safe reverse commit and one safe return-to-forward commit. There must be no per-tick target-gear spam.
11. **Diagnostic failure honesty** — force or inspect a failed phase. `phase=DIAGNOSTIC result=FAIL` must prevent the final scenario result from being accepted even if later phases recover.
12. **Global deadline** — the dedicated sequence must finish by 122 seconds so evidence is emitted before the existing 125-second packaged smoke minimum can terminate the process.
13. **Native gate dependency** — `evaluate_drivetrain_scenario.ps1` must refuse acceptance when `NATIVE_CHAOS_RUNTIME.json` is not `PASS` or belongs to another SHA.
14. **Evidence manifest** — a passing run must create `NATIVE_DRIVETRAIN_SCENARIO.json` with schema `gtt.native-drivetrain-scenario.v1`, measured shift speeds, reverse/forward velocities, observed gears and zero diagnostic failures.
15. **Unsafe reverse rejection** — edit/inject evidence showing a reverse commit above 3.75 km/h; evaluator must return non-zero and preserve a FAIL manifest.
16. **Unsafe forward rejection** — repeat for the reverse-to-forward commit above 3.75 km/h; evaluator must fail.
17. **Win64 candidate integration** — the self-hosted UE 5.8 workflow must run the drivetrain evaluator after Native Chaos runtime evaluation, require the manifest during evidence re-validation and upload it in both success and failure diagnostics when available.
18. **Existing gameplay regression** — re-run deterministic demo, Native Chaos telemetry and automatic drivetrain source gates. Police/roadblock, damage, persistence, workshop, ROAD/CARGO and the rest of the current sandbox loop must remain unchanged.
19. **Rendered visual acceptance** — this milestone is technical drivetrain evidence only. A passing JSON manifest does not replace visual review of world presentation, vehicles, HUD, characters, lighting or the final demo slice.
20. **Demo honesty** — do not publish a Demo Release until the complete UE 5.8 Win64 package/runtime chain and visual acceptance are genuinely green.

## Expected evidence

A qualifying package produces a `NATIVE_DRIVETRAIN_SCENARIO.json` PASS tied to the same `git_sha` as `BUILD_INFO.json` and the already-passing `NATIVE_CHAOS_RUNTIME.json`. The route must be `forward-auto-reverse-forward`, automatic gear 2+ must be observed, reverse must be physically measurable, both direction commits must occur within the safe window, and no diagnostic failure marker may exist.

This document does not itself close a Roadmap checkbox. Until a real qualifying Win64/UE 5.8 artifact executes these steps, the project remains at the existing runtime-gated completion state.
