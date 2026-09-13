# GTT 0.0.32 — Combat Presentation Playtest

This milestone turns the existing Rural Arsenal combat loop into a visually readable fight system using original, source-authored runtime geometry and motion. It does not import protected third-party weapon meshes, animations, logos or game assets.

## Player weapon acceptance

1. Load a save with the Rural Arsenal or collect the available weapon pickups.
2. Cycle through **Pitchfork, Axe, Branch, Rake, Cow Chain, Shovel, Workshop Wrench and Old Farm Shotgun**.
3. Verify each equipped item has a visibly different silhouette made from GTT-owned runtime primitive composition.
4. Switch back to Bare Hands and verify the weapon prop hides.
5. Drop an equipped weapon and verify the held visual immediately returns to Bare Hands.
6. Pick/equip a weapon again and verify the presentation follows the real `UGTTCombatComponent::GetEquippedWeapon()` state rather than a separate inventory.

## Attack motion acceptance

1. Attack with each melee weapon and verify a short forward swing plays while the existing damage sweep/cooldown remains authoritative.
2. Fire the Old Farm Shotgun with shells and verify the held model produces a shorter recoil motion.
3. Fire with zero shells and verify gameplay still reports no ammunition; presentation must not create ammunition or bypass the combat component.
4. Verify weapon cycling, dropping, save/load and controller attack bindings continue to work.

## NPC hit/attack reactions

1. Start Bent Axle Brawl and at least one hostile-territory encounter.
2. Hit a civilian/brawler/hostile and verify body/head visibly recoil during the existing physical knockback.
3. Reduce an NPC to zero health and verify a visible knocked-out pose remains instead of the actor simply disappearing; recovery after the existing knockout timer must restore the normal pose.
4. Verify hostile archetypes display simple differentiated combat props and visibly swing them when their existing retaliation attack fires.
5. Confirm Runner, Scrapper, Bruiser and Enforcer health/speed/damage behavior is unchanged except for presentation.

## Regression

- wanted heat still comes from the existing assault/firearm hooks;
- player health/defeat consequence remains unchanged;
- faction victory/notoriety and brawl payouts still resolve normally;
- world-performance critical simulation remains forced during combat/reaction windows;
- no external weapon/animation asset files are required for a clean checkout.

## Runtime boundary

Repository sanity verifies source wiring, all eight weapon profiles, attack/reaction state integration, roadmap accounting and absence of imported combat asset files. A real Unreal Engine 5.8 Win64 compile/package/runtime playtest is still required before release acceptance, and this milestone does **not** claim that an EXE has passed that test.
