# Restart-safe Ten Hour War timing

This milestone adds only TEN_HOUR_WAR → OPEN timing. No world/character SQL or
schema migration is required. Install the changed module files, rebuild/install
worldserver with your existing AzerothCore build settings, and restart. Existing
campaign rows, supplies, journal records and client patches are retained.

## Configuration and authority

```ini
AQWarEffort.TenHourWar.Duration = 36000
```

Seconds, default ten real hours. Valid decimal integers are 1..4294967295.
Zero, signs, fractions, non-numbers and overflow disable automatic expiration,
with an error log and status diagnostic; collection and gong authorization are
not disabled or changed. Correct the value and `.reload config` or restart.
Duration is the only newly reloadable setting; campaign ID, enable and goals
retain their restart requirement. Edit the active `.conf`, not just `.conf.dist`.

A legitimate accepted gong sets `gong_rung_at == phase_started_at`. While in
TEN_HOUR_WAR, that nonzero equality identifies the current gong timer. Otherwise,
`phase_started_at` is the administrative timer origin; a historical nonzero gong
timestamp cannot shorten a later administrative war. Forcing a different phase
then `phase war` sets a new phase start but never modifies gong history. Repeating
`phase war` while already in that phase preserves its start/deadline.

The timestamps have one-second resolution. If an administrator leaves and reenters
war in the exact second of a legitimate gong, both origins are numerically equal:
the deadline is still correct, but status identifies it as Scarab Gong. Existing
fields cannot distinguish that label-only edge case; no new schema is justified.

Every world update checks elapsed wall-clock seconds against the cached duration.
There are no countdown writes or countdown DB reads. Shutdown time counts.
Startup reconciles after existing gong recovery and before final event/wall
synchronization. An already expired war transitions immediately. Config reload
also reconciles immediately using the ORIGINAL persisted origin, never reload time.
For example, replacing 36000 with 300 after 400 seconds immediately opens the
campaign. Lengthening duration extends an active war, but cannot reopen OPEN.

Expiration uses existing `SetPhase(OPEN)` and `Persist`: copy the campaign, set
phase/phase_started_at/opened_at, synchronously commit and verify the full database
snapshot before publishing OPEN. Supplies and gong_rung_at retain their values;
the ordinary persistence path does not write the gong journal. Verification failure
makes campaign management unavailable until corrected/restarted, as before.
One worldserver writer per campaign ID remains required.

Event 22 stays inactive; the existing wall controller reconciles OPEN; the existing
gong authorization rejects further ringing. No wall/gong script changes are made.
As with an administrative OPEN, expiration cancels unfinished presentation. Use
300 or more seconds for development tests that need the full 12-second ceremony.
Pending gong protection remains intact. Disabled/unavailable management never
expires a war; re-enabling/recovering evaluates the original deadline at startup.

Missing zero origins disable expiration and show a status diagnostic. Future
origins clamp elapsed to zero until the system clock catches up, preventing unsigned
underflow. Keep the host clock correct: intentional clock jumps affect real-time
expiration. Invalid data is not repaired by fabricating timestamps.

## Status

During an enabled, available TEN_HOUR_WAR with valid timing:

```text
Ten Hour War duration: 10:00:00
Elapsed: 02:17:31
Remaining: 07:42:29
Timer origin: Scarab Gong
```

Administrative origins read `Administrative phase start`. Hours do not wrap at
24. Outside TEN_HOUR_WAR there is no active timer display. Expiration occurs at
the first world update at/after the deadline (or startup/reload reconciliation).

## Automated validation

From the module root:

```sh
python3 tests/test_campaign.py
python3 tests/test_scarab_gong.py
python3 tests/test_scarab_gong_sql.py --core /path/to/azerothcore-wotlk --mysql-root /path/to/mysql-prefix
```

`test_campaign.py` compiles actual Manager methods with a deterministic wall clock
and mock core/database boundaries. It checks all requested timer cases: current
gong/admin/stale origins; restart while active and after deadline; runtime OPEN;
OPEN across restart; opened_at, unchanged gong/supplies; short duration; status over
24 hours; new duration against original origin after restart/reload. It also checks
invalid/overflow durations, missing/future origins, no countdown commits, pending
protection, disabled mode, persistence failure, same-phase preservation and manual
OPEN. The collection suite covers all 60 reward IDs and existing progression,
validation, event synchronization and persistence failure behavior.

The gong/wall harness compiles their actual implementation with test doubles and
checks authorization, concurrent reservation, delayed stock reward durability,
crash recovery, failure handling and ordered presentation. The SQL suite uses an
isolated MySQL initialization (never the running realm) to exercise existing
convergent gong SQL and exact atomic acceptance SQL. These are not live-client tests.

## Exact live test procedure

Use a test realm/campaign; administrative phase changes intentionally alter its
phase. Do not reset supplies, timestamps or journal records with SQL. Preserve a
read-only baseline before each run (replace campaign ID 1 if configured otherwise):

```sql
SELECT * FROM aq_war_effort_campaign WHERE id = 1;
SELECT * FROM aq_war_effort WHERE id = 1 ORDER BY faction;
SELECT * FROM aq_war_effort_gong WHERE id = 1;
```

1. Install/build the files, set the active config duration to `300`, enable the
   module, restart. Check logs and `.aqwareffort status` for valid configuration.
2. **Legitimate gong:** in READY, use a legitimately eligible character that has
   completed stock 8742 and has not rewarded 8743. Complete stock 8743 at entry
   180717 / GUID 9100717. Confirm the existing roots → runes → gate ceremony,
   TEN_HOUR_WAR, duration 00:05:00, Scarab Gong origin, and equal nonzero gong/start
   timestamps. Capture supplies and accepted journal. Do not use quest cheats to
   substitute for the eligibility test.
3. **Runtime expiry:** keep worldserver running for five minutes from the persisted
   origin. Confirm automatic OPEN, no active countdown, phase_started_at=opened_at
   near expiration, unchanged gong/supplies/journal, event 22 inactive, wall absent,
   no further gong reward. Restart and confirm OPEN and timestamps unchanged.
4. **Administrative/stale gong:** wait at least one second after step 2's acceptance,
   then from OPEN issue `.aqwareffort phase war`. Confirm a new phase_started_at,
   unchanged historical gong timestamp and Administrative phase start/00:05:00.
   Repeat the same command after 30 seconds; elapsed must not reset. Restart before
   expiration and verify remaining time has continued from the same persisted start.
5. **Offline expiry:** from OPEN issue `phase war` again. Stop worldserver before
   its five-minute deadline; start it after that deadline. Confirm OPEN immediately,
   no ceremony replay, opened_at set at startup, supplies/gong/journal unchanged.
   Restart once more and confirm durable OPEN without new timestamps.
6. **Changed config at restart:** set duration 36000, `.reload config`, and from OPEN
   issue `phase war`. Wait more than 300 seconds, stop, change duration to 300 and
   restart. Confirm immediate OPEN against the original start. Repeat with live
   `.reload config` instead of restart to verify the same behavior.
7. **Formatting/validation:** set 90061 and reload, enter war from OPEN. Status must
   show duration 25:01:01. Set duration 0 and reload: status/log must report timing
   disabled, without changing campaign data. Restore 300 and reload: evaluate the
   original start. Repeat with `abc` and `-1` if desired.
8. **Commands and guards:** verify immediate `phase open`; test `disabled`, `effort`,
   `ready`, `war`, `open` retain their existing effects. Outside war there must be
   no countdown. Confirm event 22 only in effort and stock 8743 remains blocked
   outside READY. Preserve existing pending-gong crash-recovery tests in SCARAB_GONG.md.
9. Restore production duration 36000 and desired test phase. Do not leave production
   with a short development duration. Do not reset or clear the accepted gong journal.
