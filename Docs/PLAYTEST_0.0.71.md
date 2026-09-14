# GTT 0.0.71 Playtest — Native Roadside Recovery & Police Impound

## Purpose
Verify that catastrophic Native road-vehicle failures now resolve through the existing economy, wanted, workshop and garage/save loop instead of a free reset.

## Setup
1. Run a real Unreal Engine 5.8 build with an accepted Native Rattleback 82 or Mulebox 1200 takeover.
2. Own the vehicle and enter it normally.
3. Keep the Output Log visible and filter for `NATIVE_ROADSIDE` and `NATIVE_POLICE_IMPOUND`.
4. Note player cash, wanted level, vehicle condition/fuel/tire integrity and body-zone state before each scenario.

## Scenario A — Mechanical roadside recovery
1. With no wanted level, strand the active Native road vehicle by exhausting fuel, destroying tire integrity or reducing condition/body state to the recovery threshold.
2. Stop below 3.5 km/h and remain in the vehicle.
3. Confirm `NATIVE_ROADSIDE_RECOVERY_ARMED` appears and the player receives an inbound roadside message.
4. After roughly seven seconds confirm the fee is deducted, the player and vehicle are delivered to the workshop area and `NATIVE_ROADSIDE_RECOVERY_COMPLETE` is logged.
5. Confirm condition, fuel, tires, cooling stress and detached body panels are restored by the existing Native workshop service.
6. Exit/re-enter or trigger a save/load path and confirm the legacy garage mirror retains the repaired state and workshop transform.

## Scenario B — Insufficient cash
1. Repeat Scenario A with cash below the quoted recovery cost.
2. Confirm `NATIVE_ROADSIDE_RECOVERY_DENIED` is logged.
3. Confirm the vehicle is not teleported or repaired and cash does not go negative through a hidden free recovery.

## Scenario C — Search-state exploit protection
1. Become wanted level 1, then strand the Native road vehicle.
2. Confirm civilian roadside service is blocked and `NATIVE_ROADSIDE_RECOVERY_BLOCKED` appears.
3. Wait until the wanted search clears naturally.
4. Confirm normal roadside recovery can then arm without restarting the vehicle or mission state.

## Scenario D — Police impound
1. Reach wanted level 2 or higher and disable the Native road vehicle during the pursuit.
2. Stop and remain in the vehicle.
3. Confirm `NATIVE_POLICE_IMPOUND_ARMED` appears instead of civilian roadside assistance.
4. After the response window confirm an impound/safety-service fine is charged, wanted is cleared, the driver and car are moved to the workshop/impound destination and `NATIVE_POLICE_IMPOUND` is logged.
5. Confirm the serviced vehicle remains owned and its repaired state is synchronized to the normal garage/save mirror.

## Scenario E — Regression
- Verify ordinary healthy vehicles never arm recovery merely because they stop at traffic lights or park.
- Verify Native takeover/fallback, wheel-state traction control, cargo dynamics, crash-scene AI and hit-and-run escalation still work.
- Verify a loaded Mulebox can still complete the legal farm cargo contract after a normal non-disabling collision.
- Verify the existing roadside recovery *job* remains separate and playable; this milestone is player-vehicle assistance, not a replacement for the legal towing contract.

## Demo gate
This playtest is not sufficient to publish a demo by itself. Demo remains blocked until a verified UE 5.8 Win64 package, packaged-EXE runtime smoke, rendered visual acceptance, green relevant Actions and no demo-critical blockers all exist for the same release candidate.
