# GTT 0.1.41 — Garage / Workshop Recovery Integration

## Added

- Player-authorized full workshop service for owned native road vehicles physically present at the workshop.
- One authoritative quote from the existing breakdown decision model covering condition, tires, missing fuel and native body damage.
- Transactional service: validate → single debit → reuse `ApplyNativeWorkshopService()` → verify exact ID/cargo/full restoration → flush persistence mirror.
- Fail-safe rollback/refund when post-service identity, cargo or restoration checks fail.
- World-level **H** workshop interaction for the nearest eligible owned vehicle, plus Blueprint-friendly quote/purchase APIs.
- Wanted fail-closed behavior and proximity/speed/ownership guards.
- 68-scenario playtest and dedicated source-contract workflow.

## Integration

Roadside tow remains transport-only and keeps its locked tow quote separate. Its existing workshop destination naturally places the exact towed native vehicle inside the new workshop service radius; the player then explicitly chooses whether to pay for the full repair. Emergency patch remains a temporary limp-home option. Police impound keeps its existing mandatory consequence path and does not use the voluntary H purchase flow.

Farm Cargo is deliberately not mutated by workshop code. Service verifies that `PersistentVehicleId` and native cargo load factor are unchanged; payout, timer, integrity and route authority remain owned by the existing Farm Cargo systems.

## Verification boundary

This milestone does not claim an Unreal Engine 5.8 Win64 compile/package or packaged EXE smoke. Roadmap completion remains 125/130 (96.2%) until the real Native Chaos/authored trailer/Win64 runtime gates are satisfied.
