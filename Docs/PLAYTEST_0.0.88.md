# GTT 0.0.88 Playtest — Native Chaos Spike-Strip Parity

## Goal
Prove that an active Native Chaos road vehicle receives the same police spike-strip consequence as the legacy vehicle path and that the consequence persists into the shared workshop/save state.

## Route
1. Own either Rattleback 82 or Mulebox 1200 and allow Native Chaos takeover to become active.
2. Confirm the vehicle is driveable and tire integrity is above 0.70.
3. Raise wanted heat until a tier-1 or tier-2 roadblock deploys.
4. Cross the spike strip with the Native Chaos vehicle.
5. Confirm `ROADBLOCK_SPIKE_CONSEQUENCE` reports `path=NATIVE_CHAOS`, the correct vehicle id, and `tire_after < tire_before`.
6. Continue driving. Confirm degraded tire integrity increases Native wheel risk and reduces effective grip/steering authority under load.
7. Exit/re-enter or trigger the shared mirror/save path and confirm the reduced tire integrity remains present.
8. Use the workshop service and confirm tire integrity/body condition recover through the existing repair loop.
9. Repeat with the other Native road vehicle and verify the 0.75 s repeat-hit cooldown prevents one physical crossing from becoming a burst of duplicate damage.

## Acceptance
- Native Rattleback and Mulebox both take spike damage.
- Legacy vehicle behavior remains unchanged.
- Telemetry identifies the damage path.
- Tire damage feeds Native driving dynamics and persists through the mirror/save path.
- No crash, fallback or duplicate-hit storm occurs.

## Demo note
Passing this playtest in source/PIE is not sufficient for a demo release. The packaged Win64 executable and visual acceptance route remain mandatory.
