# GTT 0.1.41 — Garage / Workshop Recovery Integration

0.1.41 deliberately extends the existing `AGTTServiceTerminal`, garage fleet dashboard and roadside recovery path. The initial idea of a second world-level workshop subsystem was removed before merge because the existing workshop is already the correct gameplay authority.

## Functional package

- Roadside tow remains transport-only: locked tow price, exact target identity and ordinary damage preservation are unchanged.
- Towed owned native road vehicles naturally become serviceable at the existing workshop terminal.
- Existing **E / Interact** is the only player action required; no new conflicting keybind is introduced.
- Fuel-only visits use exact per-litre billing and preserve mechanical/body state.
- Mechanical/body visits use the existing `UGTTBreakdownDecisionSubsystem` repair estimate and existing `ApplyNativeWorkshopService()` implementation.
- Native service is now transactional: ownership + exact ID + Wanted + physical radius → single debit → apply → verify exact ID/cargo/full restoration → flush persistence mirror → existing GameMode save.
- Verification failure rolls migration/body/cargo state back and refunds the exact debit.
- Farm Cargo systems remain sole owners of cargo timer, integrity, route completion, payout and reputation.
- Existing garage recall remains state-preserving and does not become a free repair path.

## Verification boundary

Dedicated 0.1.41 CI runs the new integration verifier plus existing garage fleet, roadside recovery and deterministic progress checks. This is not a substitute for an actual Unreal Engine 5.8 Win64 compile/package/runtime smoke.

Roadmap remains **125/130 = 96.2%** until the five real Native Chaos/authored trailer/Win64 runtime gates are satisfied.
