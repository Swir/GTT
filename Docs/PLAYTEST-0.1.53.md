# GTT 0.1.53 — Dynamic traffic incidents & roadside assistance playtest

Status: source milestone / pre-packaged-runtime evidence. This document is a manual test matrix plus source-contract verification target; it is **not a packaged Win64 runtime proof**.

## Player loop under test

A real condition loss on an ambient traffic car now promotes the existing crash-response behavior into a live incident. Nearby traffic slows around the scene. If the traffic vehicle becomes disabled, the player can interact to begin a **6 seconds** roadside assist while staying within **500 cm**. Completing the assist applies a field repair, leaves the civilian vehicle in a temporary limp state, and pays a severity-scaled legal reward from **$65** up to **$110**. The current incident grants a **single payout** only; walking away pauses/cancels the timed interaction without charging the player.

## Manual matrix

| # | Area | Setup / action | Expected result |
|---:|---|---|---|
| 1 | Incident | Damage traffic car by <4% condition | No new major incident is inferred |
| 2 | Incident | Damage traffic car by >=4% condition | Collision incident is registered automatically |
| 3 | Incident | Moderate damage | CAUTION/stop response appears |
| 4 | Incident | Condition falls to disable threshold | Vehicle becomes HAZARD / disabled |
| 5 | Incident | Disabled car continues physics motion | Braking force settles the car |
| 6 | Incident | Non-disabled incident expires | Car resumes route |
| 7 | Incident | Post-crash limp is active | Cruise speed remains reduced |
| 8 | Incident | Limp timer expires | Normal cruise target returns |
| 9 | Traffic | Nearby traffic within awareness radius | Nearby car performs SLOW response |
| 10 | Traffic | Traffic outside awareness radius | No forced nearby response |
| 11 | Traffic | Disabled neighbor receives fan-out | Disabled car does not re-enter reaction loop |
| 12 | Traffic | Ranger stop overlaps incident | Incident/disabled authority still stops vehicle |
| 13 | Traffic | Performance budget during incident | Incident car receives critical tick treatment |
| 14 | Traffic | Performance budget during assistance | Assisted car receives critical tick treatment |
| 15 | Interaction | Interact with healthy traffic car | NPC traffic remains non-stealable |
| 16 | Interaction | Interact with disabled traffic car | Roadside assist starts |
| 17 | Interaction | Helper has no economy component | Assist refuses safely |
| 18 | Interaction | Start beyond 500 cm | Assist refuses and asks player to move closer |
| 19 | Interaction | Start inside 500 cm | Six-second timer begins |
| 20 | Interaction | Stay beside car for full duration | Assist completes |
| 21 | Interaction | Walk beyond 500 cm before completion | Assist cancels without payout |
| 22 | Interaction | Return and interact after cancellation | Assist restarts from full duration |
| 23 | Interaction | Interact again while active | No duplicate concurrent assist is created |
| 24 | UX | Assistance active | World text shows ASSIST progress |
| 25 | UX | Leave scene mid-assist | HAZARD text returns |
| 26 | UX | Complete assist | THANKS feedback appears |
| 27 | Repair | Complete assist | Vehicle receives 45% max-condition field repair |
| 28 | Repair | Field repair clears disable threshold | Disabled flag clears |
| 29 | Repair | Complete assist | Incident stop timer clears |
| 30 | Repair | Complete assist | Temporary post-assist limp remains |
| 31 | Economy | Complete low-severity disabled incident | Reward starts at $65 |
| 32 | Economy | Complete max-severity incident | Reward caps at $110 |
| 33 | Economy | Cancel assist | No player cash is spent or awarded |
| 34 | Economy | Complete assist once | Exactly one AddCash payout occurs |
| 35 | Economy | Interact again with same incident | No second payout for same incident |
| 36 | Economy | Later genuinely new disabled incident | Payout authority is reset for new incident |
| 37 | Safety | Helper disappears during timer | Assist cancels safely |
| 38 | Safety | Incident clears externally during timer | Assist cancels without payout |
| 39 | Safety | Repair somehow remains below threshold | Completion aborts instead of paying |
| 40 | Safety | Nearby response fan-out occurs | It does not recursively register collisions |
| 41 | Route | Successful assist completes | Civilian car remains on original route |
| 42 | Route | Successful assist completes | Player never possesses civilian traffic car |
| 43 | Regression | Obstacle avoidance after limp | Probe/horn behavior still works |
| 44 | Regression | Stuck recovery after incident | Route-point recovery still works |
| 45 | Regression | Ranger road stop after assist | STOP/SLOW authority still works |
| 46 | Regression | Collision while Ranger stop active | Crash authority is not bypassed |
| 47 | Regression | No active incident | Existing traffic behavior is unchanged |
| 48 | Verification | Run `python Scripts/verify_0_1_53_roadside_assistance.py` | Source-contract verification passes |

## Evidence boundary

The verifier proves wiring, ordering, payout authority, interaction safety and documentation consistency from repository source. A real Unreal Engine 5.8 Win64 compile/cook/package plus packaged EXE smoke test is still required before this loop can count as demo runtime evidence.
