# GTT 0.1.44 Playtest — Packaged Workshop Hours & After-Hours Recovery Evidence

This matrix validates the 0.1.43 day/night → workshop → roadside tow → WORKSHOP HOLD → emergency-service loop in the exact packaged Win64 candidate. Source CI verifies wiring and invariants only; these runtime rows remain unverified until Unreal Engine 5.8 packages and executes the candidate.

## Test setup

- Use the ordinary village world with `AGTTDayNightCycle`, workshop terminal, garage fleet and at least one owned native road vehicle.
- Run the deterministic technical smoke route with `-GTTWorkshopHoursRuntimeScenario` after the existing Farm Cargo workshop-recovery scenario.
- Regular workshop hours are **06:30 inclusive through 20:00 exclusive**.
- After-hours emergency recovery is available only for an authoritative `TOW` / `IMMOBILE` WORKSHOP HOLD and uses the shared **+35%** surcharge calculation.
- Keep `FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json` from the same exact package/SHA; schema-14 promotion requires both evidence streams.

## A — World clock boundary truth

- [ ] A01 06:29 reports workshop CLOSED through `AGTTServiceTerminal::IsWorkshopOpenNow()`.
- [ ] A02 06:30 reports workshop OPEN.
- [ ] A03 12:00 remains OPEN.
- [ ] A04 19:59 remains OPEN.
- [ ] A05 20:00 reports CLOSED.
- [ ] A06 23:30 remains CLOSED.
- [ ] A07 02:00 remains CLOSED.
- [ ] A08 opening and closing values come from `GTTWorkshopHoursPolicy`, not duplicated constants in the evidence route.

## B — Closed-hours ordinary-service rejection

- [ ] B01 stage a damaged-but-mobile native road vehicle without WORKSHOP HOLD at 20:00.
- [ ] B02 the vehicle is close enough to the real workshop terminal to be the production service target.
- [ ] B03 service interaction is rejected while CLOSED.
- [ ] B04 rejection happens before any cash debit.
- [ ] B05 condition is unchanged.
- [ ] B06 tire integrity is unchanged.
- [ ] B07 fuel is unchanged.
- [ ] B08 rejection does not create WORKSHOP HOLD.
- [ ] B09 rejection does not change `PersistentVehicleId`.
- [ ] B10 opening the shop later remains possible; closed rejection does not poison later service state.

## C — Production tow creates authoritative hold

- [ ] C01 the same vehicle is damaged into a tow-eligible state.
- [ ] C02 `RequestRoadsideTow()` accepts the production request.
- [ ] C03 pending target equals the same exact `PersistentVehicleId`.
- [ ] C04 locked tow quote is positive.
- [ ] C05 request does not pre-charge cash.
- [ ] C06 tow completes within the deterministic runtime window.
- [ ] C07 exactly one tow debit equals the locked quote.
- [ ] C08 production `NATIVE_ROADSIDE_TOW_COMPLETE` evidence reports quote locked and target pinned.
- [ ] C09 damage is preserved by ordinary tow.
- [ ] C10 identity is preserved by tow.
- [ ] C11 destination is workshop.
- [ ] C12 garage fleet reports authoritative WORKSHOP HOLD after completion.

## D — After-hours emergency quote authority

- [ ] D01 clock remains CLOSED at 20:00 after tow.
- [ ] D02 hard WORKSHOP HOLD is still present before quote evaluation.
- [ ] D03 `GetNativeRoadRepairQuote()` returns the normal authoritative base repair quote.
- [ ] D04 `GetNativeRoadCheckoutQuote()` returns the after-hours checkout quote.
- [ ] D05 checkout quote is greater than the base quote.
- [ ] D06 surcharge percentage is exactly 35.
- [ ] D07 expected surcharge is rounded upward through the shared policy.
- [ ] D08 checkout quote equals `base + ceil(base * 0.35)` exactly.
- [ ] D09 quote calculation does not debit cash.
- [ ] D10 quote calculation does not clear the hold.

## E — Emergency service execution

- [ ] E01 interacting with the real workshop terminal while CLOSED + held enters the emergency path.
- [ ] E02 exactly one debit equals the previously observed checkout quote.
- [ ] E03 no second hidden service charge occurs.
- [ ] E04 mechanical condition reaches repaired state.
- [ ] E05 tire integrity reaches repaired state.
- [ ] E06 fuel reaches workshop-refuelled/full state.
- [ ] E07 authoritative WORKSHOP HOLD clears only after successful service.
- [ ] E08 `PersistentVehicleId` remains unchanged.
- [ ] E09 workshop is still CLOSED immediately after service; success did not cheat the clock.
- [ ] E10 the vehicle is dispatchable again because the real hold was removed.

## F — Evidence manifest and demo gate

- [ ] F01 runtime emits `WORKSHOP_HOURS_RUNTIME_BEGIN version=1`.
- [ ] F02 BOUNDARIES marker is PASS.
- [ ] F03 CLOSED_ORDINARY marker is PASS.
- [ ] F04 TOW_REQUEST and TOW_COMPLETE markers are PASS.
- [ ] F05 EMERGENCY_QUOTE marker is PASS.
- [ ] F06 EMERGENCY_SERVICE marker is PASS.
- [ ] F07 completion marker is PASS and contains stable vehicle ID plus tow/base/emergency quotes.
- [ ] F08 zero `WORKSHOP_HOURS_RUNTIME phase=DIAGNOSTIC result=FAIL` markers exist.
- [ ] F09 evaluator writes `WORKSHOP_HOURS_RUNTIME.json` with schema `gtt.workshop-hours-runtime.v1`.
- [ ] F10 manifest `git_sha` matches `BUILD_INFO.json` and expected GitHub SHA.
- [ ] F11 schema-13 Farm Cargo workshop recovery gate is a prerequisite.
- [ ] F12 promotion advances the same technical gate to schema 14 only after this manifest PASSes.
- [ ] F13 final candidate validation requires both `farm_cargo_workshop_recovery_runtime=PASS` and `workshop_hours_runtime=PASS`.
- [ ] F14 success and failure artifact paths retain the workshop-hours manifest.

## G — Regression and release truth

- [ ] G01 0.1.43 source verifier remains green.
- [ ] G02 0.1.42 packaged workshop-recovery verifier remains green.
- [ ] G03 ordinary daytime workshop service behavior remains unchanged.
- [ ] G04 garage recall still cannot bypass a TOW/IMMOBILE hold.
- [ ] G05 Farm Cargo workshop recovery manifest remains mandatory and same-SHA.
- [ ] G06 deterministic SWIR progress remains 125/130 = 96.2%.
- [ ] G07 README contains one progress card and Roadmap contains one progress mini.
- [ ] G08 no ASCII/Unicode progress meter returns.
- [ ] G09 source CI does not claim the new runtime manifest exists.
- [ ] G10 no Demo Release is published without verified Win64 compile/package/runtime, Native Chaos/trailer gates and rendered visual acceptance.

## Acceptance

Source milestone acceptance requires the 0.1.44 verifier, 0.1.43 gameplay regression, 0.1.42 packaged workshop-recovery regression and deterministic progress checks to pass. Runtime acceptance additionally requires the exact UE 5.8 Win64 package to emit `WORKSHOP_HOURS_RUNTIME.json` from the production markers and promote the same-SHA technical gate to schema 14. This playtest does not authorize closing any of the five remaining roadmap blockers by itself.
