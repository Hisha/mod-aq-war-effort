# Scarab Gong and opening ceremony

This milestone adds one authorized questgiver gong and the live wall-opening
presentation. It does not implement a ten-hour timer, automatic OPEN transition,
battle content, Jonathan/Rajaxx, area triggers, barriers, or quest 8519 changes.

## Installation and ownership

Stop worldserver, back up both databases, deploy the changed/new module files,
and the existing gong schema must already be installed. This follow-up requires no
SQL migration or reapplication on a working installation. Fresh installs retain
these existing convergent base files:

- Characters: `data/sql/db-characters/base/002_aq_scarab_gong.sql`.
- World: `data/sql/db-world/base/003_aq_scarab_gong.sql`.

Regenerate the normal core build to discover `AQScarabGong.cpp`, rebuild and
restart. Keep the existing material migration and wall SQL installed. These
scripts may be reapplied; they do not reset supplies, campaigns or gong attempts.

The world migration replaces only the checked decorative spawn 49451 with owned
spawn **9100717**, entry **180717**, script `go_aq_war_effort_gong`. Both templates
remain unchanged; 180718 remains type 5. The owned gong's AddWorld hook removes
INTERACT_COND from that instance, leaving the stock template addon unchanged.
The placement comes from stock spawn 49451:

```
map 1; x -8069.05; y 1641.72; z 27.03; orientation -1.53589
rotation 0, 0, -0.694658, 0.71934
```

The only added quest relations are `(180717, 8743)` in gameobject_queststarter and
gameobject_questender. SQL rejects GUID conflicts, another 180717 spawn, alternate
8743 givers, event/pool ownership, customized decorative placement, and legacy
180718 conversion. It rolls back rather than guessing how to remove those changes.
Resolve reported legacy conflicts against a backup before retrying. Do not run
old mod-war-effort alongside this module. Deleting a row by entry is not a repair.

AzerothCore alone checks eligibility and interaction, consumes the Scepter, and
awards items, reputation and quest/achievement history. This module does not call
RewardQuest, consume/grant items, or change quest templates. Stock 8743 requires
8742 and item 21175. The current deployment has verified normal completion of
8742 and natural availability of 8743 with the required Brood reputation. This
update leaves that working progression and the waiting test character untouched.

## Authorization and successful reward

The pre-dispatch `ServerScript::CanPacketReceive` hook handles only
CMSG_QUESTGIVER_CHOOSE_REWARD for 8743. It reads a packet copy and preserves the
handler's cursor. It rejects other givers of this quest, including legacy gongs.
All unrelated quest requests continue unchanged.

A reward attempt requires enabled, available campaign management, a healthy gong
journal, the exact owned gong, READY, and no pending attempt. WAR_EFFORT,
TEN_HOUR_WAR, OPEN, DISABLED and config-disabled states reject it, including stale
reward dialogs and direct packets. Gossip gives an early explanation, but the
packet guard is the authorization boundary. Standard stock reward handling runs
after that boundary; missing prerequisites/items, range and inventory failures
are still stock decisions.

A shared recursive manager mutex serializes PROCESS_INPLACE packets across map
workers and campaign mutations. A pending attempt blocks other ringing attempts
and admin phase overrides until it is resolved. One running worldserver must own
a campaign ID/database; multi-world shared campaign writers are not supported.
Do not change campaign/journal rows externally while worldserver is running.

`AllGameObjectScript::CanGameObjectQuestReward` is the successful observer. Despite
its name, core calls it after RewardQuest. It requires the owned gong and 8743,
checks rewarded status, and returns false to preserve normal stock callbacks.
The generic PlayerScript reward hook does not initiate opening.

## Durable recovery protocol

1. Before allowing stock processing, synchronously persist and read back an
   intent containing campaign ID, player GUID, the original READY phase timestamp,
   preparation time and accepted=0. A pre-existing rewarded-quest row cannot be
   used to authorize a new attempt.
2. Let stock perform the reward. Its character save is asynchronous. The specific
   post-reward observer records live success, but presentation does not start yet.
3. At the end of World::Update, after session processing and completion of map
   workers, examine the intent and stock character_queststatus_rewarded row.
   If stock rejected the attempt, clear the unaccepted intent after verifying the
   deletion. If live success was observed but its save is not durable yet, retain
   the reservation and wait. Never time out and discard that successful evidence.
4. Once the reward is durable, the existing Manager::Persist boundary performs
   one conditional multi-table UPDATE, atomically setting campaign phase=3 and
   phase_started_at/gong_rung_at to the same current server timestamp
   and journal accepted=1. It requires READY and the intent's original phase epoch.
   It leaves contributions, opened_at and other data untouched. Read back both
   campaign and journal before publishing state or beginning presentation.
5. Keep the accepted journal row as durable evidence. Repeated callbacks/updates
   cannot accept it twice or rewrite its timestamp. An intentional later admin
   reset to READY permits a new eligible player, replacing that accepted journal.

Startup clears an interrupted unaccepted intent only when no durable stock reward
exists. If a durable reward exists in the original READY epoch, startup accepts it
and opens the wall immediately without presentation. If the phase was already
persisted as TEN_HOUR_WAR, ordinary wall reconciliation opens it immediately.
No ceremony stage is resumed or replayed after restart.

Database errors fail closed for gong use. An ambiguous write retains a pending
reservation, preventing phase overrides. Repair the database and restart to retry
recovery. If stock's asynchronous save never becomes durable, the live reservation
intentionally stays blocked; inspect core DB errors. Do not delete a pending
journal just to unblock players. On restart, the stock persisted reward determines
whether the turn-in succeeded. A phase-epoch mismatch requires operator review.
Direct GM reward commands bypass normal quest processing and are not supported
as ringing tests; do not use them during a pending recovery attempt.

## Presentation and timing

`AQScarabWall.cpp` remains the sole wall owner. Only these dedicated spawn IDs are
changed: roots 9100147, runes 9100148, gate 9100146. Entry matching alone is never
used to select a wall. Quest 8519's summoned walls remain untouched.

The existing manager owns an EventMap, advanced by WorldScript::OnUpdate after
session/map processing. It schedules runes at 4000 ms, gate at 8000 ms, and final
absence at 12000 ms. No thread sleeps and no delayed callback retains an object
pointer. Admin changes/startup reset the event map. If any required part is absent
at the start, the cosmetic sequence is skipped; AddWorld/phase synchronization
still applies the persisted open state when the objects become available.

All timing is centralized in `PartAnimationMs`, `CeremonyDurationMs` and the
`WallParts` table in AQScarabWall.cpp:

| Elapsed from durable live acceptance | Presentation |
|---|---|
| 0 ms | Roots GO_STATE_ACTIVE, sound 7114; runes and gate still closed. |
| 4000 ms | Hide roots; runes GO_STATE_ACTIVE, sound 7116; gate still closed. |
| 8000 ms | Hide runes; gate GO_STATE_ACTIVE, sound 7115. |
| 12000 ms | Hide gate; all three parts absent, no collision. |

Each part loses collision as its opening stage begins. Following parts continue
to block until their stage. Phase is TEN_HOUR_WAR throughout presentation and
remains there afterward. No timer advances it to OPEN. Sounds use PlayDistanceSound
on the corresponding object, not realm-wide playback. Large server stalls may
cross several stage boundaries in one update; the controller converges to elapsed
time rather than prolonging a stalled ceremony.

Four seconds per part is provisional. Syntax/runtime tests cannot establish
client animation duration, acoustic range or duplicate client-provided sounds.
Validate these three sounds and animations on the deployed 3.3.5a client before
considering timing final. Administrative phase commands cancel any live ceremony
and reconcile immediately; they do not play sounds or set gong_rung_at.

## Precise live-test procedure

Use a disposable test realm/database and ordinary, non-GM players. Keep backups
for repeat tests because stock 8743 is nonrepeatable and consumes its quest item.
Prepare two legitimately eligible characters with 8742 rewarded, the stock
reputation/level requirements satisfied, and 21175 in carried inventory. If the
baseline cannot supply that character through its chain, explicitly label any
GM-prepared fixture as a fixture, then perform the actual turn-in through the gong
as a normal player. Never use `.quest reward` as proof of a gong opening.

1. Apply both new SQL files twice while stopped. Verify world query results:
   ```sql
   SELECT guid,id,map,position_x,position_y,position_z,orientation,
          rotation0,rotation1,rotation2,rotation3,ScriptName
   FROM gameobject WHERE guid IN (49451,9100717) OR id=180717;
   SELECT entry,type,ScriptName FROM gameobject_template WHERE entry IN (180717,180718);
   SELECT * FROM gameobject_queststarter WHERE quest=8743;
   SELECT * FROM gameobject_questender WHERE quest=8743;
   ```
   Expect only owned 9100717, type 2 for 180717, type 5 for 180718 and one relation
   in each table. Compare unrelated world rows and campaign/material data to backup.
   On separate restored fixtures, confirm a GUID collision, alternate giver,
   customized 49451 or event/pool link aborts without partial world changes.
2. Start enabled. Set `.aqwareffort phase effort`. An eligible Scepter holder must
   not reward 8743 at the gong; item and reward history must remain unchanged.
   Repeat with `.aqwareffort phase disabled`, `war`, `open`, configuration disabled
   after restart, and unavailable campaign data on an isolated fixture.
3. Set `ready`. Test characters missing prerequisites, missing the Scepter, having
   it only in the bank, or already rewarded. Confirm stock rejection and unchanged
   campaign. A rejected request must release its reservation by the next world
   update so another qualified character can try. Test out-of-range and full
   reward-inventory cases according to the core's normal reward rules.
4. Open a valid reward dialog in READY, change phase to effort before submitting,
   and submit it. It must fail without consuming the Scepter. With a packet test
   client, send the same reward opcode directly in every disallowed phase and
   with an unauthorized giver GUID. Ordinary unrelated quest rewards must still work.
5. Restore READY. Have both eligible players submit together. Exactly one succeeds
   through the module; the loser retains its Scepter. Confirm the winner receives
   the normal stock reward/history and the journal records only that player.
6. Record the ceremony with game sound enabled. Verify roots -> runes -> gate,
   sounds 7114 -> 7116 -> 7115, visibility and physical collision at every stage.
   Observe from multiple distances and with a second client. Check for duplicate
   automatic model sounds. Adjust only centralized timing after measured results.
7. Inspect the character DB (replace ID with AQWarEffort.Id):
   ```sql
   SELECT * FROM aq_war_effort_campaign WHERE id=1;
   SELECT * FROM aq_war_effort_gong WHERE id=1;
   SELECT * FROM character_queststatus_rewarded WHERE guid=<winner_guid> AND quest=8743;
   ```
   Expect phase=3, equal new gong_rung_at/phase_started_at, accepted=1 and unchanged
   contributions/opened_at. Repeat packets and additional ringers must not change
   timestamps, rewards or sounds. The wall must stay absent after 12 seconds.
8. Restart at approximately 1, 5 and 9 seconds in separate runs. On each restart,
   confirm immediate absent wall and no sounds/replay, even before players visit
   Silithus. Leave and reload the grid; all parts remain absent.
9. Exercise recovery with debugger breakpoints on an isolated test worldserver:
   - Pause after PrepareGong's verified intent, before stock reward; terminate.
     Restart: no reward row means READY remains, intent clears, ringing is retryable.
   - Pause in ObserveGongReward, allow the DB worker to finish the stock save and
     verify the rewarded row externally, then terminate before UpdateGong accepts.
     Restart: phase becomes TEN_HOUR_WAR, journal accepted=1, no presentation.
   - Pause after the conditional acceptance UPDATE, before readback/presentation;
     terminate. Restart: already-open wall, no second transition or sounds.
   - Inject a journal write failure or deny the acceptance UPDATE on a test DB.
     Confirm no presentation and retained recovery evidence; restore DB service
     and restart. A durable reward is recovered once, otherwise READY remains.
10. Confirm `.aqwareffort phase ready` restores the wall after a completed test,
    while `war`/`open` skip ceremony. Test a phase override during live presentation:
    it cancels presentation and reconciles the requested phase. Overrides during a
    pending reward/recovery are refused. Recheck collection totals, event 22 and
    ordinary contribution rewards. Observe quest 8519 separately: its scene and
    temporary objects are not changed by this controller.

No live-client or production-server tests were performed while preparing this
patch. Automated validation is described in [GONG_VALIDATION.md](GONG_VALIDATION.md).
