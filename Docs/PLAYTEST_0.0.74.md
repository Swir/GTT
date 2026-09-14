# GTT 0.0.74 Playtest — Native Fleet Rollover Safety & Emergency Righting

## Purpose
Verify that Native Fieldmaster, Rattleback 82 and Mulebox 1200 share one grounded rollover-safety layer without creating airborne auto-leveling or a free recovery exploit.

## Setup
1. Use an accepted Native takeover vehicle with its canonical Chaos wheel/powertrain setup and Physics Asset.
2. Open Output Log and filter for `NATIVE_FLEET_ROLLOVER`.
3. Repeat the scenarios with the Fieldmaster, Rattleback and Mulebox.

## Scenario A — Two-wheel lift assistance
1. Drive across a side slope or curb transition fast enough to raise body tilt above the normal range while at least two wheels remain in contact.
2. Confirm `NATIVE_FLEET_ROLLOVER_EVIDENCE` reports non-zero `correction` as tilt increases.
3. Confirm the assistance reduces progressive rollover tendency without snapping or teleporting the vehicle upright.
4. Repeat at low speed below the anti-roll speed gate and confirm no unnecessary torque is injected.

## Scenario B — Airborne exclusion
1. Crest a rise so the vehicle has fewer than two valid wheel contacts.
2. Confirm correction strength remains zero while airborne/unsupported.
3. Land normally and confirm the subsystem resumes only after grounded contact is re-established.

## Scenario C — Tire condition connection
1. Test with healthy tires and record the approximate correction strength at a repeatable slope/speed.
2. Reduce tire integrity through the existing damage/wear loop.
3. Repeat and confirm corrective authority is lower with worn tires.
4. Repair tires at the existing workshop and confirm normal authority returns.

## Scenario D — Genuine rollover recovery
1. Roll the vehicle beyond 105 degrees and allow it to become nearly stationary.
2. Keep the player in the driver seat.
3. Confirm emergency righting does not start immediately; the vehicle must remain stranded for the arm delay.
4. Confirm `NATIVE_FLEET_EMERGENCY_RIGHTING` is emitted once, followed by a short physical torque/lift pulse rather than a teleport.
5. Confirm the cooldown prevents repeated righting pulses from being spammed.

## Scenario E — No unattended recovery
1. Roll a Native vehicle and exit it, or observe an unoccupied active Native vehicle.
2. Confirm emergency righting does not arm without a driver.
3. Confirm normal physics remains authoritative.

## Demo gate
Do not publish the Windows demo from this source-level milestone alone. The release candidate still requires a real Unreal Engine 5.8 Win64 compile/package, successful packaged-EXE runtime smoke test, rendered visual acceptance, green relevant GitHub Actions and no demo-critical blocker.
