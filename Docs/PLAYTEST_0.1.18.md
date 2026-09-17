# GTT 0.1.18 Playtest — Runtime acceptance closure

This playtest is for the next qualifying UE 5.8 Win64 package. Source CI validates the contract only; it does not satisfy the runtime gates.

## Evidence prerequisites

1. `WIN64_PREFLIGHT.json` = PASS and proves Windows x64, UE 5.8, UBT/UAT, MSVC, Windows SDK, Chaos Vehicles and adequate disk.
2. `BUILD_ATTEMPT.json` = PASS with UAT exit code 0 and the exact source SHA.
3. `RUNTIME_SMOKE.json`, `DEMO_SCENARIO.json`, `GAMEPLAY_SMOKE.json`, and `NATIVE_CHAOS_RUNTIME.json` all PASS.
4. `NATIVE_DRIVETRAIN_SCENARIO.json` PASS proves automatic forward upshift, high-speed reverse interlock, safe reverse commit, measured reverse motion, safe forward return, and zero diagnostic failures.
5. `NATIVE_TRAILER_RUNTIME.json` PASS proves the final authored trailer rig was actually active in packaged runtime.

## Authored trailer runtime checks

- Observe at least two `AUTHORED_TRAILER_RUNTIME_EVIDENCE` samples with `active=1` while attached to the Native Fieldmaster (`nativeTow=1`).
- In at least two samples, dual wheel contact must be proven: both authored wheel contacts must be present (`contacts=1.0`, `left=1`, `right=1`).
- In at least two samples, hitch alignment must remain inside the existing 80 cm warning envelope and `warning=0`.
- Exercise loaded and unloaded handling if the demo route exposes both states. Confirm the trailer does not visibly tunnel, hover, jackknife uncontrollably, or detach without a gameplay cause.
- Confirm legacy greybox presentation remains the safe fallback when no valid authored rig exists; such a fallback must not count as final authored trailer acceptance.

## Integrated candidate checks

- Re-run the complete 33-step deterministic demo scenario and verify police, roadblock, spike consequence, post-spike escape, save/load, structural limp-home and recovery remain green.
- Verify `DEMO_TECHNICAL_GATE.json` is schema 6 and records both `deterministic_drivetrain=PASS` and `authored_trailer_runtime=PASS`.
- Verify every runtime manifest binds to the exact packaged Git SHA.
- Perform rendered visual acceptance separately: world composition, vehicles, trailer, character/weapons, HUD, lighting, atmosphere, and absence of placeholder text clutter.

## Roadmap rule

The five remaining runtime/hardware Roadmap items must remain open until a real UE 5.8 Win64 package and the relevant runtime evidence exist. In particular, `Authored skeletal trailer wheel assets and final hitch sockets` must remain open unless the packaged authored rig itself passes this runtime gate and visual inspection.
