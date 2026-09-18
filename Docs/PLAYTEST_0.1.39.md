# GTT 0.1.39 Playtest — Persistent Roadside Dispatch & Save/Load Recovery

> Source milestone test plan. Items that require an Unreal runtime remain acceptance scenarios, not claims of execution.

## Checkpoint creation

1. Start an eligible emergency patch request and verify a sidecar checkpoint is created before service completes.
2. Start an eligible tow request and verify the checkpoint records RoadsideAssistance rather than PoliceImpound.
3. Verify the patch checkpoint stores the request-time locked quote, not a later recomputed estimate.
4. Verify the tow checkpoint stores the request-time locked quote, not a later recomputed estimate.
5. Verify the checkpoint stores the exact native vehicle PersistentVehicleId.
6. Verify remaining ETA decreases in saved checkpoints while the service is live.
7. Verify checkpoint writes are throttled rather than occurring every frame.
8. Verify no checkpoint is written when there is no voluntary roadside service.

## Reload and exact-ID recovery

9. Create a patch checkpoint, restart the session and verify restore waits until the exact vehicle actor exists.
10. Create a tow checkpoint, restart the session and verify restore waits until the exact vehicle actor exists.
11. Verify restore waits while the exact vehicle has no driver/takeover instead of transferring service.
12. Enter the exact vehicle after reload and verify the patch dispatch rebinds to it.
13. Enter the exact vehicle after reload and verify the tow dispatch rebinds to it.
14. Verify the restored locked quote equals the pre-reload locked quote.
15. Verify the restored ETA is the saved remaining ETA rather than a fresh full dispatch duration.
16. Verify a second same-model vehicle cannot satisfy the saved PersistentVehicleId.

## Economy and transaction safety

17. Verify checkpoint loading never calls SpendCash.
18. Verify checkpoint rebinding never calls SpendCash.
19. Verify checkpoint creation never calls SpendCash.
20. Verify restored patch charges exactly once only when the existing production completion succeeds.
21. Verify restored tow charges exactly once only when the existing production completion succeeds.
22. Verify cancellation after restore removes the checkpoint and charges nothing.
23. Verify insufficient cash at eventual arrival follows the existing completion denial and does not create a second charge path.
24. Verify an invalid restore is discarded with charged=NO evidence.

## Invalidation and law response

25. Reload a checkpoint while Wanted is active and verify voluntary service is rejected without charge.
26. Reload a checkpoint after the vehicle is no longer recovery-eligible and verify it is rejected.
27. Reload a patch checkpoint when emergency patch is no longer valid and verify it is rejected.
28. Inject an invalid service mode and verify the checkpoint is deleted.
29. Inject a zero/negative quote and verify the checkpoint is deleted.
30. Inject zero/negative remaining ETA and verify the checkpoint is deleted.
31. Create two actors with the same persistent ID in a test build and verify restore rejects the ambiguity.
32. Verify PoliceImpound is never written into the voluntary dispatch checkpoint.

## Farm Cargo continuity

33. Accept Farm Cargo, bind the Mulebox, request patch and verify checkpoint target equals FarmCargoBoundVehicleId.
34. Accept Farm Cargo, bind the Mulebox, request tow and verify checkpoint target equals FarmCargoBoundVehicleId.
35. Reload an active cargo + patch checkpoint and verify only the bound cargo Mulebox may restore the service.
36. Reload an active cargo + tow checkpoint and verify only the bound cargo Mulebox may restore the service.
37. Tamper the sidecar to another vehicle ID while cargo is active and verify the checkpoint is rejected.
38. Verify cargo authority remains owned by the primary Farm Cargo systems, not the sidecar.
39. Verify restoring roadside dispatch does not reserve depot stock again.
40. Verify restoring roadside dispatch does not pay cargo revenue or reputation.

## Lifecycle cleanup

41. Complete a normal patch and verify the sidecar is deleted on the next persistence scan.
42. Complete a normal tow and verify the sidecar is deleted on the next persistence scan.
43. Cancel a patch before arrival and verify the sidecar is deleted.
44. Cancel a tow before arrival and verify the sidecar is deleted.
45. Let Wanted cancel a voluntary dispatch and verify the sidecar is deleted.
46. Let eligibility disappear before arrival and verify the sidecar is deleted.
47. Restart after a completed service and verify no dispatch resurrects.
48. Restart after a cancelled service and verify no dispatch resurrects.

## HUD and player-facing continuity

49. Restore a patch and verify the HUD reads PATCH from the existing authoritative recovery subsystem.
50. Restore a tow and verify the HUD reads TOW from the existing authoritative recovery subsystem.
51. Verify HUD locked quote after restore matches the pre-reload accepted quote.
52. Verify HUD ETA after restore begins near the saved remaining ETA.
53. Verify HUD target ID after restore is the exact restored PersistentVehicleId.
54. Verify active Farm Cargo still reports CARGO PIN OK when the same bound vehicle is restored.
55. Verify same-key cancel works after a restored dispatch.
56. Verify opposite-service conflict rules still apply after a restored dispatch.

## Regression / compatibility

57. Verify a profile with no roadside sidecar loads normally.
58. Verify an invalid/old sidecar schema is safely deleted without touching the primary save.
59. Verify primary SaveVersion remains unchanged by this transactional sidecar milestone.
60. Verify the primary world snapshot remains the authority for cash, cargo, fleet and story progression.
61. Verify the sidecar records primary revision only as diagnostics and does not manufacture a new revision.
62. Verify 0.1.36 locked-quote/exact-target contract source sanity still passes.
63. Verify 0.1.37 dispatch HUD source sanity still passes.
64. Verify 0.1.38 Farm Cargo dispatch runtime source contract still passes.

## Release / presentation honesty

65. Run the new 0.1.39 source verifier and require PASS.
66. Run deterministic progress SVG check and require 125/130 = 96.2%.
67. Verify README contains exactly one progress-card SVG and no duplicate mini SVG.
68. Verify ROADMAP contains exactly one progress-mini SVG and no character progress meter.
69. Verify progress-template remains TEMPLATE/N/A and is not embedded as project data.
70. Verify source CI does not claim an Unreal compile.
71. Verify source CI does not claim packaged Win64 save/load evidence.
72. Do not create a Demo Release until the real Win64/package/runtime/visual gates all pass.

## Required evidence boundary

- Source verifier PASS proves repository wiring and invariant checks only.
- Runtime scenarios must be executed in Unreal before they can be marked as runtime evidence.
- Packaged Win64 save/load proof requires a qualifying Unreal Engine 5.8 Windows runner.
- Demo release remains blocked until the existing technical and visual gates are all satisfied.
