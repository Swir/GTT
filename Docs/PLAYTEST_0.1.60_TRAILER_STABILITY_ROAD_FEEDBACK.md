# GTT 0.1.60 — Trailer Stability & Road Feedback playtest

## Scope

This milestone turns the physical farm trailer into a clearer road-going gameplay object without claiming the remaining authored skeletal trailer or Win64 package gates. The loaded trailer receives a bounded anti-sway assist and state-driven tail/brake/reverse/hazard lighting.

## Source-contract acceptance

- [ ] The trailer road-feedback subsystem discovers physical `AGTTFarmTrailer` actors in Game/PIE worlds.
- [ ] Attached trailers show low-intensity red rear lighting.
- [ ] Measured deceleration of at least 6 km/h/s raises rear lighting to brake intensity.
- [ ] Reverse motion at or below -2 km/h activates white reverse lights.
- [ ] Lost wheel, hitch integrity below 45%, trailer integrity below 45%, or an active roadside repair activates pulsing amber hazards.
- [ ] Anti-sway is disabled for detached trailers, empty trailers and any trailer with a lost wheel.
- [ ] Loaded-trailer anti-sway begins at 25 km/h, reaches its configured speed authority at 70 km/h and is capped at 45% authority.
- [ ] Damage reduces stability assistance instead of concealing a failing trailer.
- [ ] Lateral correction force is capped at 650000 and yaw correction torque at 5000000.
- [ ] Existing 0.1.59 heavy-haul dynamic hitch stress remains authoritative; stability does not remove its speed/attitude damage path.

## Editor/PIE playtest

1. Start a Heavy Haul contract and attach the farm trailer with cargo loaded.
2. Accelerate from 0 through 25 km/h and verify no obvious low-speed steering correction is introduced.
3. Cruise between 35–55 km/h, make a progressive lane change and verify the trailer settles without snapping back or oscillating unnaturally.
4. Repeat above 65 km/h and verify 0.1.59 still records rough driving/dynamic hitch stress while 0.1.60 only damps the trailer response.
5. Brake from road speed and verify both rear red lights visibly brighten during measured deceleration.
6. Reverse below -2 km/h and verify both white reverse lights illuminate.
7. Damage the hitch below 45% and verify amber hazards pulse; repair the hitch and verify hazards clear when no other critical state remains.
8. Lose a trailer wheel and verify hazards pulse and anti-sway assistance is disabled.
9. Begin roadside trailer repair and verify hazards pulse during the repair window.
10. Detach the trailer and verify normal tail/brake/reverse road lighting turns off.

## Runtime evidence boundary

Passing the source verifier and Linux GitHub Actions proves the deterministic source/math contract only. It does **not** prove an Unreal Engine 5.8 Win64 compile, package, packaged-EXE smoke test, visual acceptance, authored skeletal trailer wheels, or final hitch sockets. Those gates remain open and must not be marked complete from this milestone alone.
