# GTT 0.1.59 — Heavy Haul Driving Quality Playtest

## Scope

This milestone makes the loaded timber leg reward deliberate heavy-trailer driving instead of treating every successful arrival the same. It also connects loaded speed/attitude to the existing breakable-hitch physics so rough hauling has a bounded physical consequence. It does **not** claim authored trailer assets, Native Chaos runtime acceptance, a Win64 packaged build, or demo readiness.

## Setup

Use the Heavy Haul contract from Player Farm. Hitch an eligible owned Fieldmaster, tow the empty trailer to North Wood Yard, load the logs, then drive the loaded trailer to Hill Farm.

## Manual acceptance cases

- [ ] Starting a new Heavy Haul contract clears smooth/rough driving-quality counters.
- [ ] Loading timber clears the counters again so only the loaded leg is scored.
- [ ] The load message explains the 16–52 km/h smooth band, stability requirement, 60 s target and bonus.
- [ ] Driving 16–52 km/h with hitch load at or below 32%, modest roll/pitch, intact axle and healthy cargo accumulates smooth time.
- [ ] Standing still does not accumulate smooth time.
- [ ] Crawling below 16 km/h does not accumulate smooth time.
- [ ] Driving above 52 km/h does not accumulate smooth time even if it is not yet rough.
- [ ] Speed above 65 km/h accumulates rough exposure.
- [ ] Hitch load above 58% accumulates rough exposure.
- [ ] Roll above 22 degrees accumulates rough exposure.
- [ ] Pitch above 17 degrees accumulates rough exposure.
- [ ] A lost trailer wheel accumulates rough exposure.
- [ ] Re-hitching the loaded trailer preserves the loaded-leg driving-quality counters.
- [ ] Reaching 60 s smooth time while rough exposure is within 12 s reports that the smooth-haul bonus is armed.
- [ ] Exceeding 12 s rough exposure reports that the smooth-haul bonus is lost.
- [ ] The Heavy Haul objective displays smooth time, rough time and `BONUS ARMED` only when the live gate is true.
- [ ] Delivery with armed gate, cargo >=90% and trailer >=70% adds the $180 smooth-haul bonus exactly once.
- [ ] Delivery without the gate does not add the smooth-haul bonus.
- [ ] Existing fast bonus remains independent and may stack with the smooth-haul bonus.
- [ ] Existing roadside repair time penalty remains active.
- [ ] A pending roadside repair objective uses the locked quote rather than a recomputed future quote.
- [ ] Completion log includes reward, fast status, smooth bonus status, smooth seconds, rough seconds, cargo and trailer condition.
- [ ] Contract reset clears all driving-quality state.
- [ ] Runtime suspension travel remains 24 cm after the dynamics subsystem refreshes the axle constraints.
- [ ] Empty-trailer speed/attitude does not add artificial dynamic hitch stress beyond measured hitch load.
- [ ] Loaded-trailer dynamic hitch stress starts rising above 52 km/h and reaches the bounded full stress input at 78 km/h.
- [ ] Loaded roll and pitch progressively add dynamic hitch stress, reaching full attitude stress at 28° roll or 20° pitch.
- [ ] The strongest of hitch load, loaded speed stress and loaded attitude stress drives the existing bounded hitch weakening.
- [ ] Full dynamic stress weakens break thresholds by at most 28%; integrity/load multipliers remain independently active.
- [ ] `TRAILER_NATIVE_DYNAMICS` evidence reports dynamic stress, speed, roll and pitch from the same runtime snapshot.

## Source verification

Run:

```bash
python Scripts/verify_v0_1_59_heavy_haul_driving_quality.py
```

The verifier checks source integration plus deterministic driving-quality and trailer-dynamics threshold math. It is not a substitute for an Unreal Editor/Win64 runtime playtest.

## Demo gate

This milestone alone does not satisfy the demo release gate. A real UE 5.8 Win64 package, packaged EXE runtime smoke test, authored/accepted trailer and vehicle presentation, green relevant CI, and visual acceptance are still required.
