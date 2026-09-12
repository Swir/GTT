# GTT 0.0.26 — Player Experience Playtest

This milestone makes keyboard/mouse and gamepad first-class input paths and adds persistent player-facing settings through `UGTTGameUserSettings`.

## Controller smoke test

1. Launch with an Xbox-compatible controller connected.
2. On foot verify left stick movement, right stick camera, A jump, X interact, RT attack, RB next weapon and D-pad Down drop weapon.
3. Enter the Fieldmaster, Rattleback and Mulebox. Verify RT accelerates, LT reverses/brakes, left stick steers, B exits and D-pad Right cycles radio.
4. Verify keyboard/mouse bindings still work without disconnecting the controller.
5. Verify Select/View quick-saves and Menu quick-loads.

## Persistent settings

1. Change look sensitivity, invert-Y, HUD/subtitle scale, master/radio volume, controller vibration/dead-zone and accessibility fields through the `UGTTGameUserSettings` API or a temporary Blueprint/debug settings surface.
2. Call `ApplyGTTSettings(true)`, quit completely and relaunch.
3. Confirm values persist in Unreal's normal GameUserSettings config and mouse/right-stick look uses the saved sensitivity/invert-Y setting.
4. Call `ResetGTTSettings()` and confirm the documented defaults return and persist.

## Accessibility regression

- Confirm subtitle enable/scale, HUD scale, reduced-camera-motion preference and color-vision mode are exposed as persistent Blueprint-readable/writeable settings for UI/render/audio consumers.
- Confirm values are clamped to safe ranges before save.
- Confirm no setting mutates story/economy/world SaveGame state.

## Gameplay regression

Run Player Farm -> village -> hostile territory -> North Pass in one session. Test interaction, combat, weapon cycling, driving, radio, save/load and wanted escape on controller, then repeat critical actions on keyboard/mouse.

## CI / build boundary

`Scripts/verify_player_experience.py` structurally verifies the custom GameUserSettings class, persistence hooks, character sensitivity/invert integration, complete current gameplay gamepad mappings, roadmap arithmetic and CI wiring. Repository CI still does not provide a full Unreal Engine 5.8 Win64 compile/package/smoke environment, so this milestone does not claim a verified packaged EXE.
