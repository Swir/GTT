# GTT 0.0.95 — Breakdown, Tow & Repair Economy Playtest

## Goal
Prove a coherent damage economy instead of a free reset: Native road damage is assessed from persisted mechanical/body state, roadside transport has its own quote, normal towing preserves damage, and workshop service is a second damage-based payment.

## Manual UE 5.8 playtest
1. Enter an owned Rattleback 82 or Mulebox 1200 and confirm cash, wanted and radio state still follow the driver while the Native road pawn is possessed.
2. Create light and then heavy structural/tire/condition damage. The workshop charge must rise with condition loss, tire loss, missing fuel and structural parts.
3. Fully strand the vehicle with no wanted level. The roadside message must show both the tow quote and the separate workshop estimate.
4. Let roadside transport complete. Cash must drop by the tow price, the vehicle must arrive at the workshop, and condition/tire/body damage must remain unchanged. Towing must not silently repair it.
5. Interact with the workshop. A second damage-based payment must be taken; only then should condition, tires, fuel, cooling stress and detached-panel state be restored.
6. Repeat at wanted level 1: assistance must remain blocked. Repeat at wanted level 2+: police impound must still fine, clear wanted state and perform mandatory safety service.
7. Save/load before workshop repair and confirm persisted damage still produces an equivalent repair estimate after reload.

## Source/CI acceptance
`Scripts/verify_breakdown_repair_economy.py` and the full Project sanity workflow must pass. The verifier also re-counts the locked roadmap dashboard from checklist truth.

## Packaging truth
This source milestone does not prove a UE 5.8 Win64 package, packaged-EXE runtime smoke or rendered visual acceptance. Demo release remains blocked until those gates actually run successfully.
