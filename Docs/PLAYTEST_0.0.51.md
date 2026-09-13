# GTT 0.0.51 — Trailer Presentation & Damage Readability Playtest

## Goal
Validate that the heavy-haul farm trailer now reads as a coherent functional object during normal gameplay instead of a single greybox cargo block, while preserving the existing articulated physics, axle loss, roadside repair and Native Fieldmaster hitch behavior.

## Visual acceptance
1. Start Heavy Timber Haul and inspect the empty trailer from rear, side and hitch angles.
2. Confirm the silhouette includes a long drawbar, visible hitch coupler, front bulkhead, side rails, tailgate, left/right fenders and rear reflector bar.
3. Confirm those pieces move with the same physical trailer body; there must not be a second showcase trailer or detached presentation actor.
4. Load timber at North Wood Yard. The old single cargo cube must remain hidden and a visible multi-log timber stack must appear inside the trailer bed.
5. Complete at least one daylight and one nighttime tow pass and verify the larger silhouette remains readable without creating a wall of debug text.

## Gameplay-driven damage presentation
1. Strike the trailer hard enough to reduce trailer integrity and confirm the tailgate/reflector presentation visibly sags with damage.
2. Force the left axle constraint to break. Confirm the left wheel-loss state still affects cargo/trailer integrity and the matching left fender disappears.
3. Repeat for the right wheel and confirm the right fender follows that state independently.
4. Drive loaded with reduced cargo integrity. Confirm the top timber log shifts progressively instead of the cargo presentation remaining pristine.
5. Use roadside repair. Confirm restored wheels regain their matching fenders and the presentation updates without resetting mission stage or lost cargo integrity.

## Physics/gameplay regression
- Trailer body remains the simulated physical root.
- Both wheel rigid bodies and breakable axle constraints remain active.
- Legacy Fieldmaster hitch still works.
- Native Fieldmaster `rear_hitch` route still works when its acceptance gates pass.
- Loaded/unloaded mass switching remains 1680/980 kg.
- Hitch break/re-hook behavior remains functional.
- Heavy-haul cargo integrity and payout behavior remain unchanged except for the new visual feedback.
- Roadside repair still charges the economy and preserves contract progression.

## Demo gate
This milestone improves presentation but does **not** by itself make the Windows demo releasable. Do not create a GitHub Release until a real UE 5.8 Win64 package passes runtime smoke, rendered visual review and the other demo-critical gates.

## Known limitation
The new trailer presentation is original source-built Unreal geometry. It is intentionally **not** claimed as the final authored skeletal trailer wheel/hitch asset set. The corresponding roadmap checkbox must remain open until real authored assets and UE runtime acceptance exist.
