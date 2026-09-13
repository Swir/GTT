# GTT 0.0.36 — Native Rig Acceptance Playtest

This milestone hardens the 0.0.35 Native Rig Architecture into a runtime acceptance gate. A `UChaosWheeledVehicleMovementComponent` plus an arbitrary skeletal mesh is no longer enough to disable the proven legacy drivetrain. The bridge now requires the exact per-vehicle bone/socket contract before native Chaos can take control.

## What is implemented

- Runtime resolution of the existing `FGTTChaosRigContract` for Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.
- Native skeletal validation for every required wheel/root bone via `GetBoneIndex`.
- Native socket validation for `driver_seat`, `driver_exit` and the required `rear_hitch` on Fieldmaster/Mulebox.
- Explicit validation diagnostics surfaced through `GetRigValidationSummary()` and the bridge status string.
- Native Chaos takeover is gated on all three prerequisites: canonical vehicle spec, native movement component and a fully valid rig contract.
- Legacy `UGTTVehicleDynamicsComponent` stays enabled when the rig is incomplete, preventing a half-migrated vehicle from becoming undriveable.
- Farm-trailer attachment now consumes the validated native `rear_hitch` world transform when available and falls back to the existing source-driven hitch location otherwise.

## Source-level acceptance

1. Run `python Scripts/verify_native_rig_acceptance.py`.
2. Run the complete Project sanity workflow.
3. Confirm a skeletal mesh missing any required wheel/root bone does **not** transition the bridge to native ready.
4. Confirm a Fieldmaster/Mulebox skeletal mesh without `rear_hitch` remains on fallback dynamics.
5. Confirm Rattleback does not require `rear_hitch`.
6. Confirm the legacy dynamics component is disabled only after the complete rig contract passes.
7. Confirm trailer attachment chooses the native `rear_hitch` socket only after validation and retains the existing fallback path for current greybox vehicles.

## UE 5.8 runtime acceptance still required

When original GTT skeletal/physics assets are available:

1. Spawn Fieldmaster with `UChaosWheeledVehicleMovementComponent` and its authored skeletal body.
2. Verify bridge state changes `WAITING -> READY -> DRIVING` only with the exact bones/sockets present.
3. Remove/rename one required bone or socket in a test rig and verify the vehicle remains safely on fallback dynamics with a useful diagnostic.
4. Attach the articulated trailer and verify the physics constraint anchors at `rear_hitch` rather than the legacy approximate location.
5. Drive loaded/unloaded, damage tires/body, change tuning, consume fuel, enter/exit, garage/save/load and repeat the trailer test.
6. Repeat contract validation for Rattleback and Mulebox.
7. Package Win64 and perform a real EXE launch/drive/trailer/exit/relaunch smoke test.

## Demo gate

This is a meaningful reliability milestone for the upcoming native vehicles, but the first demo is **not** ready solely because the acceptance gate exists. The demo still requires presentable runtime visuals, a coherent playable slice, a verified Win64 packaged build and a real runtime smoke test.

There is no packaged EXE verification for 0.0.36 on the current sanity runner. Native Chaos, authored trailer assets and Win64 runtime roadmap tasks therefore remain open.
