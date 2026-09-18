# GTT 0.1.42 — Packaged Workshop Recovery + Farm Cargo Service Playtest

This milestone adds packaged-runtime evidence for the connected 0.1.41 tow → garage hold → workshop service loop. It does **not** claim that Win64 evidence exists until the qualifying Unreal Engine 5.8 runner actually executes this route and produces the required manifest.

## A. Clean start and contract setup

1. Launch the packaged candidate with the deterministic demo/Farm Cargo flags plus `-GTTFarmCargoWorkshopRecoveryScenario`.
2. Confirm `FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_BEGIN version=1` appears only when the dedicated flag is present.
3. Confirm the new route starts after the earlier dispatch-persistence evidence window rather than overlapping it.
4. Confirm the route refuses to start from an active illegal/wildlife state.
5. Confirm Wanted is cleared for the legal evidence setup.
6. Confirm enough cash is available for both tow and workshop charges without modifying production prices.
7. Confirm Cargo route tier is at least tier 2 before acceptance.
8. Confirm the Feed Cargo start terminal is used rather than directly mutating the job director.
9. Confirm job stage changes from `Idle` to `ReachPickup` through the real terminal.
10. Confirm no payout, reputation or cargo completion is granted at contract acceptance.

## B. Exact Mulebox pickup authority

11. Enter the native Mulebox through its normal interaction path.
12. Use the real pickup terminal.
13. Confirm the Farm Cargo authority binds the exact native Mulebox actor.
14. Confirm bound `PersistentVehicleId` equals the native Mulebox persistent ID.
15. Capture cargo timer before recovery.
16. Capture cargo integrity before recovery.
17. Confirm the contract reaches `DeliverCargo`.
18. Confirm a decoy van exists for later wrong-vehicle rejection.
19. Confirm no substitute vehicle is marked as cargo authority.
20. Confirm route stock/reservation state remains owned by the existing Farm Cargo systems.

## C. Production roadside tow

21. Move the exact Mulebox away from the pickup terminal into the controlled evidence location.
22. Stage condition/tire state into the production `TowRecommended` range.
23. Confirm the breakdown subsystem reports `TowRecommended`.
24. Record condition and tire integrity before tow.
25. Record player cash before requesting tow.
26. Request tow through `UGTTRoadsideRecoverySubsystem::RequestRoadsideTow`.
27. Confirm the request succeeds.
28. Confirm a positive locked tow quote exists.
29. Confirm the pending target ID equals the loaded cargo vehicle ID.
30. Confirm cash has not changed before tow arrival.
31. Confirm pending mode is roadside assistance.
32. Allow the real 2.5-second production dispatch to complete.
33. Confirm the pending tow clears after completion.
34. Confirm cash decreased by exactly the locked tow quote once.
35. Confirm vehicle condition after tow matches pre-tow condition within tolerance.
36. Confirm tire integrity after tow matches pre-tow tire integrity within tolerance.
37. Confirm persistent vehicle identity is unchanged.
38. Confirm Farm Cargo authority still points to the exact Mulebox.
39. Confirm the native production log says `damage_preserved=YES`.
40. Confirm the native production log says `identity_preserved=YES` and `serviced=NO destination=WORKSHOP`.

## D. Workshop destination and hard garage hold

41. Confirm the towed Mulebox is in the workshop district near the real workshop terminal.
42. Build the authoritative garage fleet snapshot.
43. Confirm the exact Mulebox fleet entry exists.
44. Confirm its service status is `TOW` or `IMMOBILE`.
45. Confirm `IsVehicleOnWorkshopHold(exact-id)` returns true.
46. Confirm `GetWorkshopHoldCount()` includes at least this vehicle.
47. Confirm the hold is derived from authoritative fleet/breakdown state rather than a standalone test flag.
48. Confirm LIMP/SERVICE policy semantics have not been changed by the runtime evidence route.
49. Confirm cargo authority remains exact while the vehicle is held.
50. Confirm no workshop service has been silently applied by ordinary tow.

## E. Garage recall bypass prevention

51. Resolve the numbered garage slot whose snapshot owns the exact Mulebox ID.
52. Record cash immediately before recall attempt.
53. Record the exact native Mulebox transform immediately before recall attempt.
54. Interact with the actual garage slot terminal.
55. Confirm interaction is rejected while WORKSHOP HOLD is active.
56. Confirm no garage recall fee is charged.
57. Confirm the native Mulebox does not move to the garage bay.
58. Confirm the hold remains active after the rejected recall.
59. Confirm cargo authority remains bound to the same persistent vehicle ID.
60. Confirm the rejected recall does not advance or complete the cargo contract.

## F. Authoritative paid workshop service

61. Move the exact towed Mulebox into the normal workshop interaction radius.
62. Query the existing native road repair quote before service.
63. Confirm the workshop quote is positive.
64. Record cash before workshop interaction.
65. Record cargo timer and integrity before workshop service.
66. Interact with the actual workshop terminal.
67. Confirm cash decreases by exactly the queried workshop quote once.
68. Confirm no second/double workshop debit is observed.
69. Confirm native condition returns to the workshop-serviced ready state.
70. Confirm native tire integrity returns to the workshop-serviced ready state.
71. Confirm fuel is restored to capacity within tolerance.
72. Confirm the same persistent vehicle ID survives workshop service.
73. Confirm Farm Cargo authority still binds that exact Mulebox.
74. Confirm `IsVehicleOnWorkshopHold(exact-id)` becomes false because the underlying authoritative damage state changed.
75. Confirm the hold is not cleared by a special test-only flag.
76. Confirm cargo timer did not reset or gain time during tow/workshop service.
77. Confirm cargo integrity did not improve because of vehicle workshop service.
78. Confirm the normal game save path is still invoked by workshop service.

## G. Cargo continuity after service

79. Move the serviced exact Mulebox away from Hill Farm.
80. Move the decoy vehicle inside the Hill Farm handoff area.
81. Attempt the Hill Farm handoff using the decoy situation.
82. Confirm the wrong vehicle is rejected and stage remains `DeliverCargo`.
83. Confirm cargo authority still points to the exact Mulebox.
84. Re-enter the exact native Mulebox through normal interaction if required after tow exit.
85. Move the exact Mulebox to Hill Farm.
86. Complete Hill Farm through the real terminal.
87. Confirm stage advances to `DeliverFinalStop`.
88. Confirm the same exact vehicle remains authoritative between Hill Farm and final stop.
89. Move the exact Mulebox to North Wood Yard/final cargo terminal.
90. Complete the final handoff through the real terminal.
91. Confirm stage returns to `Idle`.
92. Confirm cargo authority clears after final delivery.
93. Confirm exactly one cargo completed-run increment.
94. Confirm payout delta is positive.
95. Confirm logistics reputation delta is positive.
96. Confirm the final explicit `SaveProgress()` succeeds.

## H. Runtime manifest evaluator

97. Run `Scripts/evaluate_farm_cargo_workshop_recovery_runtime.ps1` against the packaged runtime log.
98. Confirm all required phase markers are present and PASS.
99. Confirm any `DIAGNOSTIC result=FAIL` marker makes evaluation fail.
100. Confirm the evaluator requires a positive tow quote.
101. Confirm the evaluator requires the tow cash delta to equal the locked quote.
102. Confirm the evaluator requires preserved damage and identity.
103. Confirm the evaluator requires workshop destination proof.
104. Confirm the evaluator requires hard hold observation.
105. Confirm the evaluator requires garage recall blocked + no charge + no movement.
106. Confirm the evaluator requires positive workshop quote and exact single workshop debit.
107. Confirm the evaluator requires repaired + refuelled + hold cleared.
108. Confirm exact cargo identity and timer/integrity continuity are required.
109. Confirm wrong-vehicle rejection, Hill Farm, final handoff, save and authority cleanup are required.
110. Confirm one payout/completion/reputation result is required.
111. Confirm the native production `NATIVE_ROADSIDE_TOW_COMPLETE` marker is required.
112. Confirm a BUILD_INFO SHA mismatch causes evaluation failure.
113. Confirm the output file is `FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json`.
114. Confirm schema is exactly `gtt.farm-cargo-workshop-recovery-runtime.v1`.

## I. Demo technical gate promotion

115. Run the existing schema-12 demo technical evaluator first.
116. Confirm workshop promotion refuses a missing `DEMO_TECHNICAL_GATE.json`.
117. Confirm workshop promotion refuses a gate whose result is not PASS.
118. Confirm workshop promotion refuses a gate whose schema is not 12.
119. Confirm workshop promotion refuses a missing workshop runtime manifest.
120. Confirm workshop promotion refuses workshop result other than PASS.
121. Confirm workshop promotion refuses the wrong workshop schema.
122. Confirm exact candidate SHA is required across gate and workshop manifest.
123. Confirm all workshop boolean gates are rechecked rather than trusting only `result=PASS`.
124. Confirm positive tow/workshop charges are rechecked.
125. Confirm exactly one cargo completion and positive payout/reputation are rechecked.
126. Confirm production tow marker count must be positive.
127. Confirm diagnostic failure count must be zero.
128. Confirm only then is the demo gate promoted to schema 13.
129. Confirm schema 13 records workshop vehicle ID, quotes, hold, garage block, service, hold clear, exact vehicle and delivery deltas.
130. Confirm presentation/roadmap completion is not modified by gate promotion.

## J. Win64 evidence workflow and release safety

131. Confirm Win64 workflow default candidate version is 0.1.42.
132. Confirm deterministic NullRHI smoke includes `-GTTFarmCargoWorkshopRecoveryScenario`.
133. Confirm minimum runtime is long enough to pass the 356–382 second evidence window.
134. Confirm the workflow evaluates workshop recovery before technical gate promotion.
135. Confirm the new manifest is included in technical bundle validation.
136. Confirm the existing schema-12 evaluator still has to PASS before promotion.
137. Confirm the final bundle asserts schema 13 and workshop-recovery PASS.
138. Confirm the new manifest is uploaded with successful candidate evidence.
139. Confirm the new manifest is also retained in failure diagnostics when available.
140. Confirm a source-only Linux CI pass does not manufacture this runtime manifest.
141. Confirm no public Release is created without actual Win64 packaged evidence.
142. Confirm no public Release is created without the existing Native Chaos, authored trailer and rendered visual gates.

## K. SWIR presentation and roadmap invariants

143. Confirm `Docs/ROADMAP.md` retains `<!-- SWIR-ROADMAP-STANDARD:v1 -->`.
144. Confirm `ROADMAP-PROGRESS` contains exactly one `progress-mini.svg` and no ASCII/Unicode meter.
145. Confirm roadmap table remains Completed 125 / Remaining 5 / Total 130 / Progress 96.2%.
146. Confirm `README.md` retains `<!-- SWIR-README-STANDARD:v2 -->`.
147. Confirm README contains exactly one project `progress-card.svg` in project status.
148. Confirm `progress-template.svg` remains TEMPLATE/N/A project-neutral data.
149. Run `python Scripts/generate_progress_svg.py --check` and require PASS.
150. Confirm 0.1.42 does not check off any Native Chaos/Win64/trailer/visual blocker without qualifying runtime evidence.
