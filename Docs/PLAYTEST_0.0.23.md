# GTT 0.0.23 Playtest — North Pass Run

## Arc 4 unlock and faction proof
1. Finish Main Story Arc 3 and return to the Player Farm Arc 4 terminal.
2. Verify Arc 4 refuses to start with active police/ranger attention.
3. Start North Pass Run and confirm the objective requires one **new** faction victory after Arc 4 begins.
4. Complete a Rust Dogs, Stone Crows or Mud Jackals encounter and verify the objective advances.

## Contraband / Backlot Fence integration
1. Use the existing forest poaching loop until the persistent stash reaches at least 2 units.
2. Verify Arc 4 advances to the fence stage without granting story-only fake cargo.
3. Try to fence while police/ranger attention is active and verify the existing fence refuses the sale.
4. Clear attention, sell at Backlot Fence and verify Arc 4 recognizes the stash becoming empty.

## North Pass / police escape / ridge exchange
1. Follow `NEXT ROAD` guidance through the new North Pass Approach and checkpoint road nodes.
2. Hit the North Pass terminal and verify a large real wanted response is added.
3. Escape using the existing police pursuit/roadblock system; Arc 4 must not advance while wanted remains active.
4. After wanted clears, follow routing to Ridge Exchange.
5. Verify Ridge Exchange refuses the handoff if police/ranger attention returns.
6. Complete the exchange and verify `$1250` reward, then return to Player Farm for the `$1500` Arc 4 completion reward.

## Persistence
1. Quit/relaunch during faction proof, prepared-stash, police-escape and ridge-exchange stages.
2. Verify `GTT_MainStory_Arc4_01` restores stage, baseline faction victories and the prepared-stash flag.
3. Confirm faction and contraband state remain owned by their existing persistent systems.

## Countryside / routing regression
- Shared road graph contains at least 28 nodes.
- Routes to North Pass, River Ford, Ridge Exchange and Quarry North Cut are connected to the older Old Quarry/North Wood network.
- Existing traffic/police/story code can still call the shared graph without duplicate Arc 4 topology.
- Road-law metadata remains present on all new nodes.

## Known limitation
Repository CI is structural Python sanity coverage only. It does **not** perform a real Unreal Engine 5.8 Win64 compile/package/smoke test, so packaged executable behavior is not claimed as verified.
