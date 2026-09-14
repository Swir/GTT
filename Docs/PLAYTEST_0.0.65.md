# GTT 0.0.65 Playtest — Mulebox Cargo Dynamics

## Goal
Verify that the legal farm cargo contract changes the real handling and economy of the Mulebox 1200 instead of treating cargo as mission text only.

## Scenario A — unloaded baseline
1. Enter Mulebox 1200 before starting the farm cargo contract.
2. Accelerate through a straight section and make several medium/high-speed steering inputs.
3. Confirm the vehicle uses the normal fleet handling and no `MULEBOX_CARGO_LOAD ... load=1.00` evidence is emitted.

## Scenario B — loaded delivery
1. Start the legal farm contract and collect cargo at FEED DEPOT with Mulebox 1200.
2. Confirm `MULEBOX_CARGO_LOAD vehicle=Mulebox1200 load=1.00` is emitted.
3. Accelerate above roughly 45 km/h and compare steering authority with the unloaded baseline.
4. Above roughly 75 km/h confirm full throttle is moderated while the cargo is loaded.
5. Finish the route to HILL FARM without destroying the cargo.

Expected: the loaded van feels heavier and less willing to make abrupt high-speed direction changes. This response is routed into the existing Chaos bridge input path, so a future accepted Native Chaos Mulebox consumes the same gameplay state rather than a parallel tuning system.

## Scenario C — economy integration
Complete the farm delivery with Mulebox 1200 and confirm the payout message contains `MULEBOX ROLE BONUS`. Repeat with another valid work vehicle and confirm that bonus is absent.

## Scenario D — cleanup/regression
1. Complete the job and confirm `MULEBOX_CARGO_LOAD ... load=0.00`.
2. Repeat and deliberately fail by letting the timer expire or cargo integrity reach failure.
3. Confirm load returns to zero after failure as well.
4. Confirm ordinary farm contracts still accept other working vehicles and the existing fast-delivery / integrity reward logic remains active.

## Acceptance note
This milestone is source-level gameplay integration. It does **not** prove a packaged Win64 Unreal build or a visually accepted demo. Native Chaos roadmap items remain open until genuine UE 5.8 runtime evidence exists.
