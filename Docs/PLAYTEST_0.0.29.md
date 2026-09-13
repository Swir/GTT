# GTT 0.0.29 — Workshop Customization Playtest

This milestone closes the remaining Vehicle Damage & Tuning gameplay gaps with workshop services that consume the existing economy, ownership, persistent tune levels and breakable-body systems.

## Fieldmaster visual upgrades

1. Own/register the **Rusty Fieldmaster 60** and park it at the village workshop.
2. Interact with the Fieldmaster style bay.
3. Confirm cash is charged and the first persistent engine tune stage is installed.
4. Confirm a visible front brush guard appears on the tractor.
5. Buy later stages and verify the work-light bar/lamps and rear toolbox appear as engine/tire tuning advances.
6. Save, reload, return to the style bay and interact again; the bay must rebuild the visual package from the already-persisted tune levels without charging for an already-complete package.
7. Drive through mud and a legal farm job to verify the existing power, cooling and tire systems still react normally.

## Rattleback street/performance variant

1. Own/register the **Rattleback 82** and park it at the same workshop.
2. Interact with the Rattleback variant bay.
3. Confirm the purchase consumes cash and advances the existing persistent engine/tire tune rather than creating a disconnected stat system.
4. Verify staged visual changes: hood scoop, ducktail and wider front/rear lip package.
5. Compare acceleration/grip before and after the package; changes should come from the existing engine/tire drivetrain multipliers.
6. Save/reload and confirm the tune levels remain; revisit the bay to reconstruct visuals from persistent state.

## Replacement body-panel economy

1. Damage an owned tractor, Rattleback or Mulebox until at least one registered body part detaches.
2. Note `BODY PARTS LOST` / detached-part count in the existing damage status.
3. Park the vehicle at the body-panel bay and interact.
4. Confirm the quote scales with the number of missing parts (`$55` per missing part, minimum `$90`).
5. Confirm cash is deducted, registered parts are physically reattached through the existing repair/breakable-part path and body condition is restored.
6. Interact again with no missing parts and confirm no charge occurs.

## Cross-system regression

- Insufficient cash must reject every purchase without granting an upgrade.
- A vehicle that is not owned by the player must be ignored by the workshop bays.
- The Fieldmaster bay must reject Rattleback/Mulebox; the Rattleback bay must reject Fieldmaster/Mulebox.
- Standard tuning, tire repair, workshop repair/refuel, garage recall, quick save/load, police/ranger heat and mission triggers must remain functional.
- Damage a customized vehicle after service and confirm registered breakable panels can still detach normally.

## Acceptance boundary

Repository CI validates source wiring, economy/tuning integration and exact roadmap arithmetic. Runtime art is intentionally original greybox/procedural workshop geometry. This milestone does **not** claim a successful Unreal Engine 5.8 Win64 compile/package/runtime smoke test until the self-hosted Unreal runner executes it.
