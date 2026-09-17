# Farm Cargo packaged runtime acceptance — 0.1.28

GTT 0.1.28 adds a dedicated packaged-game exercise for the legal Farm Cargo vertical slice. It is deliberately separate from the long general demo smoke so a cargo failure has an isolated log and cannot be hidden by unrelated police/combat evidence.

## Runtime route

A packaged `GTT.exe` is launched with `-GTTFarmCargoScenario` in an isolated user directory. The scenario uses the real Farm Cargo director and real terminals. It clears incidental Wanted state, starts during the depot work window, accepts a real stock-backed contract, places the Native Mulebox at the Feed Depot and invokes the pickup terminal. The pickup terminal must lock the exact actor that received the cargo.

The scenario then moves that exact loaded vehicle more than 750 cm away from Hill Farm and invokes the Hill Farm terminal. The terminal must refuse the handoff through the normal exact-vehicle authority path; a healthy decoy nearby must never be enough. The loaded Mulebox is then staged inside the legal yard, stopped, and the same real terminal is used again. Tier-1 work completes there; higher-tier work continues through the same load to North Wood Yard.

A successful route must finish through the existing economy, logistics-reputation and save authorities. The evidence scenario observes positive payout and a completed cargo-run increment; it does not inject cash or reputation itself.

## Evidence files

`smoke_test_farm_cargo_windows.ps1` writes `FARM_CARGO_SMOKE.json` only after the packaged process emits `FARM_CARGO_SCENARIO_COMPLETE result=PASS`. `evaluate_farm_cargo_runtime.ps1` consumes that smoke manifest plus `GTT_FARM_CARGO_RUNTIME.log` and writes `FARM_CARGO_RUNTIME.json` with schema `gtt.farm-cargo-runtime.v1`.

A PASS manifest proves the exact loaded vehicle ID was captured, the wrong-vehicle / loaded-vehicle-away case was rejected beyond 750 cm, every accepted handoff used the same vehicle at no more than 3.0 km/h and no more than 750 cm from its terminal, the contract completed, cash increased, the Cargo completed-run counter advanced and `SaveProgress()` succeeded.

## What this does not prove

Source sanity for these scripts **does not prove** Unreal C++ compilation, Win64 packaging, Native Chaos correctness, final authored trailer assets, rendered visual quality or demo readiness. Those claims require the self-hosted Windows x64 + Unreal Engine 5.8 pipeline to actually produce matching runtime evidence from the exact candidate commit. Until that happens, the five hardware/runtime Roadmap items remain open and no release is authorized.
