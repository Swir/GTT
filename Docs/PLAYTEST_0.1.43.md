# GTT 0.1.43 Playtest — Workshop Hours & After-Hours Recovery

This matrix validates the connected day/night → workshop → garage hold → economy → save loop. Source-contract CI may verify wiring and invariants, but packaged/runtime rows remain unverified until an actual Unreal Engine 5.8 Win64 package executes them.

## Test setup

- Use the ordinary village world with `AGTTDayNightCycle`, garage office/bays, workshop terminal and owned fleet.
- Keep one serviceable vehicle, one damaged but mobile vehicle and one `TOW`/`IMMOBILE` vehicle available.
- For Farm Cargo continuity rows, bind the exact Mulebox through the production cargo authority; do not substitute another vehicle.
- Workshop regular hours are **06:30 inclusive through 20:00 exclusive**.
- After-hours emergency recovery is allowed only for a hard **WORKSHOP HOLD** and charges the normal repair quote plus **+35%**, rounded up through the shared policy.

## A — Clock boundaries and status presentation

- [ ] A01 06:29 reports workshop CLOSED.
- [ ] A02 06:30 reports workshop OPEN.
- [ ] A03 12:00 reports workshop OPEN.
- [ ] A04 19:59 reports workshop OPEN.
- [ ] A05 20:00 reports workshop CLOSED.
- [ ] A06 23:30 reports workshop CLOSED.
- [ ] A07 02:00 reports workshop CLOSED.
- [ ] A08 day rollover does not invert the schedule.
- [ ] A09 workshop interaction text includes the shared schedule.
- [ ] A10 garage office summary reports the same OPEN/CLOSED state.
- [ ] A11 garage office reports current world clock when present.
- [ ] A12 a stripped test map without `AGTTDayNightCycle` fails open instead of soft-locking service.

## B — Normal daytime workshop service

- [ ] B01 damaged Rattleback at 06:30 can buy normal repair/refuel service.
- [ ] B02 damaged Mulebox at noon can buy normal repair/refuel service.
- [ ] B03 daytime hard `TOW` hold uses the ordinary damage-based quote with no emergency surcharge.
- [ ] B04 daytime `IMMOBILE` hold uses the ordinary quote with no emergency surcharge.
- [ ] B05 fuel-only road vehicle uses the exact per-litre fuel quote during open hours.
- [ ] B06 healthy/full vehicle is not charged.
- [ ] B07 successful native service calls the existing authoritative workshop repair path.
- [ ] B08 successful native service persists progress through the normal save checkpoint.
- [ ] B09 failed native service refunds the exact amount charged.
- [ ] B10 service preserves the vehicle `PersistentVehicleId`.
- [ ] B11 service clears the underlying hard hold only by repairing the authoritative state.
- [ ] B12 `LIMP`/ordinary `SERVICE` remain advisory rather than hard-hold emergency eligibility.

## C — Closed-hours ordinary service rejection

- [ ] C01 damaged mobile Rattleback at 20:00 is rejected before debit.
- [ ] C02 damaged mobile Mulebox at 02:00 is rejected before debit.
- [ ] C03 fuel-only native road vehicle after hours is rejected before debit/refuel.
- [ ] C04 healthy vehicle after hours remains informational and is not charged.
- [ ] C05 ordinary Native Fieldmaster repair after hours is rejected before debit.
- [ ] C06 ordinary legacy vehicle repair after hours is rejected before debit.
- [ ] C07 rejected service does not change condition.
- [ ] C08 rejected service does not change tire integrity.
- [ ] C09 rejected service does not change fuel.
- [ ] C10 rejected service does not clear garage service status.
- [ ] C11 rejected service does not write a new repair transaction.
- [ ] C12 closing/opening transition immediately changes eligibility without a duplicate scheduler.

## D — Emergency WORKSHOP HOLD recovery

- [ ] D01 `TOW` native road vehicle at 20:00 is still eligible for emergency recovery.
- [ ] D02 `IMMOBILE` native road vehicle at 02:00 is still eligible for emergency recovery.
- [ ] D03 emergency quote is based on the same authoritative daytime repair estimate.
- [ ] D04 +35% surcharge is applied exactly once.
- [ ] D05 fractional surcharge rounds upward deterministically.
- [ ] D06 HUD/interaction quote equals the amount actually debited.
- [ ] D07 insufficient cash leaves the vehicle held and unchanged.
- [ ] D08 successful emergency service repairs/refuels through `ApplyNativeWorkshopService()`.
- [ ] D09 successful emergency service clears the hard hold.
- [ ] D10 successful emergency service saves progress.
- [ ] D11 emergency service does not create a second vehicle/economy ledger.
- [ ] D12 ordinary damaged-but-mobile vehicle cannot use the emergency lane merely because the shop is closed.
- [ ] D13 held Native Fieldmaster can use the same after-hours surcharge rule if a hard hold exists.
- [ ] D14 held legacy fleet vehicle can use the same after-hours surcharge rule if a hard hold exists.

## E — Garage and fleet consequence continuity

- [ ] E01 garage office shows hold count while workshop is open.
- [ ] E02 garage office shows hold count while workshop is closed.
- [ ] E03 closed-hours office explicitly explains emergency recovery availability for hard holds.
- [ ] E04 closed-hours office explains ordinary repair/refuel waits for opening.
- [ ] E05 numbered garage bay still blocks `TOW` recall before movement.
- [ ] E06 numbered garage bay still blocks `IMMOBILE` recall before movement.
- [ ] E07 hold rejection still occurs before recall fee.
- [ ] E08 normal `LIMP` fleet entry remains dispatchable.
- [ ] E09 normal `SERVICE` fleet entry remains dispatchable.
- [ ] E10 after emergency service the same vehicle becomes dispatchable again.

## F — Farm Cargo continuity and regression

- [ ] F01 loaded Mulebox keeps the same cargo-bound persistent ID while waiting for workshop opening.
- [ ] F02 cargo timer continues while ordinary service is unavailable after hours.
- [ ] F03 cargo integrity is not healed by workshop scheduling logic.
- [ ] F04 a tow-created hard hold retains exact cargo authority.
- [ ] F05 after-hours emergency service does not transfer cargo authority to another vehicle.
- [ ] F06 decoy vehicle remains unable to complete the loaded contract.
- [ ] F07 exact Mulebox can continue Hill Farm handoff after emergency service.
- [ ] F08 exact Mulebox can complete North Wood Yard after emergency service.
- [ ] F09 cargo completion still pays exactly once.
- [ ] F10 cargo reputation still increments through the production contract path.

## G — Release/evidence truth

- [ ] G01 source verifier passes without changing roadmap checkbox count.
- [ ] G02 deterministic progress SVG check remains green at 125/130 = 96.2%.
- [ ] G03 README contains one progress card and Roadmap contains one progress mini.
- [ ] G04 no legacy ASCII/Unicode progress meter returns.
- [ ] G05 0.1.41 garage hold verifier remains green.
- [ ] G06 0.1.42 packaged workshop evidence verifier remains green.
- [ ] G07 packaged Win64 runtime is not claimed from Linux/source CI.
- [ ] G08 no Demo Release is published without the existing Win64/Chaos/trailer/visual gates.

## Acceptance

Source milestone acceptance requires the dedicated 0.1.43 verifier, the 0.1.41 and 0.1.42 regression contracts, deterministic progress validation and the normal repository CI to pass. Packaged behavior remains a future runtime acceptance requirement until the exact candidate executes under Unreal Engine 5.8 on Win64.
