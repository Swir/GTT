# GTT 0.0.54 — Native Physics Acceptance & Ground Contact Evidence

## Goal
Turn Native Fieldmaster acceptance from a simple asset-presence check into a stronger runtime gate that can reject malformed authored physics and produce measurable four-wheel ground-contact evidence before demo acceptance.

## What changed
- Added `UGTTNativePhysicsAcceptanceSubsystem` for active Native Fieldmaster takeovers.
- Runtime acceptance now checks that Chaos wheeled movement is active, mesh collision is enabled, the Physics Asset contains rigid bodies and owns the required root-body contract.
- Wheel-bone geometry is validated for non-degenerate front/rear track widths and axle separation before the authored rig is considered healthy.
- Four wheel-bone ground probes sample actual world contact underneath the tractor and periodically emit `NATIVE_PHYSICS_EVIDENCE` with contact count and per-wheel clearance.
- A sustained authored-physics failure for 1.5 seconds emits `NATIVE_PHYSICS_FALLBACK` and returns to the legacy Fieldmaster instead of leaving the player in a broken native takeover.
- Airborne/low-contact states are evidence only; they do not falsely trigger fallback by themselves.

## Runtime playtest
1. Launch in Unreal Engine 5.8 with the authored Native Fieldmaster skeletal mesh and Physics Asset assigned.
2. Confirm native takeover becomes active and no `NATIVE_PHYSICS_FALLBACK` appears on a healthy rig.
3. Park on flat road and inspect `NATIVE_PHYSICS_EVIDENCE`; expect 3–4 ground probes in contact and plausible clearances rather than `-1` on every wheel.
4. Drive slowly over a curb, ditch edge and uneven farm terrain. Confirm contact count can naturally vary without forcing fallback.
5. Confirm front/rear wheel bones do not collapse to coincident locations and evidence continues while driving.
6. Temporarily break the authored setup (missing Physics Asset/root body or disabled mesh collision) in an editor-only test copy. Confirm the invalid state persists for the grace interval and then returns to the legacy tractor.
7. Restore the authored setup and confirm native takeover can be accepted again.
8. Repeat with workshop damage/tuning and heavy-haul attached to confirm the acceptance layer does not create duplicate gameplay state.

## Acceptance limits
- Repository sanity proves source wiring and roadmap honesty only; it is not a substitute for UE runtime execution.
- Ground probes are runtime evidence around wheel bones, not a claim that Chaos wheel contact internals were package-tested.
- Native Chaos movement/drivetrain roadmap items remain open until authored suspension/wheel behavior is verified in UE 5.8.
- Demo remains blocked until Win64 package, packaged EXE runtime smoke, rendered visual acceptance and relevant green Actions all exist.
