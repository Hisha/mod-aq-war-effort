# Ten Hour War content controller: one-crystal proof

This milestone adds lifecycle infrastructure and exactly one temporary visible
formation near the Scarab Wall. It adds no combat, invasion waves, bosses, rewards,
battle events, quest changes or additional wall owner. No SQL is required.

## Inventory and selection (inspected before implementation)

References inspected:

- AzerothCore `06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`: stock
  `gameobject_template`, `gameobject_template_addon`, `gameobject`,
  `creature_template`, `creature_template_model`, current world updates,
  `zone_silithus.cpp`, and core object/map lifecycle code.
- Original mod-war-effort `69f3f0bcc2095b75c3ed3825df5a26a8d4b79530`:
  `data/sql/db-world/updates/warevent.sql` and `src/mod_aq_war_effort_scripts.cpp`.
- Local 3.3.5a `GameObjectDisplayInfo.dbc` from `enUS/patch-enUS-3.MPQ`, and
  model asset from `common-2.MPQ`, read without changing client files.

| Stock entry | Name / existing use | Decision |
|---|---|---|
| GO 180810 | Resonating Crystal Formation; reference places six in Silithus/Darkshore | **Use one runtime copy** |
| GO 180811 | Resonating Crystal Formation Glow | Not spawned |
| 15758 | Supreme Anubisath Warbringer; reference Silithus groups | Defer combat groups |
| 15810 | Eroded Anubisath Warbringer; reference Darkshore groups | Defer invasions |
| 15813 | Qiraji Officer Zod | Defer named bosses/pools |
| 15818 | Lieutenant General Nokhor; stock instance-bind flag | Defer boss/template changes |
| 15740 / 15741 / 15742 | Colossus of Zora / Regal / Ashi; stock SmartAI, reference paths/loot | Defer all Colossi |
| 15693 / 15341 | Jonathan the Revelator / General Rajaxx | Defer presentation and raid AI |
| 15414 / 15422 / 15424 | Qiraji Wasp / Qiraji Tank / Anubisath Conqueror | Stock `npc_qiraj_war_spawn` is quest-8519 flashback AI; do not reuse its spell/target logic for this proof |
| 15423 | Kaldorei Infantry, same flashback script | Defer battle actors |
| 15421 | Qiraji Drone, stock unbound template | Not needed for a visual lifecycle proof |
| 15797 / 15798 / 15799 | Colossus Researchers Sophia / Nestor / Eazel | Defer post-battle systems |

180810 is stock type 0 (door), display 6573, scale 1, without AIName or ScriptName.
Display 6573 resolves to
`World\Kalimdor\Silithus\PassiveDoodads\Crystals\FloatingRedCrystalBroken01.mdx`.
The client model is present; DBC bounds extend roughly 10 units above its origin.
The reference explicitly sets the crystal's NOT_SELECTABLE flag. We apply this
only to our runtime object, never the shared template. Core `GameObject::Use`
and report-use handling reject this flag. Server-side collision is disabled on
our copy. The stock client model does contain collision geometry; actual client
movement around it must be checked live. No client model or animation is changed
merely to force collision behavior.

The stock template has Data0=0, Data1=10, Data2=180811. Because its type is DOOR,
Data2 is interpreted as autoCloseTime, **not** a linked glow object. Core
`GetLinkedGameObjectEntry()` returns zero for doors. We do not invent a glow,
interaction or crystal reward mechanic from this ambiguous historical data.
The inspected stock `gameobject` base table has no persistent 180810 spawns.
Runtime validation rejects incompatible type/display/script/AI/link overrides.

The nearest reference crystal (legacy GUID 105003) is at
`-7648.079590, 1426.084717, 2.876715`, orientation 3.538846, well away from the dais.
Other reference crystals are near Silithus battlefronts and in Darkshore. None of
those six placements is imported. The proof deliberately relocates **one** crystal
onto stock quest-8519 `SpawnLocation[37]`, a verified Anubisath ground position:

```text
Map 1: -8088.0, 1530.0, 2.61; orientation 0
Quaternion: 0, 0, 0, 1 (identity for orientation zero)
```

This is a new module proof placement on the outside of the wall, about 42 units
from the wall origin. It is not claimed as a historical crystal spawn. No quest
8519 placement/script is modified. No reference SQL GUID, pool, waypoint, loot,
questgiver association or shared template update is imported.

## Lifecycle and ownership

`AQWarContent.h/.cpp` contain the separate `WarContentController` and
`WarContentState`. The manager exposes a locked, coherent read-only snapshot:

- campaign ID, persisted phase, enabled/available management;
- current war timer origin using the existing equal-gong/phase-start semantics;
- current configured duration, elapsed wall-clock seconds, remaining seconds;
- timing validity and `Progress()` clamped to 0..1.

Later stages can test progress ranges such as `[0.25, 0.5)` using this snapshot:
a 300-second test and a 36000-second production war select the same stages at
the same elapsed fraction. This proof occupies the whole active interval `[0,1)`;
no independent stage or timer is persisted.

The existing world script invokes reconciliation:

1. After manager initialization, which includes gong recovery and expired-war
   convergence. Expired startup therefore creates **no transient proof**.
2. At each world-update boundary, after map/session workers have finished, then
   after gong processing and timer reconciliation. This covers gong acceptance,
   administrative phase commands, natural expiry, config reload and failures.

Map mutations never run from a quest-reward/map-worker callback. Phase commands
and reload take effect on the scene by the end of that world tick; no new command
is necessary. Same-phase commands/repeated updates do not duplicate the scene.

During managed, available, valid, nonexpired TEN_HOUR_WAR, create a runtime GO
through `Map::SummonGameObject`. There is no database spawn, GUID reservation,
SaveToDB, event membership, respawn record or independently persisted lifetime.
Store only the returned runtime ObjectGuid. Resolve it through map 1 each time;
never retain a GameObject pointer or find/delete by entry or proximity. Retain
ownership across repeated initialization in the same process. Repair a missing
runtime object if the war still requires it; a real restart starts with no old
runtime objects and reconstructs the same one-object scene.

OPEN, READY, WAR_EFFORT, DISABLED, unavailable/disabled management or invalid
war timing deactivate the proof. Cleanup queues core deletion, immediately hides
the owned object and disables its collision/activity while deletion is pending.
It does not load a grid merely to clean it. Unrelated same-entry objects are left
alone. Campaign ID/origin changes also release old ownership before rebuilding.

There are no content DB writes. Existing persistence, timer, gong, wall ceremony,
contributions and event 22 code paths are unchanged. Invalid duration still has
its established campaign behavior (automatic expiration disabled); the new scene
fails closed until timing is valid again. Future timestamps use the timer's same
zero-elapsed clamp. Missing origins produce no scene.

Activation/restoration logs include phase timing/progress. Deactivation and
startup-absent messages identify cleanup/restoration outcomes. Failed creation
retries at most every five seconds using a steady clock solely for retry pacing;
one module error is logged per continuous failure episode. Successful steady-state
updates log nothing and create nothing. A missing template never changes campaign
progression or opens another authorization route.

## Installation and live tests

1. Apply the changed-files ZIP over the module root with worldserver stopped.
   Preserve your active configuration and client patch. Rebuild/install with your
   normal AzerothCore CMake configuration. **Apply no SQL for this milestone.**
2. Use an isolated test campaign/realm. Set the existing
   `AQWarEffort.TenHourWar.Duration = 300` in the active config and restart.
   Check `.aqwareffort status`; fix any content-template error in the deployment
   rather than importing the old warevent.sql. Existing templates should suffice.
3. Visit the proof point with `.go xyz -8088 1530 2.61 1`, then step back to view it.
   In READY, verify no new crystal. With an eligible test character, reward stock
   8743 at the controlled gong. Confirm unchanged ceremony and campaign transition,
   plus exactly one red crystal outside the wall. Its activation log should show
   current origin/duration/remaining. It must not be clickable. Check movement around/through the formation and
   report any client-side blocking despite server collision being disabled.
4. While the war is active, leave/return to the area and wait through many updates.
   Confirm one crystal, no accumulating copies, no repeated activation log spam.
   Restart before expiry: one restored crystal, `restored at startup` log, correct
   remaining time and no wall ceremony replay.
5. From an active war issue `.aqwareffort phase open`. The crystal should disappear
   by the end of the world tick, with one deactivation log; wall remains open,
   event 22 remains inactive, gong still rejects another reward. Restart in OPEN:
   crystal absent and startup-absent log. Inspect from a normal non-GM character
   too; administrator visibility must not be mistaken for normal phase visibility.
6. Issue `.aqwareffort phase war` directly from OPEN: one crystal appears without
   a gong reward. Record unchanged historical gong_rung_at. Repeat `phase war`:
   no duplicate crystal and no timer reset. Repeat war → ready, war → effort and
   war → disabled. Each removes the crystal; existing wall/event-22 behavior must
   follow its own phase rules. Do not use `.gobject add` to create this scene.
7. Start a fresh admin war and let the 300 seconds expire naturally. Confirm OPEN
   and crystal removal. Start another, stop before its deadline, remain offline
   past it, then restart: immediate OPEN and **no crystal activation** in between.
8. Verify shorter-duration reload against the original start also removes content
   when it expires the campaign. Invalid duration 0 plus `.reload config` removes
   only the proof and disables automatic expiry as documented by the timer; fix
   the duration and reload to reconcile from the original origin. No timer reset.
9. Compare read-only character DB snapshots before and after these runs:
   `aq_war_effort`, `aq_war_effort_campaign`, `aq_war_effort_gong` for your campaign
   ID. Only the existing phase/timestamp transitions should differ; supplies and
   accepted gong history stay unchanged. World DB
   `SELECT * FROM gameobject WHERE id = 180810;` should gain **no row** from the proof.
   Pre-existing legacy crystals are outside this controller's ownership.
10. Restore duration 36000 and your intended campaign phase. Confirm the same
    activation/cleanup behavior; no Ten Hour War battle content is added yet.

The deployed realm database is remote and was not accessible here. Its local
customizations and live visibility/collision must be confirmed with these tests.

## Automated validation

From the module root:

```sh
python3 tests/test_campaign.py
python3 tests/test_scarab_gong.py
python3 tests/test_scarab_gong_sql.py --core /path/to/azerothcore --mysql-root /path/to/mysql-prefix
python3 tests/test_war_content_stock.py --core /path/to/azerothcore --reference /path/to/mod-war-effort
```

The campaign harness compiles actual Manager/controller methods with a controlled
clock and isolated core/DB/map doubles. It checks READY → war, admin war, active
restart and same-process reinitialization, 1000 reconciliations without duplicates,
DB writes or info-log spam, OPEN and every other non-war cleanup, OPEN restart,
offline expiry, equal progress at 300/36000 seconds, current/stale gong origins,
invalid/disabled/unavailable states, unrelated-object preservation, deferred
removal, missing-object repair, template mismatch, and bounded failed-spawn retries.
Existing timer/collection regressions remain included. The actual gong/wall source
harness and isolated MySQL SQL tests cover the established authorization/recovery
boundaries. These tests complement, and do not substitute for, the live procedure.
