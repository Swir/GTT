# GTT 0.0.37 — Village Presentation Pass Playtest

## Goal
Validate that the source-built countryside is easier to read and materially cleaner to look at without breaking the existing sandbox. This milestone is a presentation pass, not a claim of final authored environment art or a verified Windows demo.

## 1. Daytime village readability
1. Start a fresh play session in the prototype world during daylight.
2. Walk or drive the main village loop.
3. Confirm roadside lamp posts, reflector posts and existing roads form a readable route without new floating navigation arrows.
4. Confirm street point lights are not visibly lit during daytime.
5. Move more than roughly 19 m from a world-space gameplay label and confirm the distant label disappears.
6. Approach a garage, service, tuning, mission or activity terminal and confirm its nearby label becomes readable again.

**Pass:** nearby interaction information stays usable while distant walls of text no longer dominate the view.

## 2. Night lighting
1. Continue until the existing day/night system enters night (`<06:00` or `>=21:30`) or restore a save at night.
2. Drive the village loop past the farm, shop/police side, workshop and hall/tavern side.
3. Confirm the roadside point lights become visible automatically and illuminate the route rhythmically.
4. Continue across the day/night boundary and confirm they switch off again in daylight.

**Pass:** presentation consumes the real `AGTTDayNightCycle`; it does not run a disconnected visual clock.

## 3. Rural dressing
1. Drive to Hill Farm and inspect the delivery/mowing area.
2. Confirm source-built hay-bale props add farm context without blocking vehicle routes or mission triggers.
3. Drive to North Wood Yard.
4. Confirm log-stack props add visual context without blocking timber pickup/unload gameplay.

**Pass:** dressing improves location identity and remains non-colliding gameplay-safe scenery.

## 4. Vehicle-speed readability
1. Enter Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200 in separate passes.
2. Drive the main loop at each vehicle's normal speed.
3. Confirm reflector posts and lamp spacing make road edges/corners easier to parse.
4. Confirm the presentation pass does not alter vehicle fuel, condition, tire, tuning, damage, garage or save state.

## 5. Gameplay regression
Run one short sandbox chain:
1. Enter a vehicle.
2. Trigger one legal job or story/side activity.
3. Use workshop/service or tuning.
4. Trigger traffic/NPC activity on the village roads.
5. Save, quit and reload.
6. Revisit the same area at a different time of day.

**Pass:** presentation remains visual/readability-only and all existing gameplay loops continue to function.

## 6. Performance sanity
1. Drive through the densest village section with traffic/NPCs active.
2. Verify the 14 street lights use bounded attenuation and no shadow casting.
3. Verify distant labels are hidden rather than all being rendered across the countryside.
4. Watch for frame-time spikes while labels enter/leave the 1900 cm visibility radius.

## Release / demo acceptance boundary
- Repository sanity checks may validate source integration and regression guards.
- This milestone does **not** prove Unreal Engine 5.8 compilation, Win64 packaging, packaged EXE startup, GPU performance or visual quality on a real Windows build.
- Do not publish the first demo solely because this playtest document exists or source CI is green.
- Demo acceptance still requires a verified packaged Win64 build, runtime smoke test, green relevant CI and a manual visual pass showing the project is presentable enough to represent GTT.
