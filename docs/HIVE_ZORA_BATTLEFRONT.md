# Hive'Zora — third Ten Hour War battlefront

Zora is added through the existing battlefront data. The only production changes
are `BattlefrontCount = 3` and a six-slot Zora definition in
`AQBattlefrontData.h/.cpp`. Ashi and Regal definitions are unchanged, and the
shared controller, boss persistence SQL/API, reconciliation frequency, campaign,
gong, wall, timer, event 22 and proof crystal are unchanged. No SQL, schema,
configuration, custom loot, rewards or core changes are included.

## Inspected references

- Input module: `/home/smithkt/git/mod-aq-war-effort`, commit
  `cf84c7794602f2b2a31e90ae1ceb780fb545a6d9` (clean at start).
- Actual AzerothCore checkout:
  `/home/smithkt/Documents/Codex/2026-09-11/cu/work/azerothcore`, commit
  `06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`.
- Historical reference: `/home/smithkt/git/mod-war-effort`,
  `data/sql/db-world/updates/warevent.sql`.

The current battlefront data, shared controller, stage helper and named-boss
identity/persistence were inspected before implementation. Stock inventory uses
core `data/sql/base/db_world/{creature_template,creature,creature_template_addon,
smart_scripts,game_event,game_event_creature}.sql`. Matching current SQL updates
were also checked: Colossi receive stock immunity updates in
`2026_03_25_03.sql`; ordinary Zora world-loot references are updated in
`2026_02_20_03.sql`. These remain stock-owned. The live realm database is remote
and was not queried here; verify any local overrides with the live procedure.

| Entry | Stock identity | Stock behavior / selection |
|---|---|---|
| 11725 | Hive'Zora Waywatcher | Level 58–59, faction 310, no custom AI/script; selected once for the stage-2 ground roster. |
| 11726 | Hive'Zora Tunneler | Level 58–59, faction 311, SmartAI spell 14120; excluded to keep this surface scene small. |
| 11727 | Hive'Zora Wasp | Level 58–59, faction 310; SmartAI Poison (19448). Two selected for the opening stage. |
| 11728 | Hive'Zora Reaver | Level 59–60, faction 310; SmartAI Knockdown (16790) and Cleave (40504). One selected at stage 2. |
| 11729 | Hive'Zora Hive Sister | Level 59–60, faction 310; SmartAI Toxic Spit (7951). One selected at stage 3. |
| **15740** | **Colossus of Zora** | Verified stock and reference identity. Level 63, faction 370, SmartAI Colossal Smash (26167), 1000x stock health modifier. Stage 4 only. |
| 15758 | Supreme Anubisath Warbringer | Historical nearby Silithus event group; excluded from Zora to distinguish its native Silithid roster from Ashi/Regal. |
| 15818 | Lieutenant General Nokhor | Historical nearby group leader; excluded, no additional named encounter. |
| 15319 / 15320 | Hive'Zara Collector / Soldier | AQ raid creatures, distinct from outdoor Hive'Zora; not selected. |

Zora 15740 currently has **lootid 0** in the inspected stock template. This
milestone leaves it unchanged, including all stock AI, immunities, scaling and
reward behavior. It does not import the reference custom Colossus loot.

The stock base contains 21 Waywatcher, 9 Tunneler, 107 Wasp, 29 Reaver and
29 Hive Sister spawns, and no permanent 15740/15758/15818 spawns. No event
membership was found for those resident Zora creatures. Resident stock actors
remain present and are never included in module ownership or cleanup. Example
stock Reaver GUID 43641 at (-7348.12,1693.22,-36.6768), orientation 4.35316,
and Hive Sister 43698 at (-7439.62,1613.39,-43.439), orientation 3.89742,
corroborate the selected surface-route area. Some other stock fauna are deep
underground; their coordinates were not copied for this scene.

Stock event 22 is the AQ War Effort; events 131–190 are supply presentation
tiers. No stock event is activated or changed to create the Zora front.
Historical warevent.sql is reference material only, never an installation step.

## Historical positions and chosen roster

The reference Colossus spawn is legacy GUID **311617**, entry **15740**, map 1,
(-7461.777832,1611.004272,-48.327751), orientation 0.616755. Its historical
MovementType is 2, with addon path **157400** (`15740 * 10`). That path has eleven
points: unique points 1–6, then points 5,4,3,2,1 in reverse. It is a local route.
We use exact points 1–5 as stationary runtime emergence slots, and the exact
Colossus spawn. No global path/addon is installed; stock combat movement remains.

All following coordinates are map 1; orientations are radians:

| First stage | Actor | Entry | X | Y | Z | Orientation | Source |
|---|---|---:|---:|---:|---:|---:|---|
| 1 | Wasp | 11727 | -7418.407227 | 1649.863770 | -32.103611 | 1.029089 | Path 157400 point 1 |
| 1 | Wasp | 11727 | -7409.972656 | 1705.264404 | -36.461433 | 1.437496 | Path point 2 |
| 2 | Waywatcher | 11725 | -7352.450684 | 1710.901733 | -38.267399 | 5.460305 | Path point 3 |
| 2 | Reaver | 11728 | -7329.275391 | 1640.641968 | -32.322731 | 5.153214 | Path point 4 |
| 3 | Hive Sister | 11729 | -7299.695801 | 1599.568481 | -30.213583 | 5.754819 | Path point 5 |
| 4 | Colossus of Zora | 15740 | -7461.777832 | 1611.004272 | -48.327751 | 0.616755 | Spawn 311617 |

Unused path point 6 is (-7250.575195,1534.470459,-10.517513), orientation 5.041683.
Points 7–11 repeat 5–1 including their orientations. The new composition is a
small module design using these historical anchors, not a claim that the legacy
SQL had the same staged native-fauna roster.

Other nearby historical event data inspected but not installed:

| Legacy GUID | Entry | X | Y | Z | Orientation |
|---|---:|---:|---:|---:|---:|
| 311618 | 15758 | -7623.261719 | 1416.035767 | 4.126772 | 4.945646 |
| 311619 | 15758 | -7659.168457 | 1392.619751 | 3.995544 | 3.687438 |
| 311620 | 15758 | -7688.503418 | 1428.886963 | 3.855407 | 2.550966 |
| 311621 | 15758 | -7652.402344 | 1464.758667 | 4.526736 | 0.600033 |
| 311622 | 15818 | -7644.985840 | 1422.093628 | 3.326948 | 5.378395 |
| 105003 (GO) | 180810 | -7648.079590 | 1426.084717 | 2.876715 | 3.538846 |

No reference pools, world deletions, crystal spawns, template changes or rewards
are imported. The original Scarab Wall proof crystal is untouched.

## Stages, population and ownership

| Stage | Global elapsed fraction | Zora roster | Per front | Three-front total | 300s start | 36000s start |
|---|---|---|---:|---:|---:|---:|
| 1 | [0,25%) | Two Wasps | 2 | 6 | 0s | 0s |
| 2 | [25,50%) | Above + Waywatcher + Reaver | 4 | 12 | 75s | 9000s |
| 3 | [50,75%) | Above + Hive Sister | 5 | 15 | 150s | 18000s |
| 4 | [75,100%) | Above + undefeated Colossus | 6 | **18** | 225s | 27000s |

The maximum is **18 active module-owned battlefront creatures**: six in each
hive, including three Colossi. The proof crystal is one additional gameobject,
not a creature. Dead actors and defeated bosses reduce these counts. Resident
stock mobs and Playerbots are separate. Core may briefly retain hidden objects
queued for deletion during a stage change; they are not a second active roster.
Selected stock SmartAI does not introduce summoned creature waves. No new
reconcile timer, poll rate or DB query frequency is added. The existing per-front
five-second retry pacing and per-boss cached reads service Zora as well.

`WarContentState` supplies the same global elapsed/duration to all three fronts.
At 60%, all restore directly to stage 3; at 80%, all restore to stage 4. Each
front keeps its own GUID slots, spent flags, stage and retry state. A successful
ordinary spawn remains spent for that stage after death/removal. Stage changes
refresh that front's roster; restart reconstructs ordinary slots at the current
stage. No missed-stage replay or individual ordinary-death persistence exists.

Existing boss support already includes 15740. Only the owned stage-4 runtime
GUID in the current campaign/epoch can record a death. It immediately marks the
boss defeated in memory, then uses the existing idempotent insert/read-back
against `aq_war_effort_boss_kill`. The key is still
`(campaign_id, war_started_at, boss_id)`. All eight combinations of three boss
states are independent. New epochs allow another life while preserving history.
Failed reads suppress only the affected boss until retry. Existing pending-write
recovery is unchanged; as already documented, a crash during an unconfirmed
write while the DB is unavailable can lose that unconfirmed death. Confirmed
rows survive restart. No in-memory-only replacement is introduced.

OPEN, READY, WAR_EFFORT, DISABLED, unavailable management and invalid/expired
war timing clean up all recorded runtime GUIDs. Startup in OPEN creates none.
Stock actors are never deleted by entry or proximity. Existing front-specific
activation/stage/cleanup logs automatically name Hive'Zora, and named-boss logs
identify boss 15740, campaign and origin without per-tick chatter.

## Changes and installation

Overlay the ZIP on the current module, regenerate your existing CMake build and
build/install worldserver using your normal options. No SQL/config changes are
needed: the existing named-boss table already supports 15740. Do not reset
campaign or boss history, and do not apply warevent.sql.

Changed production files:

- `src/AQBattlefrontData.h`: front count from two to three.
- `src/AQBattlefrontData.cpp`: Zora constants, six placements and definition.

Changed tests/documentation:

- `tests/battlefront_regression.cpp`: retains Ashi/Regal coverage and adds all
  three fronts, eight defeat combinations, Zora failure/recovery and observer tests.
- `tests/test_battlefronts.py`: updates the coverage description.
- `tests/test_named_war_boss_sql.py`: exact production SQL tests now include 15740
  and the eight three-boss combinations.
- `tests/test_regal_stock.py`: bounds the roster parser at the array terminator,
  so adding the following Zora array does not incorrectly enlarge Regal's audit.
- `tests/test_zora_stock.py`: new stock identity, AI and exact-placement audit.
- `README.md`, `docs/NAMED_WAR_BOSS_KILLS.md`,
  `docs/HIVE_REGAL_BATTLEFRONT.md`: current milestone links/ownership notes.
- `docs/HIVE_ZORA_BATTLEFRONT.md`: this inventory, report and live-test guide.

No other production files change. Original source is untouched; no commits made.

## Compile and regression results

All six translation units compiled against the actual local AzerothCore checkout
listed above with `-Wall -Wextra -Werror`, producing a module static archive.
This includes unchanged database calls using **`QueryResult->Fetch()`** and
`Field::Get<uint64>()` against real core headers. No substitute DB API was used
for compilation. This is not a full worldserver link or live realm execution.

Passing checks:

- Production controller and data exercised with isolated core/map/DB doubles:
  exact Ashi and Regal golden data; Zora composition; both duration scales;
  stage boundaries; maximum 18 actors; disjoint GUIDs; 100 repeated reconciliations
  per stage; ordinary deaths isolated per front; direct-stage restart; one-front
  cleanup/repair isolation; all eight boss-defeat/restart combinations; wrong
  GUID/campaign/epoch rejection; duplicate and concurrent callbacks; independent
  boss reads; failed-write recovery; new epochs; all nonwar cleanup; OPEN startup;
  actual unit-death observer; unchanged proof crystal.
- Existing stage and named-boss rule tests; actual campaign/contribution, timer,
  gong and wall regression suites.
- Isolated MySQL 8.4 tests of the exact named-boss SQL, all eight boss combinations,
  idempotent timestamps, campaign/epoch isolation and rerunnable existing schema;
  existing gong installation/acceptance/recovery SQL tests also pass.
- Read-only Regal and Zora reference audits; core C++ style and whitespace checks.

Run from the module root:

```sh
python3 tests/test_battlefronts.py
python3 tests/test_campaign.py
python3 tests/test_scarab_gong.py
python3 tests/test_regal_stock.py --core /path/to/azerothcore --reference /path/to/mod-war-effort
python3 tests/test_zora_stock.py --core /path/to/azerothcore --reference /path/to/mod-war-effort
python3 tests/test_named_war_boss_sql.py --mysql-root /path/to/mysql/prefix
python3 tests/test_scarab_gong_sql.py --core /path/to/azerothcore --mysql-root /path/to/mysql/prefix
```

MySQL tests initialize disposable private data directories with networking off;
they do not connect to the realm database. Visual placement, navigation, combat
and deployment-specific template overrides still require live testing.

## Exact live-test procedure

Use a test realm/campaign. Administrative phase commands intentionally change its
state; retain all historical rows. Keep the proven enable/campaign settings.

1. Stop worldserver, overlay/build/install, set active config
   `AQWarEffort.TenHourWar.Duration = 300`, and restart. Verify deployed world data:

   ```sql
   SELECT entry,name,AIName,ScriptName,HealthModifier,lootid
   FROM creature_template WHERE entry IN (11725,11727,11728,11729,15740);
   ```

   Save the initial character campaign and boss rows:

   ```sql
   SELECT * FROM aq_war_effort_campaign WHERE id = <campaign_id>;
   SELECT * FROM aq_war_effort_boss_kill WHERE campaign_id = <campaign_id>;
   ```

2. Run `.aqwareffort phase ready`, then `.aqwareffort phase war`. Record
   `.aqwareffort status` and the new `phase_started_at` (the admin war origin).
   At elapsed <75 seconds, Zora must have two owned Wasps and each peer two actors.
   Visit `.go xyz -7418.407227 1649.863770 -32.103611 1` and the second Wasp
   coordinate above. Identify temporary actors by their runtime GUIDs/positions;
   stock residents must not be counted or deleted.
3. At 75,150,225 seconds, inspect the listed emergence positions: Zora grows to
   4,5,6 actors if none are killed; the three-front totals are 12,15,18. At stage 4
   use `.go xyz -7461.777832 1611.004272 -48.327751 1` for the Colossus. It must
   not exist earlier. Compare Ashi at `-6497.20 1021.79 0.38` and Regal at
   `-7870.958496 687.510498 -27.781849` on map 1. All stages must agree.
4. Observe several reconcile intervals (>10 seconds): no duplicate GUIDs/actors
   or repeated activation spam. Kill an ordinary owned actor in each front within
   a stage: none refills, surviving peers keep their GUIDs, and no boss record is
   written. At the next boundary the ordinary roster refresh is expected.
5. Start a fresh war and restart near 180 seconds (60%). All three must restore
   directly to stage 3, five ordinary actors each, with no stages 1/2 replay.
   Ordinary deaths are deliberately forgotten across restart. Inspect terrain,
   line of sight, stock poison/cleave/knockdown and Playerbot navigation around
   all five Zora points. The Colossus retains stock tuning.
6. For boss/restart tests, set duration 1200 while worldserver is stopped before
   a fresh war. Stage 4 begins at 900 seconds, leaving 300 seconds for the test.
   Run separate epochs for all eight combinations in the table below. Kill the
   identified owned bosses through combat, wait for durable-death logs, then
   query and restart before expiry. Only undefeated bosses may restore; the
   five ordinary actors in each front reconstruct. Verify no duplicate kill
   rows or changed original kill timestamps.

   | Defeated | Expected boss IDs in this epoch |
   |---|---|
   | None | None |
   | Ashi | 15742 |
   | Regal | 15741 |
   | Zora | 15740 |
   | Ashi + Regal | 15742,15741 |
   | Ashi + Zora | 15742,15740 |
   | Regal + Zora | 15741,15740 |
   | All three | 15742,15741,15740 |

   ```sql
   SELECT campaign_id,war_started_at,boss_id,killed_at
   FROM aq_war_effort_boss_kill
   WHERE campaign_id = <campaign_id> AND war_started_at = <recorded_origin>
     AND boss_id IN (15740,15741,15742) ORDER BY boss_id;
   ```

7. Between epochs run `.aqwareffort phase open`, wait at least two seconds, then
   `.aqwareffort phase war`. Confirm a different origin. Reissuing `phase war`
   while already in war intentionally does not reset it. All three bosses may
   return in a later epoch's stage 4; previous rows must remain unchanged.
8. From active stage 4 issue `.aqwareffort phase open`: all owned hive actors and
   the unchanged proof crystal disappear; stock fauna remain. Restart in OPEN:
   none returns. Repeat cleanup with `phase ready`, `phase effort`, and
   `phase disabled`. Re-enter war from each and check a single stage-1 roster.
   Also let a 300-second war expire normally and while offline: OPEN cleanup
   must be identical. Do not change duration mid-war to accelerate a test.
9. Recheck the legitimate READY → quest 8743 gong path with an eligible character,
   existing wall ceremony/no-replay behavior, event-22 phase control and normal
   contribution turn-ins. The three fronts must share the accepted gong origin
   (`gong_rung_at = phase_started_at`) and timer. No stock Scepter requirement
   or forbidden-phase authorization changes are expected.
10. Restore production duration 36000 before the next production war. Boundaries
    become 9000/18000/27000 seconds with the same rosters. No database resets.
    Record live terrain/combat results; this package does not claim a live run.
