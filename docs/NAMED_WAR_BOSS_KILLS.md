# Named Ten Hour War boss kills

## Identity and schema

The reference `mod-war-effort/warevent.sql` identifies Colossus of Zora as
15740, Regal as 15741, and Ashi as 15742, with distinct historical Silithus
spawns. The module's earlier AzerothCore stock audit identifies 15742 as the
stock Ashi template with SmartAI. The upstream AzerothCore death hook and map
summon APIs were inspected for this change. The subsequent Regal milestone verified both Ashi and Regal against the local
AzerothCore checkout; see [its build and test report](HIVE_REGAL_BATTLEFRONT.md).
The deployment database remains remote; confirm local overrides there:

```sql
SELECT entry, name, AIName, ScriptName, LootId
FROM creature_template WHERE entry IN (15740,15741,15742)
ORDER BY entry;
```

`aq_war_effort_boss_kill` lives in the **character database** and has exactly
four columns: `campaign_id` (INT UNSIGNED), `war_started_at` (BIGINT UNSIGNED),
`boss_id` (INT UNSIGNED), and `killed_at` (BIGINT UNSIGNED). The primary key is
`(campaign_id, war_started_at, boss_id)`. `war_started_at` is the same timer
origin used by `WarContentState`: equal `gong_rung_at` and `phase_started_at`
for an accepted gong war, otherwise the current administrative war's
`phase_started_at`. Historical gong timestamps therefore cannot identify a
later admin war. A new war origin gets a fresh boss state; no old rows are
deleted or rewritten.

The existing `base/003_aq_named_war_boss_kills.sql` uses
`CREATE TABLE IF NOT EXISTS` and is rerunnable. Apply it with worldserver stopped
if the table is not already installed. The Regal milestone needs no SQL. It does
not touch campaign, supply or gong rows. The runtime uses an idempotent insert
and reads the exact key back because `DirectExecute` reports no write status.

## Runtime and failure behavior

Only the controller's stage-4 Ashi and Regal summon GUIDs are eligible for recording. The
global unit-death hook checks that GUID, entry, active phase, campaign and war
origin. It does not change the boss's stock AI or loot. Ordinary Drones and
Warbringers retain their existing stage-local death behavior. Regal now has its own battlefront and uses boss_id 15741 independently of Ashi
15742. Zora is recognized by the shared identity API but has no battlefront or
summons. The Regal guide includes the full two-boss live-test matrix.

Before creating either named boss, the controller reads the exact campaign/epoch/boss key.
A recorded kill suppresses it. A failed read is treated as unknown and **does
not spawn the boss**; it retries after five seconds. On an owned death, the
controller immediately marks the in-memory boss state defeated, performs an
idempotent synchronous character-DB insert, then verifies the row by read-back.
If verification fails, it logs an error, keeps that boss suppressed in this process,
and retries the write every five seconds, even if the war ends. It never logs a
failed write as durable. A process crash while the DB is still unavailable can
lose that unconfirmed death; the error message calls out that unavoidable
failure window. Once the DB recovers, the pending write is retained under the
original campaign and epoch, so it cannot affect a newer war. Reconciliation
and restart after a confirmed write keep that boss absent.

## Exact live test

Use a disposable campaign ID and `AQWarEffort.TenHourWar.Duration = 300`.
Ensure the existing character base/003 table is installed, rebuild, and restart worldserver.

1. Check the three stock names with the SQL query above. In READY, query the
   character DB: `SELECT * FROM aq_war_effort_boss_kill WHERE campaign_id = <id>;`.
   Save this baseline; do not delete historical rows.
2. Run `.aqwareffort phase war`. Record `.aqwareffort status` and the DB
   `phase_started_at`, `gong_rung_at`; confirm the active origin is the new
   `phase_started_at` for an admin war. At elapsed 225 seconds, check that one
   Ashi appears alongside the existing stage-4 ordinary roster.
3. Kill the module-owned Ashi. Query the boss table for the exact campaign,
   origin and boss 15742; it must contain **one** row with `killed_at`. Observe
   several reconciliation ticks: no new Ashi. Kill an ordinary actor too;
   no row for its entry should appear. Repeated or duplicate death callbacks
   must not add a second row or change the original `killed_at`.
4. Restart worldserver before the 300-second war expires. Ashi must remain
   absent; ordinary stage-4 actors reconstruct. A later restart in OPEN must
   leave all temporary actors absent. For easier restart timing, repeat this
   step at duration 36000 if needed.
5. While this war is active, kill or spawn an unrelated stock/test creature
   entry 15742 outside this controller's GUID ownership. Its death must not
   create a new boss row. Do not alter permanent stock spawns or loot.
6. Start a fresh admin war with a **different** `phase_started_at`. At stage 4
   Ashi should appear again. Verify the old kill row remains unchanged and
   the new epoch initially has no row. If the two tests would start in the same
   wall-clock second, wait at least two seconds before starting the new war.
7. Test an accepted gong war through the existing authorized quest. Verify its
   equal `gong_rung_at`/`phase_started_at` origin. After it ends, start a later
   admin war; its current `phase_started_at` must be used, not the historical
   gong timestamp. Ashi should be available at stage 4 of that new epoch.
8. In an isolated test realm, temporarily make the character DB unavailable
   *after* Ashi has spawned, then kill it. Verify an error is logged and Ashi
   remains absent for that process. Restore DB access and verify the pending
   row appears under the original epoch. Separately, make DB unavailable
   before stage 4; the boss should stay absent until the kill-state read works.
9. Recheck OPEN, READY, WAR_EFFORT and DISABLED cleanup, the Scarab Wall crystal,
   the gong/timer behavior, and the existing campaign regression suite. No
   named-boss status command is needed for this change.

## Validation performed in packaging workspace

The Regal milestone compiled all module translation units against the actual local
AzerothCore checkout. Shared controller tests, the two-boss restart/death matrix,
existing campaign/contribution/gong/wall/timer regressions and isolated MySQL tests
passed. This is not a full worldserver link or a live realm test. See the
[Regal report](HIVE_REGAL_BATTLEFRONT.md) for exact scope and commands.
