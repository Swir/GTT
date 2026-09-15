# GTT 0.0.87 Playtest — Roadblock Spike Consequences

Use a packaged Win64 build from the same commit. Escalate wanted until a roadblock is deployed, then drive a shared-damage-model vehicle across the spike strip at road speed.

Acceptance requires a `ROADBLOCK_SPIKE_CONSEQUENCE` log entry naming the vehicle and showing `tire_after < tire_before`. Confirm that handling degrades through the existing tire-integrity grip model and that workshop tire repair restores the vehicle. A single collision burst must not generate repeated accepted hits faster than the 0.75 s per-vehicle cooldown.

Repeat against response tier 2 and confirm its tire/body damage is greater than tier 1. Continue the pursuit after the strip to verify the consequence remains integrated with wanted/police gameplay rather than resetting the vehicle.

This playtest does not certify Native Chaos spike parity, Win64 packaging, rendered visual quality or Demo Release readiness by itself.
