# GTT 0.1.21 Playtest — Warden enforcement and police handoff

This checklist is for a real Unreal gameplay session. Source-contract CI may validate wiring, but it does not replace the Win64/runtime gates tracked separately in the Roadmap.

## Setup

Use a fresh sandbox session with the player near the forest poaching interaction. Keep the message HUD visible. Test once during daylight and once after the day/night cycle reports night (`<06:00` or `>=21:30`). Have enough cash to observe fines and leave the county-police response systems enabled.

## Runtime scenarios

1. **Day attempt creates wildlife response** — perform one forest-poaching attempt in daylight. Confirm wildlife heat rises, at least one ranger is dispatched and the normal poaching result still resolves.
2. **Cooldown remains authoritative** — immediately interact again. Confirm the forest-quiet cooldown blocks a second roll and does not add another crime.
3. **Contraband creation** — repeat after cooldown until an animal is obtained. Confirm rural contraband units/value increase and the existing poaching message reports the stash.
4. **Citation reaches the same inventory** — allow a ranger to reach citation radius. Confirm the normal warden citation fires and all current poaching contraband is seized.
5. **Seizure is persistent** — after the citation, visit/use the fence path. Confirm the seized contraband cannot still be sold after the rural-economy save reloads.
6. **No phantom seizure** — trigger a later citation while carrying no contraband. Confirm no bogus positive-unit seizure message appears.
7. **Alert-3 police handoff** — commit enough wildlife crimes to reach alert 3. Confirm `WARDEN RADIO HANDOFF` appears and the existing wanted meter gains police heat.
8. **Handoff is one-shot per incident** — remain at wildlife alert 3 for several ranger response ticks. Confirm wanted heat is not repeatedly added every second by the director.
9. **Police systems actually consume the handoff** — after alert-3 handoff, confirm ordinary police response reacts to the resulting wanted level; there must be no separate fake police state owned by the ranger system.
10. **Incident re-arm** — clear the wildlife incident normally (citation/decay), then create a new alert-3 incident. Confirm exactly one new police handoff can occur for the new incident.
11. **Night risk** — after 21:30 or before 06:00, perform the same poaching attempt and compare wildlife heat. Confirm the after-dark attempt applies the configured 1.25x heat severity.
12. **Night reward** — obtain the same class of poaching result after dark and verify its generated estimated fence value receives the configured 1.18x night multiplier.
13. **Night reinforcement** — reach wildlife alert 2 at night. Confirm two rangers may be active; repeat at alert 2 during daylight and confirm the normal one-ranger response remains.
14. **No over-spawn** — hold alert 3 at night and confirm the director never exceeds two active rangers.
15. **Legal-work consequence remains connected** — while wildlife alert is active, attempt to start a legal farm job. Confirm the existing game-w arden block still prevents taking the legal job until the incident is resolved.
16. **Wanted and warden are independent after handoff** — accept a warden citation after a police handoff. Confirm wildlife alert clears, but any police wanted state follows its existing police/wanted rules rather than being silently erased by the ranger system.
17. **Day/night transition stability** — cross the 21:30 boundary while alert 2 is active. Confirm night reinforcement can appear without resetting wildlife heat or duplicating police handoff heat.
18. **Save/load sanity** — after a warden seizure, save/load through the normal path and confirm confiscated contraband stays gone; do not expect transient active-ranger actors themselves to persist.

## Demo relevance

This milestone improves the live sandbox loop that the eventual demo must show: illegal rural work now has a readable risk/reward choice, a real game-warden pursuit, inventory consequences and an escalation path into the existing county-police chase. Demo publication is still forbidden until the exact candidate passes the separate UE 5.8 Win64 package/runtime, Native Chaos/trailer, rendered evidence and human visual review gates.
