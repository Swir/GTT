# GTT 0.1.69 AUDIT-FIRST closeout

Status: **IN PROGRESS**  
Canonical roadmap: **125/130 = 96.2%**  
Integration lane: PR #196 `audit/0.1.69-final-gates`

This document is an engineering checkpoint for the FINISH-FIRST audit. It is not release evidence and it does not close any of the five remaining roadmap gates.

## Failure-class history reviewed

Recent exact UE 5.8 Win64 candidate diagnostics were reviewed as a sequence rather than as isolated failures:

| Candidate SHA | Observed failure class |
| --- | --- |
| `8c6af457c0dd916f5e77f50e5edbb404d3ecf074` | pursuit closing, physical spike crossing/handling, post-spike escape, damage/workshop persistence and structural recovery |
| `a51e9155b5e5d71eb7c34c095f9d87e8ca2ddff2` | same family plus Native road control instability |
| `31d89aa9cecd44fddf43c1239ddd28370d88efee` | rig/import/PhysicsAsset and Native motion/control failures plus downstream scenario failures |
| `f40a9a8081c28271aeb56e3be8f496555e2003c5` | physical spike consequence became real; pursuit closing, post-spike escape and structural persistence remained |
| `c3868f0074ee770bb12bf2337d6060b41e70ecec` | spike consequence passed on Rattleback82, but damage persistence later selected Mulebox1200; workshop recovery failed while later structural evidence continued mutating shared state |
| `d3a413743db7df588f14606aa99649470e602c67` | contains deterministic pursuit/post-spike/workshop staging fixes, but the full candidate acceptance was cancelled before these changes received end-to-end evidence |

Historical failing candidates still produced `RUNTIME_SMOKE.json result=PASS`: the packaged executable stayed alive. Their dominant remaining defect class was deterministic acceptance/evidence orchestration, not proof of a clean current candidate.

## Five-gate dependency map

1. **Exact UE 5.8 Win64 runner/toolchain/package**
   - Recent self-hosted preliminary UE 5.8 qualification passed before the candidate stage.
   - Current qualification requires preflight, `GTTEditor Win64 Development` build and `UnrealEditor-Cmd -NullRHI` probe.
   - This gate is blocked until the audit branch itself is compile-safe and all cheap contracts are clean.

2. **Dedicated Native Chaos tractor movement + drivetrain/suspension/wheels**
   - Static architecture, generated rig axis contract and root-only chassis PhysicsAsset policy are guarded in source CI.
   - Final closure still requires exact-candidate packaged runtime movement, wheel, suspension, contact and drivetrain evidence with no physics fallback.

3. **Authored skeletal trailer wheels + final hitch sockets**
   - Trailer editor evidence is exact-SHA/source-hash bound.
   - Final closure still requires real packaged wheel contact, hitch safety, loaded motion and controlled-stop evidence.

4. **Full Unreal compile/package/packaged EXE smoke**
   - Historical candidates proved packaged EXE survival, not the current PR head.
   - The current exact head must compile, cook fail-closed, package and execute before this gate can close.

5. **Rendered/human/demo technical acceptance**
   - Technical flow requires five rendered evidence frames and the sealed technical gate.
   - Human visual review remains explicitly `REQUIRED`; technical automation must not authorize a Demo Release.

## Systemic findings

### Native CDO / cook safety
`Scripts/verify_cdo_physics_safety.py` recursively scans every `Source/**/*.cpp` native A/U constructor and rejects body/material/mass operations including `SetMassOverrideInKg`, `SetCenterOfMass`, `GetSimplePhysicalMaterial`, `SetPhysMaterialOverride`, `SetMassScale`, `GetBodyInstance` and direct `BodyInstance` access. It also rejects `-IgnoreCookErrors` and the former cook allow-list token. Exact-head Project sanity must remain green.

### Runtime scenario isolation
The packaged smoke path is split into isolated `CORE`, `NATIVE` and `SERVICES` passes with separate UserDir state and separate logs. This is required so save/load, workshop, drivetrain/trailer and service scenarios cannot pre-empt each other's evidence.

### Spike -> persistence binding
The CORE roadblock evidence can prove a specific Native road vehicle was spiked, but the damage persistence subsystem currently searches for the owned Native road vehicle with the lowest tire integrity. Recent diagnostics proved this can select a different vehicle. The recovery evidence must bind to the roadblock's exact `GetLastSpikedVehicleId()` after `HasProvenSpikeConsequence()`.

### Shared-state sequencing
Damage recovery, structural recovery and structural-drive evidence share the save slot, workshop, player economy and vehicle state. A downstream subsystem must not continue because a predecessor merely stopped ticking; it must require explicit predecessor success before mutating shared state.

### Workshop quote determinism
Evidence code contains fixed cash reserves (250/450/550). The real service price is already exposed by `AGTTServiceTerminal::GetNativeRoadCheckoutQuote`. Evidence must reserve the actual quote and verify the exact charge rather than depend on incidental current cash or historical price constants.

### Native rig provenance
Generated Native Chaos rigs use deterministic project-owned glTF sources with the documented glTF Y-up -> UE Z-up axis contract and root-only chassis PhysicsAsset stabilization. As of commit `8e6f952b7ddb0ad22bab1fbdee9deb809eec0a46`, editor import evidence records the source glTF filename, byte count and SHA-256 for every generated vehicle rig. The remaining handoff work is to require and semantically verify those source hashes through the sealed candidate/archive boundary.

### Current compile-safety blocker
`Source/GTT/Private/Core/GTTDamageRecoveryEvidenceSubsystem.cpp` on the audit branch contains literal `\n` escape text between three include directives introduced with runtime-isolation work. Source-only sanity did not catch this. The full runner must not be dispatched until this is corrected and protected by a cheap source-text guard.

## Required remediation before one final full runner

- [ ] Correct the malformed DamageRecovery include block.
- [ ] Bind damage recovery to the exact roadblock-spiked vehicle.
- [ ] Replace fixed workshop evidence reserves with the real checkout quote and verify exact payment.
- [ ] Require predecessor-success handoff for damage -> structural -> structural-drive evidence.
- [ ] Finish Native rig source-hash handoff through exact candidate attestation/archive verification.
- [ ] Add cheap regression guards for the failure classes above.
- [ ] Require all exact-head cheap/source CI to pass after the complete remediation package.
- [ ] Run **one** final exact UE 5.8 Win64 qualification + build/cook/package/runtime/rendered technical acceptance.
- [ ] Keep human visual review and Demo Release authorization outside technical automation.

Audit completion remains false until the checklist above and the user-defined final acceptance condition are satisfied.
