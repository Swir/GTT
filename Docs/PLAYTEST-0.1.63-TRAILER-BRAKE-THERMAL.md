# GTT 0.1.63 — Trailer Brake Thermal Control Playtest

## Scope

Validate the new Fieldmaster trailer-brake thermal model as an extension of the existing 0.1.62 hill-haul loop. This playtest does **not** replace packaged Win64 qualification and does not close Native Chaos, authored trailer or release gates by itself.

## Required setup

- Rusty Fieldmaster 60 using the dedicated native Chaos movement path.
- Attached farm trailer with a meaningful load (`tow load >= 0.15`).
- A repeatable downhill segment of at least 4 degrees where the combination can hold 10–30 km/h.
- Blueprint/debug readout for `GetTrailerBrakeHeat01`, `GetTrailerBrakeAuthority`, `IsTrailerBrakeFadeActive`, `IsTrailerBrakeCoolingActive`, `GetHillHaulBrake`, `GetTravelGradeDegrees` and `GetTowLoadFactor`.

## Acceptance scenarios

1. **Cold baseline:** begin a loaded descent with heat near zero and throttle released. Downhill tow braking must match the 0.1.62 requested authority and `GetTrailerBrakeAuthority()` must remain 1.0 while heat is below 0.62.
2. **Sustained descent:** keep the loaded combination in the downhill-brake condition. Heat must rise monotonically toward 1.0 while the assist remains bounded.
3. **Fade entry:** once heat exceeds 0.62, `IsTrailerBrakeFadeActive()` must become true and authority must fall progressively rather than jump to zero.
4. **Maximum fade:** at/above 0.92 heat, trailer-assist authority must stop degrading at 0.55. The tractor must still retain base/shared braking authority.
5. **Recovery:** leave the downhill-brake condition on level/uphill ground or by deliberate throttle input. Heat must decrease, cooling telemetry must activate while cooling, and full authority must return when sufficiently cool.
6. **Hill-hold isolation:** stop/near-stop with a loaded trailer on a >=4 degree grade and throttle released. Hill hold must remain available and must not be reduced by trailer-brake heat.
7. **Driver override:** apply deliberate throttle during a descent. Automatic downhill trailer braking must release exactly as in 0.1.62; the thermal model must not fight the driver.
8. **Terrain regression:** repeat on low-grip ground. Existing terrain throttle authority must remain stricter than firm-road authority and thermal control must not increase propulsion.
9. **Heavy-haul regression:** on firm level road at full trailer load, max throttle authority remains 0.70 and steering authority remains 0.88.
10. **Frame-hitch safety:** induce a temporary frame hitch where practical. A single long frame must not cause an instantaneous full heat jump because thermal integration is capped at 0.10 s per drive-command update.
11. **Detach/empty transition:** remove the meaningful trailer load. New downhill heating must stop; existing heat may cool but must not create braking by itself.
12. **Damage/fuel fail-safe:** with invalid Fieldmaster configuration, no fuel, or zero condition, the existing stop authority must remain stronger than thermal behavior.

## Evidence to capture

Record cold, fade-entry, maximum-fade and recovery telemetry with speed/grade/load plus a short driving capture. For any future demo candidate, preserve the exact candidate SHA/version/configuration with the existing Win64 attestation pipeline; source-only evidence is insufficient for release.

## Expected release status after source acceptance

- Roadmap checklist remains **125 / 130 (96.2%)** unless one of the existing five runtime/build gates is independently proven.
- Demo remains **NOT READY** without a verified UE 5.8 Win64 package, packaged EXE smoke, Native Chaos/trailer runtime evidence and human visual acceptance of the same candidate.
