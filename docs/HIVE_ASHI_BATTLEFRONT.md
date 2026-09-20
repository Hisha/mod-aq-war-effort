# Hive'Ashi Ten Hour War battlefront

The subsequent [Hive'Regal milestone](HIVE_REGAL_BATTLEFRONT.md) moves this exact
Ashi roster into shared battlefront data. Ashi stage/death behavior is unchanged;
both fronts now reconcile independently from the same campaign clock.

## Data inventory and scope

Sources inspected: this module's `AQWarContent.*`, its timer and previous content
notes; the old `mod-war-effort/data/sql/db-world/updates/warevent.sql`; and the
upstream AzerothCore `zone_silithus.cpp` and `Map.h`. The earlier module content
audit records stock creature template inspection at AzerothCore commit
`06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`. No local AzerothCore checkout
or live world DB is present in this workspace, so template values and custom DB
overrides still require deployment verification. Runtime template presence is
checked before summoning. No reference SQL is imported.

| Entry | Reference/stock role | Decision |
|---|---|
| 15421 Qiraji Drone | Stock, unbound template; related to stock quest 8519 Qiraji scene | Two in stage 1, third in stage 2 |
| 15758 Supreme Anubisath Warbringer | Four legacy Silithus groups, including a group northeast of Ashi | One in stage 2, second in stage 3 |
| 15742 Colossus of Ashi | Stock SmartAI, historical Ashi spawn and reference path | One at stage 4 |
| 15414 / 15422 / 15424 | Qiraji Wasp/Tank and Anubisath Conqueror | Excluded: stock `npc_qiraj_war_spawn` assumes quest-8519 flashback targets/timers |
| 15740 / 15741 | Colossi of Zora/Regal | Excluded from this milestone |
| 15818 | Lieutenant General Nokhor | Excluded: legacy SQL modifies its instance-bind flag |

All placements are map 1. The Colossus origin `-6458.704590, 1076.014282,
-2.896275, 4.052591` is the historical Ashi spawn, legacy GUID 311615.
The five supporting positions are drawn from the first five points of the old
Ashi path (roughly `-6497,1022` through `-6704,899`). This produces a compact
front between the hive and its approach. These points need in-game terrain and
visibility review. The reference path is **not** installed: points 6–10 jump
to approximately `-4950,-1220,502`, then jump back, which is unsuitable for a
runtime boss. The Colossus therefore holds the historic emergence point.
The stock `zone_silithus.cpp` Qiraji war spawn script covers entries 15414,
15422 and 15424 and is tied to the Scepter quest scene; no such actor is used.
The existing Scarab Wall crystal remains at its separate proof location.

## Stages and lifecycle

| Progress | 300-second war | 36,000-second war | Active roster |
|---|---:|---:|---|
| Stage 1, [0%,25%) | 0–74 s | 0–8,999 s | 2 Drones |
| Stage 2, [25%,50%) | 75–149 s | 9,000–17,999 s | 3 Drones, 1 Warbringer |
| Stage 3, [50%,75%) | 150–224 s | 18,000–26,999 s | 3 Drones, 2 Warbringers |
| Stage 4, [75%,100%) | 225–299 s | 27,000–35,999 s | 3 Drones, 2 Warbringers, 1 Colossus |

The battlefront is a fixed array of spawn definitions and per-slot runtime GUIDs.
The controller reads the manager's authoritative elapsed/duration snapshot. A
stage change despawns only GUIDs that this controller summoned, clears the old
slots, then creates the current stage directly. This intentionally refreshes
the roster at each stage and never queues missed stages. A startup at 55% or
60% creates stage 3 directly. Repeated world-update reconciliation sees each
slot marked as spawned and cannot duplicate it. Loading grids and retrying a
failed summon occur only for unfilled slots and at most every five seconds.
No creature GUID, wave state, or stage is persisted. Stage and temporary ownership
are local process state; campaign phase and timer remain persistent authority.

When a summoned ordinary creature dies, its slot remains spent for that stage;
the next stage refreshes supporting actors. The module-owned Colossus death is
recorded under campaign ID and war timer origin. A restart in the same war
recovers that row and keeps Ashi absent. A new war origin allows a new Ashi.
See `NAMED_WAR_BOSS_KILLS.md` for the durable death and failure rules.

Leaving TEN_HOUR_WAR for OPEN, READY, WAR_EFFORT or DISABLED despawns only owned
GUIDs and resets the battlefront. OPEN startup leaves it absent. The existing
crystal cleanup, wall, gong, timer, event 22, and collection logic are unchanged.
The boss-kill character table is the only added SQL. Stock template behavior
and loot apply to the selected creatures; no loot or reward change is included.

## Build and live verification

Copy the ZIP contents over the module root while worldserver is stopped and
rebuild with the matching AzerothCore checkout. Before enabling, verify the
deployed world DB (including local overrides):

```sql
SELECT entry, name, AIName, ScriptName, LootId, flags_extra
FROM creature_template WHERE entry IN (15421,15758,15742);
SELECT entry, path_id FROM creature_template_addon
WHERE entry IN (15421,15758,15742);
SELECT id, position_x, position_y, position_z FROM creature
WHERE id IN (15421,15758,15742);
```

The last query is for collision/stock-spawn review; do not delete its rows.
Check that 15742 is Colossus of Ashi and confirm its SmartAI/loot locally.
With `AQWarEffort.TenHourWar.Duration = 300`, use an isolated test campaign:

1. In READY, visit `.go xyz -6497 1022 0.38 1`; no module actors should be
   present. Enter `.aqwareffort phase war` (or ring the stock gong through the
   existing authorized quest). Check stage 1 has exactly two Drones and the
   controller's existing crystal still appears at the Scarab Wall.
2. Wait past 75, 150 and 225 elapsed seconds. Check the roster above at each
   boundary, and confirm only stage 4 has the Colossus. Inspect all six final
   placements from a normal character and check pathing/terrain/aggro.
3. Kill a Drone, then the Colossus. Wait through several reconciliations: neither
   should instantly return in the same stage. A kill in stages 1–3 may return
   on the next stage refresh. The stage-4 boss death must survive restart.
4. Repeatedly run `.aqwareffort status` and observe for duplicate creatures or
   repetitive log lines. There should be at most six module actors at stage 4.
5. For restart restoration, start a fresh 300-second war and stop at about 20%.
   Restart at about 55% elapsed: it must create stage 3 directly. Separately
   restart at about 60%: stage 3, no prior-stage replay. Restart in stage 4:
   the Colossus appears once if it was not defeated in that war.
6. Issue `.aqwareffort phase open`, then repeat separate wars followed by
   `.aqwareffort phase ready`, `effort`, and `disabled`. Each transition must
   remove all owned Ashi actors and the crystal. Restart in OPEN: none return.
   Confirm unrelated stock creatures with the same entry remain untouched.
7. Let a 300-second war expire, including a restart after the expiry while
   offline. It should converge to OPEN with no Ashi actors. Restore production
   duration 36000 and repeat a stage-1 smoke test.
8. Run the module's existing campaign, gong, wall and timer regression suite
   in the deployment build. Apply the named-boss character SQL update on
   existing installs before starting this build.

The standalone stage test is `tests/test_battlefront_stage.cpp`; compile with
`g++ -std=c++20 -I src tests/test_battlefront_stage.cpp -o test_stage` and run it.
It verifies exact quarter boundaries for both durations and direct selection
after downtime. Full AzerothCore compilation and live lifecycle tests must be
run against the deployment checkout and realm; those resources were unavailable
in the packaging workspace.

Before extending the same slot model to Regal and Zora, validate Ashi encounter
load/terrain with real players and Playerbots, and decide whether localized
patrols should replace stationary emergence positions. Keep the spawn sets
independent and use the shared named-boss kill table and identity API.
