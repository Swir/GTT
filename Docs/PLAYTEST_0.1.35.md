# GTT 0.1.35 — Emergency Patch + Farm Cargo Runtime Playtest

Purpose: verify that a packaged Win64 candidate can carry one authoritative Farm Cargo contract through a paid emergency patch, real recovery cooldown, deliberate re-breakdown, paid tow, wrong-vehicle rejection, final delivery and save without transferring or duplicating the load.

> Source sanity can verify only the contract and evidence pipeline. Scenarios marked packaged/runtime require a real UE 5.8 Win64 package and must not be claimed PASS from source CI alone.

1. Start a clean Tier-2-or-higher Farm Cargo route through the real Start terminal.
2. Verify the Farm Job Director enters `ReachPickup` exactly once.
3. Enter the native Mulebox through the normal native takeover path.
4. Park the native Mulebox at Feed Depot and use the real Pickup terminal.
5. Verify the Director enters `DeliverCargo`.
6. Verify cargo authority binds the exact native Mulebox actor.
7. Verify cargo authority records the Mulebox `PersistentVehicleId`.
8. Verify no alternate van receives the load identity.
9. Verify the evidence route does not call a direct contract-completion helper.
10. Verify the evidence route does not add payout cash directly.
11. Move the bound Mulebox away from the pickup terminal before damage staging.
12. Stage mechanical condition near 19% without changing body damage.
13. Stage tire integrity near 20% without changing detached panels.
14. Keep at least the roadside minimum fuel available for the patch probe.
15. Verify the production breakdown assessment returns `TowRecommended`.
16. Verify `bEmergencyPatchPossible` is true.
17. Verify the patch quote is positive.
18. Record cash, route timer and cargo integrity immediately before patch dispatch.
19. Request the patch through `UGTTRoadsideRecoverySubsystem::RequestEmergencyRoadsidePatch`.
20. Verify `NATIVE_ROADSIDE_PATCH_REQUESTED` is emitted with `player_authorized=YES`.
21. Verify Farm Cargo recovery observes patch pending on the exact bound vehicle.
22. Verify `cargo-roadside-patch-pre-service` primary-save checkpoint succeeds.
23. Verify the pre-patch checkpoint does not pause the contract timer.
24. Verify the pre-patch checkpoint does not transfer cargo authority.
25. Wait for the real roadside patch dispatch delay rather than bypassing it.
26. Verify the patch charge equals the quoted emergency patch cost.
27. Verify patched condition reaches at least the 30% limp-home floor.
28. Verify patched tire integrity reaches at least the 32% limp-home floor.
29. Verify patched fuel is at least 5 L when capacity permits.
30. Verify the exact `PersistentVehicleId` is unchanged by the patch.
31. Verify cargo authority still points to the same native Mulebox actor.
32. Verify body-zone health is unchanged by the emergency patch.
33. Verify cooling stress is unchanged by the emergency patch.
34. Verify detached-panel count and detached-panel mask are unchanged.
35. Verify the route timer decreased while patch service was occurring.
36. Verify cargo integrity did not improve during roadside patch service.
37. Verify the patched vehicle still requires workshop-quality repair.
38. Verify `NATIVE_ROADSIDE_PATCH_COMPLETE ... result=PASS` is emitted.
39. Verify production cargo recovery emits `POST_PATCH_VERIFY result=PASS`.
40. Verify `cargo-roadside-patch-post-service` primary-save checkpoint succeeds.
41. Verify post-patch recovery still forbids transfer to another vehicle.
42. Wait through the real roadside recovery cooldown for at least 12 seconds.
43. Verify the Farm Cargo contract remains active throughout cooldown.
44. Verify the exact loaded vehicle remains authoritative throughout cooldown.
45. Verify the route timer continues throughout cooldown.
46. Deliberately damage the same Mulebox again after cooldown.
47. Verify the second damage event creates a tow-capable breakdown state.
48. Record cash, timer, cargo integrity and location before tow dispatch.
49. Request the tow through the production roadside subsystem.
50. Verify `NATIVE_ROADSIDE_TOW_REQUESTED` is player-authorized.
51. Verify `cargo-roadside-tow-pre-move` primary-save checkpoint succeeds.
52. Verify the real tow moves the exact Mulebox to the workshop area.
53. Verify the tow charges a positive cash amount.
54. Verify tow does not repair the staged tire damage.
55. Verify cargo timer continues during tow.
56. Verify cargo integrity does not improve during tow.
57. Verify `POST_RECOVERY_VERIFY result=PASS` preserves exact cargo identity.
58. Park a decoy van at Hill Farm while the bound Mulebox is outside handoff range.
59. Verify Hill Farm rejects the decoy and leaves the Director in `DeliverCargo`.
60. Verify cargo authority remains bound to the native Mulebox after the decoy probe.
61. Return the exact native Mulebox to Hill Farm and complete the real handoff.
62. Verify the Director advances to `DeliverFinalStop` exactly once.
63. Move the same native Mulebox to North Wood Yard and complete the real final handoff.
64. Verify the Director returns to `Idle`.
65. Verify cash increases by a positive delivery payout after recovery costs.
66. Verify Cargo Completed Runs increases by exactly one.
67. Verify logistics reputation increases by a positive amount.
68. Verify cargo authority is cleared after final completion.
69. Verify an explicit final primary save succeeds.
70. Verify no diagnostic failure marker was emitted by the 0.1.35 route.
71. Verify the completion marker contains the same stable vehicle ID used at pickup.
72. Verify patch cost agrees between `PATCH_COMPLETE` and the final completion marker.
73. Verify tow cost agrees between `TOW_COMPLETE` and the final completion marker.
74. Verify payout/reputation/completion deltas agree between phase markers and completion marker.
75. Verify `FARM_CARGO_BREAKDOWN_RUNTIME.json` reports schema `gtt.farm-cargo-breakdown-runtime.v2`.
76. Verify the manifest records all emergency-patch gates as true.
77. Verify the manifest records all legacy tow/exact-vehicle gates as true.
78. Verify the manifest `git_sha` equals the packaged candidate commit SHA.
79. Verify the Win64 smoke process survives at least 280 seconds.
80. Verify `DEMO_TECHNICAL_GATE.json` is schema 10 and refuses any candidate missing the v2 manifest.
81. Verify a candidate fails if the pre-patch checkpoint marker is missing.
82. Verify a candidate fails if post-patch exact-ID verification is missing.
83. Verify a candidate fails if patch body preservation is false.
84. Verify a candidate fails if patch timer continuity is false.
85. Verify a candidate fails if patch cargo-integrity continuity is false.
86. Verify a candidate fails if the real cooldown is shorter than 12 seconds.
87. Verify a candidate fails if patch charge is zero or differs from the quote.
88. Verify a candidate fails if the wrong vehicle is accepted after tow.
89. Verify a candidate fails if final payout is non-positive.
90. Verify a candidate fails if completion history increments by anything other than one.
91. Verify a candidate fails if final authority cleanup is missing.
92. Verify a candidate fails if final save evidence is missing.
93. Verify the evidence route restores baseline Mulebox migration state after completion.
94. Verify the evidence route restores baseline body damage and detached-panel state.
95. Verify the evidence route restores baseline logistics/economy/day-night state.
96. Verify temporary decoy actors are destroyed after the route.
97. Verify the route remains inert without the packaged smoke command-line flags.
98. Verify normal gameplay does not auto-run or auto-award this evidence contract.
99. Verify the Roadmap remains 125/130 until real Win64/Chaos/trailer gates are proven.
100. Verify no GitHub Demo Release is published solely because source sanity passes.
