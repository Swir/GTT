# GTT 0.0.97 — Garage Fleet Operations & Service UX Playtest

## Goal
Verify that the garage behaves like a real fleet hub instead of a row of generic recall buttons. The authoritative Native Chaos state must win over hidden compatibility mirrors, damaged vehicles must remain damaged when recalled, and the workshop prompt must quote the same real repair economy used by service gameplay.

## Setup
1. Start the prototype with at least the Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200 owned.
2. Confirm wanted level and game-warden alert are both zero.
3. Damage a Native Rattleback or Mulebox enough to produce LIMP or TOW status; leave fuel below full and tires below full.
4. Record cash, vehicle condition, fuel, tire integrity and visible body damage before using the garage.

## Fleet office
1. Interact with the garage office while no unowned vehicle is parked at the registration desk.
2. PASS: one compact fleet summary lists deterministic numbered slots, vehicle names, READY/LIMP/TOW/IMMOBILE state and C/F/T/B percentages.
3. PASS: Native road vehicles are marked with `N` and damaged Native state is shown instead of stale hidden-mirror values.
4. PASS: Native road entries show repair/tow estimates derived from the existing breakdown decision economy.
5. Park an unowned eligible vehicle at the desk and interact again.
6. PASS: registration remains the priority action; an already-owned parked vehicle no longer steals the registration interaction from fleet inspection.

## Bay recall — state preservation
1. Read the damaged vehicle bay terminal before recall.
2. PASS: its world label shows the same service state and condition/fuel/tire/body values as the office summary.
3. Recall it for the displayed $15 service charge.
4. PASS: cash falls exactly by the recall fee and the Native Chaos pawn is teleported to the bay with residual linear/angular physics cleared.
5. PASS: condition, fuel, tire integrity, body damage and tuning are unchanged by recall; garage recall is transport, not a free repair.
6. PASS: progress is saved after successful recall so the compatibility mirror carries the new location/state into normal save flow.
7. Enter the recalled vehicle and attempt recall again from another bay interaction.
8. PASS: occupied vehicles cannot be recalled.

## Crime / warden locks
1. Raise police wanted to 1 and attempt a recall.
2. PASS: recall is blocked with the existing police message.
3. Clear wanted, trigger a game-warden alert and repeat.
4. PASS: recall remains blocked while the warden is actively looking for the player.

## Workshop quote continuity
1. Move the damaged Native road vehicle into workshop range without repairing it.
2. Read the interaction prompt.
3. PASS: the prompt names the real vehicle and shows the damage-based repair+refuel estimate before money is spent.
4. Interact and complete service.
5. PASS: the paid amount matches the quote and the next garage fleet readout moves toward READY with restored mechanical/body state.

## Regression pass
- Fieldmaster Native recall still uses its existing takeover-safe recall path.
- Legacy/non-Native owned vehicles still recall through `AGTTVehicleBase::RecallToTransform`.
- Garage registration, police lock and game-warden lock remain active.
- Existing player-selectable roadside tow remains separate from garage recall.
- No GTA assets, names, maps, music, logos or code are introduced.

## Demo acceptance note
This milestone improves player-facing fleet/service UX, but it is not a demo-release approval. A first public Windows demo still requires a real UE 5.8 Win64 package, successful packaged-EXE runtime smoke/evidence, green relevant CI, rendered visual acceptance and no demo-critical blockers.
