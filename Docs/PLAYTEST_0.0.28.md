# GTT 0.0.28 — Village Social Life Playtest

This milestone turns two previously exterior-only village landmarks into playable social spaces and adds time-aware social encounters that consume existing world and vehicle state.

## Tavern/community-hall interiors

1. Start at the village and walk to **The Bent Axle Tavern**.
2. Interact with the exterior door and confirm the player enters a physical interior room rather than triggering a text-only event.
3. Walk the room, confirm the floor/walls contain the player and the bar/table props make the space readable.
4. Use the interior exit and confirm the player returns to the tavern exterior without losing control.
5. Repeat the same enter/walk/exit sequence at the **Community Hall**.
6. Confirm both venues remain separate spaces and their doors return to the correct exterior landmark.

## Time-aware social encounters

1. During daytime, talk to **Mara — bartender** and **Nell — hall organizer**; record their daytime guidance.
2. Advance world time into the active village-party window (18:30–02:30) and talk to both again.
3. Confirm their dialogue changes with the real `AGTTDayNightCycle` state rather than using a separate fake clock.
4. Talk repeatedly to **Oren — hill farmer** and confirm multiple contextual lines cycle through existing legal-work, mud/tire and game-warden systems.

## Vehicle-state mechanic test

1. Own at least one Fieldmaster, Rattleback or Mulebox and park it near the village.
2. Note its actual condition, fuel percentage and tire integrity from the existing vehicle HUD/telemetry.
3. Damage the vehicle, consume fuel and/or damage the tires, then enter The Bent Axle.
4. Talk to **Jory — local mechanic**.
5. Confirm Jory reports the nearest owned vehicle name plus values that follow its real condition, fuel and tire-integrity state.
6. Repair/refuel the vehicle at the existing workshop, talk to Jory again and confirm the advice reflects the changed values.

## Existing-loop regression

- Trigger **Night Shift Favor** and confirm the new interiors do not prevent its exterior tavern/workshop interactions.
- Trigger the Bent Axle brawl/nightlife systems and confirm the existing combat/night-event loop still runs.
- Drive through village traffic before and after visiting an interior; confirm traffic and police/ranger systems remain active.
- Quick-save/load outside a venue and verify normal world-state persistence remains intact.
- Verify controller `Interact` can enter/leave both venues and talk to all four social NPC roles.

## Acceptance boundary

Repository sanity verifies source integration, roadmap arithmetic and regression wiring. This milestone does **not** claim final authored interior art, final character models/voice acting, or a successful Unreal Engine 5.8 Win64 compile/package/runtime smoke test until an Unreal-equipped runner actually executes those steps.
