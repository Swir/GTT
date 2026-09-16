# GTT 0.1.4 Playtest — Living CARGO Market & Dynamic Contract Chain

## Goal

Verify the complete CARGO loop `inspect market -> prepare Mulebox -> collect feed -> Hill Farm -> optional North Wood Yard chain -> payout/reputation/history -> save/load`, including NPC-aligned opening hours, dynamic demand, police refusal, loaded-vehicle risk and persistent mixed ROAD/CARGO history.

## Setup

1. Use an Unreal Engine 5.8 development build or packaged Win64 candidate containing 0.1.4.
2. Own Mulebox 1200 and keep enough cash for CARGO service/dispatch.
3. Start with wanted and game-warden alert at zero.
4. Record world day/time, cash, logistics reputation, clean streak, CARGO completed/failed counts and recent-history summary.
5. Run the route once with the legacy Mulebox and once with Native Chaos Mulebox takeover where available.

## A — NPC-aligned depot schedule

1. Set time to 06:50 and inspect **Feed Cargo Run** on the unified contract board.
2. With a READY + ACTIVE Mulebox confirm acceptance is closed and the board reports **CLOSED 17:30-07:00**.
3. Damage or unstow the Mulebox and confirm PREP remains available while closed and charges the displayed preparation cost.
4. Advance to 07:00 and confirm the staged vehicle can accept without another dispatch charge.
5. Inspect again at 17:29 and 17:30; acceptance must transition from staffed to closed exactly at the existing villager work-shift boundary.

Expected: cargo-counter availability follows the same 07:00-17:30 work period used by scheduled villagers; closed hours do not create a free service path.

## B — Dynamic demand and locked market value

1. At the same reputation, compare a morning start before 10:00 with midday and a start after 14:00.
2. Confirm the board changes between **MORNING RUSH**, **STEADY DEMAND** and **LATE FEED DEMAND** and updates its displayed maximum payout.
3. Advance the village day and repeat to exercise the deterministic day-cycle demand component.
4. Accept a contract, then advance time into another demand band before completion.
5. Confirm the payout uses the multiplier locked at acceptance rather than silently repricing an already loaded run.
6. Build reputation/clean streak and confirm market multiplier grows but never exceeds **1.38x**.

Expected: market value is dynamic before acceptance, deterministic from current world/progression state and stable after the player commits.

## C — Route tier progression

1. With logistics reputation below 20, accept T1 and load at Feed Depot.
2. Reach Hill Farm and confirm the job completes there as the accessible newcomer route.
3. Raise reputation to at least 20 and accept again; confirm board/objective reports route T2.
4. Load at Feed Depot and reach Hill Farm. Confirm Hill Farm signs a **relay** only: timer, cargo integrity and Mulebox load remain live.
5. Continue the same cargo to **North Wood Yard** and use the final CARGO handoff terminal.
6. At reputation 50+, repeat as T3 and confirm the larger chain-completion bonus is reflected in the final reward.

Expected: reputation changes the physical route, not merely a UI badge; T2/T3 use one continuous loaded contract across both rural destinations.

## D — Vehicle condition and loaded risk across the chain

1. Run a healthy Mulebox as a baseline.
2. Repeat with condition below 65% and keep driving after Hill Farm.
3. Confirm cargo integrity continues decaying from the same value on the second leg; Hill Farm must not reset it.
4. In Native Chaos Mulebox, confirm `SetCargoLoadFactor(1.0)` remains active through the relay and still affects throttle/high-speed steering until the final handoff.
5. Destroy the cargo or let the common timer expire and confirm the chain fails, unloads the Mulebox, reduces reputation and saves the failed-run state.

Expected: the extended route reuses real vehicle/cargo dynamics and persistence rather than becoming a disconnected second mini-game.

## E — Police refusal and incident memory

1. Start a loaded T2/T3 run at wanted 0.
2. Gain wanted before Hill Farm and interact with the relay.
3. Confirm legal staff refuse the handoff while the timer keeps running.
4. Lose the police and complete the relay.
5. Gain wanted again at North Wood Yard and confirm the final receiver also refuses.
6. Clear wanted and finish the contract.

Expected: the delivery remains recoverable after escape, but the run remembers the police incident and does not score as a clean logistics run.

## F — Persistent mixed logistics history

1. Complete a ROAD courier, a T1 CARGO route and a T2/T3 CARGO chain with different quality/payout results.
2. Fail at least one loaded CARGO run.
3. Confirm the board shows updated CARGO completed/failed totals and the newest history summary (`tag / payout / quality`).
4. Continue until more than six ROAD/CARGO results exist and confirm history stays bounded to the newest six records.
5. Trigger `SaveProgress`, quit/relaunch or load the primary slot and verify CARGO totals, CARGO lifetime revenue and recent-history entries restore exactly.
6. Load an older v8 save that predates 0.1.4 and confirm new CARGO/history fields default safely while existing reputation, structural damage, dispatch and mission loadouts remain intact.

Expected: 0.1.4 extends save v8 additively without corrupting existing v8 profiles or earlier v5/v6/v7 migration guarantees.

## G — Contract-board UX and economy consistency

1. Compare the board's route tier, market multiplier, schedule, preparation cost and displayed max with the accepted run.
2. Confirm T2 adds the trusted chain bonus and T3 the larger reliable chain bonus.
3. Verify closed-but-unprepared CARGO can still PREP, while closed-and-ready CARGO cannot ACCEPT.
4. Confirm ROAD board keeps its existing reputation/late-shift row and unrelated Tractor/Timber boards keep their previous compact layout.
5. Complete a run and verify the history row updates without introducing another debug-text wall.

Expected: the player can plan time, vehicle readiness and expected value from one board before committing.

## H — Regression and demo evidence

1. Re-run 0.1.3 ROAD courier schedule, Hill Farm relay, police lock and persistence checks.
2. Re-run Mulebox cargo-load physics and fleet-preparation penalty checks.
3. Confirm `Docs/ROADMAP.md` still reports exactly **125/130 (96.2%)** with `SWIR-ROADMAP-STANDARD:v1` intact.
4. For demo readiness, repeat A–G on the packaged Win64 EXE and capture runtime logs/screenshots for closed/open cargo schedule, dynamic market states, T1/T2/T3 route behavior, police-blocked handoff, final payout/history and save/load persistence.

Source-level Project sanity is regression evidence only. It is **not** a UE 5.8 compile/package pass, packaged-runtime smoke pass or rendered visual acceptance.
