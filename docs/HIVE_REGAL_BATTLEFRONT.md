# Hive'Regal battlefront

The subsequent [Hive'Zora milestone](HIVE_ZORA_BATTLEFRONT.md) adds the third
front through the same data definitions; Ashi and Regal are unchanged. Counts
and scope below describe this earlier two-front milestone.

Hive'Regal and Hive'Ashi now run through one `WarContentController`, using the
same persisted campaign clock and four normalized stages. This milestone adds
six Regal runtime slots, at most six active actors per front. It adds no SQL,
configuration, new boss table, event membership, custom AI, loot or rewards.
The proof crystal and campaign/gong/wall/timer/event-22 code are unchanged.
Hive'Zora and server-wide invasions are not implemented.

## Reference inventory and selection

Inspected source snapshots:

- Module baseline: `cfc699dcebe21272d98fab4c59f873675d0411df`.
- AzerothCore: `06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`, local checkout
  `/home/smithkt/Documents/Codex/2026-09-11/cu/work/azerothcore`.
- Reference mod-war-effort: `69f3f0bcc2095b75c3ed3825df5a26a8d4b79530`,
  `data/sql/db-world/updates/warevent.sql`.

Current core `data/sql/base/db_world/creature_template.sql`, `creature.sql`,
`creature_template_addon.sql`, `smart_scripts.sql`, `game_event.sql` and
`game_event_creature.sql` were inspected. The table below distinguishes stock
Hive'Regal fauna from historical event forces. The deployed realm database was
not accessible; locally customized templates still need the live checks below.

| Entry | Stock name | Finding and decision |
|---|---|---|
| 11730 | Hive'Regal Ambusher | Local fauna; stealth makes it a poor visible escalation choice. Excluded. |
| 11731 | Hive'Regal Burrower | Local fauna with stock SmartAI. Excluded to keep the roster small. |
| 11732 | Hive'Regal Spitfire | Selected twice; level 59–60, SmartAI Corrosive Acid Spit (21047) and Swoop (5708). |
| 11733 | Hive'Regal Slavemaker | Selected once; level 59–60, SmartAI Volatile Infection (3584) and Poison Mind (19469). |
| 11734 | Hive'Regal Hive Lord | Selected once; level 59–61, SmartAI Berserker Charge (19471). |
| 15286 | Xil'xix | Hive'Regal Overlord/quest actor; excluded. |
| 15620 | Hive'Regal Hunter-Killer | Scepter-related elite; excluded. |
| 15741 | Colossus of Regal | Verified in both stock and reference. Selected only in stage 4. Stock SmartAI Colossal Smash (26167), 1000x health modifier, loot 15741. |
| 15758 | Supreme Anubisath Warbringer | Reference groups near Regal; one selected in stage 3. Stock 20x health modifier, no custom script. |
| 15818 | Lieutenant General Nokhor | Present in historical groups; excluded to avoid another named encounter. |
| 180810 | Resonating Crystal Formation (GO) | Historical Regal-area crystal GUID 105004 at -7831.400879,857.148376,-4.281037, orientation 1.477178. No new crystal added. Existing Scarab Wall proof remains unchanged. |

Stock local spawn counts in the inspected base data are 24 Ambushers, 12
Burrowers, 38 Spitfires, 43 Slavemakers and 36 Hive Lords. These remain stock
population, not module-owned actors. The selected Colossus and Warbringer have
no permanent stock spawns in that base data. No relevant stock event membership
was found for these local creature spawns. Event 22 and the stock supply-tier
events (131–190) remain under their existing behavior; no event is started to
produce this front.

Historical `warevent.sql` supplies Colossus GUID 311616, path `157410`
(`@NPC * 10`), and Warbringer group GUIDs 311623–311626 near Regal, alongside
Nokhor 311627. Its global template changes, spawn deletions, paths, pools and
loot changes are **not imported**. The reference does not specify this new
four-stage composition: it supplies exact placement anchors; the current stock
Regal fauna supplies a deliberately modest local roster.

## Exact placements and stage roster

All positions are map 1. Orientations are radians, copied from the reference.
Regal path 157410 contains seven points: unique points 1,2,3,4 followed by
3,2,1. We use its four unique surface-route anchors as stationary emergence
slots. This milestone does not install that path or alter shared template
addons. Actors retain stock combat movement and AI; no patrol is promised.
Stock local positions such as Spitfire GUID 43807 (-7812.12,741.156,-34.6761)
and GUID 43810 (-7791,646.242,-41.2599) corroborate this hive's terrain.

| Slot | Entry | First stage | X | Y | Z | Orientation | Reference source |
|---|---:|---:|---:|---:|---:|---:|---|
| 1 | 11732 | 1 | -7870.958496 | 687.510498 | -27.781849 | 0.293172 | Regal path point 1 |
| 2 | 11732 | 1 | -7791.520996 | 727.871521 | -37.473316 | 0.432190 | Regal path point 2 |
| 3 | 11733 | 2 | -7720.165039 | 719.385498 | -41.306274 | 5.582830 | Regal path point 3 |
| 4 | 11734 | 2 | -7637.914551 | 609.112854 | -51.588173 | 5.230968 | Regal path point 4 |
| 5 | 15758 | 3 | -7831.444336 | 808.078979 | -9.832852 | 4.501119 | Creature GUID 311624 |
| 6 | 15741 | 4 | -7922.958008 | 625.548523 | -29.006325 | 0.844522 | Creature GUID 311616 |

| Stage | Elapsed fraction | Regal roster on entering stage | 300-second war | 36000-second war |
|---|---|---|---|---|
| 1 | [0,25%) | Two Spitfires | 0–74s | 0–8999s |
| 2 | [25,50%) | Above + Slavemaker + Hive Lord | 75–149s | 9000–17999s |
| 3 | [50,75%) | Above + one Warbringer | 150–224s | 18000–26999s |
| 4 | [75,100%) | Above + undefeated Colossus of Regal | 225–299s | 27000–35999s |

These are roster counts, not guaranteed survivors. The existing Ashi behavior
is preserved: a successfully summoned ordinary slot stays spent for that stage
after death or removal. A stage change refreshes that front's entire roster.
Restart restores the current stage's ordinary actors; individual deaths are
not persisted. Stock resident creatures must not be included in module counts.

## Shared lifecycle and named bosses

`AQBattlefrontData.h/.cpp` holds both fronts' entries, coordinates, first-stage
values and boss identity. All six Ashi records preserve their previous numeric
values exactly. `AQWarContent.h/.cpp` has two independent runtime front states
(stage, GUID slots, retry time, log suppression), serviced by the same reconcile
and cleanup methods. A repair within one front cannot remove or rebuild the
other. Both read the same `WarContentState`; at 60% both restore directly to
stage 3. No independent front clocks or saved spawn state exist.

The existing named-boss character table and SQL remain unchanged. Only death of
an owned stage-4 GUID for the active campaign and epoch records a kill. Regal
uses boss_id 15741; Ashi uses 15742. The exact key remains
`(campaign_id, war_started_at, boss_id)`. Duplicate writes preserve the original
kill time. A new epoch permits both bosses without deleting old rows.

Boss reads now have separate retry times and error suppression per boss. A
failed Ashi read therefore cannot delay Regal's first read or vice versa. A
failed read leaves only that boss unspawned pending retry. Pending writes retain
the existing synchronous insert/read-back verification and retry behavior.
As before, a process crash while an owned death's DB write is still unconfirmed
can lose that kill; this milestone does not introduce a second persistence
system. Confirmed deaths survive restart. Errors explicitly identify unconfirmed
persistence; normal durable-kill logs identify boss, campaign and origin.

OPEN, READY, WAR_EFFORT, DISABLED, unavailable management or invalid/expired
timing removes only recorded runtime GUIDs. Removal is hidden immediately and
then queued through core despawn APIs. Startup outside an active war creates
neither front. No permanent creature is saved, replaced, or deleted by entry or
proximity. Logs announce each front's stage change, boss entry and cleanup,
without per-tick chatter. The original proof crystal continues its own unchanged
reconciliation inside the controller.

## Installation and SQL

Overlay the ZIP on the existing module. Regenerate your normal AzerothCore
CMake build and build worldserver so `AQBattlefrontData.cpp` is compiled.
Preserve your normal build options. No configuration or SQL change is required
for an installation already running the proven Ashi named-boss milestone.
The existing character table from
`data/sql/db-characters/base/003_aq_named_war_boss_kills.sql` must already exist.
If installing from scratch, apply that existing rerunnable base file along with
the other documented base files. Do not reset campaign data or boss history.

## Changed files

The ZIP contains only these new or updated module-relative files:

- `CMakeLists.txt`
- `README.md`
- `docs/HIVE_ASHI_BATTLEFRONT.md`
- `docs/HIVE_REGAL_BATTLEFRONT.md`
- `docs/NAMED_WAR_BOSS_KILLS.md`
- `src/AQBattlefrontData.cpp`
- `src/AQBattlefrontData.h`
- `src/AQWarContent.cpp`
- `src/AQWarContent.h`
- `tests/battlefront_core_stubs.h`
- `tests/battlefront_regression.cpp`
- `tests/collection_regression.cpp`
- `tests/manager_test_stubs.h`
- `tests/test_battlefronts.py`
- `tests/test_campaign.py`
- `tests/test_named_war_boss_sql.py`
- `tests/test_regal_stock.py`
- `tests/test_scarab_gong.py`
- `tests/test_scarab_gong_sql.py`
- `tests/war_timer_regression.cpp`

The campaign, gong, wall, timing, collection, existing stage/identity helpers,
configuration and every SQL file are byte-identical to the input module.
Regression harnesses restored from the earlier development package use the
current core result API; they do not alter production behavior.

## Validation performed

All six module translation units compiled against the actual local core checkout
above with `-Wall -Wextra -Werror`, producing a module static archive. This is
an object/archive compile check, **not a full worldserver link or live realm run**.
Core's C++ style checker and whitespace checks passed.

The packaged test suite covers:

- Actual production controller/data with isolated core/map/DB doubles: exact Ashi
  golden roster, both duration scales and stage boundaries, repeated reconciliation,
  independent GUID ownership, ordinary deaths, missing-template retry isolation,
  restart at current stage, both bosses' four death combinations, duplicate/concurrent
  callbacks, wrong GUID/campaign/epoch rejection, new epochs, independent read
  failures, pending-write recovery, all nonwar cleanup phases, OPEN startup and
  unchanged crystal behavior. Existing stage and named-boss rule tests also pass.
- Actual campaign/contribution and timer source through regression doubles, plus
  gong/wall authorization, durable recovery, ceremony timing and no-replay tests.
- Isolated MySQL 8.4 tests of actual gong SQL and the named-boss insert/read SQL:
  rerunnable installation, idempotence, separate boss/epoch/campaign keys,
  unchanged original kill time, preserved contribution/campaign history.
- Read-only stock/reference audit verifies selected names and SmartAI spells and
  all six Regal placements/orientations against the historical SQL.

Run from the module root (Python 3, C++20 compiler; MySQL tests need a local MySQL
installation and never connect to a realm DB):

```sh
python3 tests/test_battlefronts.py
python3 tests/test_campaign.py
python3 tests/test_scarab_gong.py
python3 tests/test_regal_stock.py --core /path/to/azerothcore --reference /path/to/mod-war-effort
python3 tests/test_named_war_boss_sql.py --mysql-root /path/to/mysql/prefix
python3 tests/test_scarab_gong_sql.py --core /path/to/azerothcore --mysql-root /path/to/mysql/prefix
```

## Exact live-test procedure

Use an isolated test campaign/realm. These commands intentionally change its
campaign phase. Do not delete historical boss rows or run the old warevent.sql.

1. Stop worldserver, overlay/build/install the module and set the active config
   `AQWarEffort.TenHourWar.Duration = 300`. Keep the proven campaign ID and normal
   enable settings. Start worldserver. Check deployed stock data:

   ```sql
   SELECT entry,name,AIName,ScriptName,lootid,HealthModifier
   FROM creature_template WHERE entry IN (11732,11733,11734,15741,15742,15758);
   ```

   Confirm the existing character boss table has the documented three-column
   primary key. Record the campaign and boss history before testing:

   ```sql
   SELECT * FROM aq_war_effort_campaign WHERE id = <campaign_id>;
   SELECT * FROM aq_war_effort_boss_kill WHERE campaign_id = <campaign_id>;
   ```

2. Set `.aqwareffort phase ready`, then `.aqwareffort phase war`. Record
   `.aqwareffort status` and `phase_started_at`. Admin war uses this new origin;
   a legitimate gong war uses its equal gong/phase timestamps. At elapsed <75s,
   both fronts should log stage 1 and have two owned actors each. Visit Regal:
   `.go xyz -7870.958496 687.510498 -27.781849 1` and the second slot in the table.
   Visit Ashi: `.go xyz -6497.20 1021.79 0.38 1`. Check the existing proof crystal
   outside the Scarab Wall remains present. Distinguish resident stock creatures
   from the new runtime actors by GUID and these slots; do not count every mob.
3. At 75s, 150s and 225s, verify both fronts advance together to stage 2,3,4:
   4,5,6 owned actors respectively if none are killed. Regal's added entries and
   placements must match the table. At stage 4 visit
   `.go xyz -7922.958008 625.548523 -29.006325 1` for Regal and
   `.go xyz -6458.70 1076.01 -2.90 1` for Ashi. Neither boss may appear earlier.
   Inspect terrain placement, aggro and stock spells with players/Playerbots.
   Stock Colossus tuning is unchanged; this is not a balance reduction.
4. Within a stage, kill an ordinary owned Regal actor and an ordinary owned Ashi
   actor. Wait >10s: neither slot refills, the surviving actors retain their
   GUIDs, and no boss row is added. Repeated status/reconcile ticks must not
   duplicate either roster. At the next stage, roster refresh is expected.
5. In a fresh war, restart near elapsed 180s (60%). Both fronts must restore
   directly at stage 3 (five slots), with no stage-1/2 replay. Count twice >5s
   apart. Ordinary dead actors reconstruct on restart by design.
6. For boss persistence tests, use duration 1200 and restart the realm before
   starting a fresh war; stage 4 begins at 900s, giving a 300s restart window.
   Run separate wars for **neither dead, Ashi only dead, Regal only dead, both
   dead**. Kill only the identified module-owned bosses through combat. Query:

   ```sql
   SELECT campaign_id,war_started_at,boss_id,killed_at
   FROM aq_war_effort_boss_kill
   WHERE campaign_id = <campaign_id> AND war_started_at = <recorded_origin>
     AND boss_id IN (15741,15742) ORDER BY boss_id;
   ```

   Expect exactly the defeated IDs, one row each, and durable-death logs. Wait
   >10s, then restart before expiry: only undefeated bosses may reconstruct.
   Ordinary stage-4 slots restore on both fronts. Historical kill timestamps
   must not change. Do not change duration during a war to accelerate this test.
7. After each test use `.aqwareffort phase open`, wait at least two seconds,
   then `.aqwareffort phase war` to create a distinct epoch. Reissuing `phase war`
   while already in war intentionally does not reset its origin. Verify prior
   kill rows remain and both bosses can appear in the new stage 4.
8. From an active stage use `.aqwareffort phase open`: both fronts and the proof
   crystal disappear; unrelated stock creatures remain. Restart in OPEN:
   neither front/crystal returns. Repeat cleanup with `phase ready`, `phase effort`
   and `phase disabled`. Re-enter war from each and verify a single stage-1 roster
   per front. With duration 300 also let automatic expiry reach OPEN; results
   must match manual OPEN, including expiry while worldserver is stopped.
9. Run the proven legitimate READY → quest 8743 reward path once with an eligible
   Scepter character. Both fronts activate alongside the unchanged gong/wall
   ceremony and use the same persisted timer. Recheck forbidden-phase gong
   rewards, contribution accounting, event-22 phase behavior and wall state.
   No commands or tests here should bypass stock Scepter requirements.
10. Restore production duration 36000 before the next production war. Its stage
    boundaries are 9000,18000,27000s; only timing differs. Preserve all campaign,
    gong, contribution and boss history. Retain the live logs for terrain and
    population review before expanding to another battlefront.
