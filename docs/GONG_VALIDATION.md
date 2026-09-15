# Gong follow-up implementation and validation

Base module: 6ca5ec84f852a1b4a5ef9e847c1e7a45488fdfc3.
Core used for compilation: 06234df3d5ab26c93f4f1f06f3edb828b73ecd3c.

The checkout already contained the authorized gong, recovery journal and wall
controller. This update extends them. No new schema, SQL migration, spawns or
quest-progression changes are required. Existing fresh-install base SQL remains
complete. The user's normal 8742 -> 8743 progression is preserved.

## Detection and persistence

The existing gong-specific AllGameObjectScript::CanGameObjectQuestReward callback
observes successful stock reward of 8743 through the dedicated 180717 gong. It is
post-reward despite its name. The existing PlayerScript reward callback remains
responsible for collection quests and logs gong observations; it does not create
a second opening trigger. Pre-dispatch phase authorization and the durable journal
are retained, including serialization of concurrent attempts.

Gong acceptance now uses Manager::EnterPhase and Manager::Persist. Only real gong
acceptance assigns gong_rung_at. Admin `phase war` sets phase/phase_started_at but
preserves gong_rung_at and opened_at and never starts the presentation. Late or
duplicate callbacks leave campaign timestamps unchanged and are debug-logged.

Stock RewardQuest queues an asynchronous character save. Therefore acceptance
runs at the first end-of-world-update opportunity after the rewarded-quest row
is durable, not blindly before stock persistence finishes. No cosmetic timer
postpones acceptance. Manager::Persist atomically accepts the journal and sets
TEN_HOUR_WAR plus equal phase_started_at/gong_rung_at current server timestamps.
It preserves opened_at and contributions. Common campaign readback and event-22
synchronization complete before presentation starts. The gong branch defers normal
wall synchronization to the existing controller's live/recovery path, preventing
the normal phase sync from bypassing the live sequence.

## Scheduling and restart

The manager's EventMap is advanced from the existing WorldScript::OnUpdate. Roots
start immediately after verified persistence; runes at 4 seconds, gate at 8,
final absence at 12. All manipulation stays in AQScarabWall, matching dedicated
spawn IDs and script identity. No sleeps, second controller or retained object
pointers are introduced. Missing parts skip cosmetics and converge to the final
open state through existing synchronization/AddWorld hooks.

Startup resets cosmetic events. Persisted TEN_HOUR_WAR opens the wall immediately,
including after a crash halfway through presentation. A durable stock reward with
an unaccepted journal is recovered through the same persistence path without
animation. No interrupted animation resumes. TEN_HOUR_WAR keeps event 22 inactive
and does not advance automatically to OPEN.

## Validation performed

- Built all four module translation units against real AzerothCore headers with
  the existing CMake compiler configuration and -Wall -Wextra -Werror; assembled
  libmod-aq-war-effort.a. This is a module static-library build, not a full
  worldserver executable relink or live-server deployment.
- Runtime harness executes actual manager gong-persistence/phase-entry code and
  actual gong/wall source against core/DB/EventMap mocks. Passed phase and ownership
  guards, concurrent reservation, rejected stock rewards, delayed save durability,
  duplicate callbacks, persistence errors, crash recovery without replay, wall
  stages/sounds/reload, missing-part fallback, admin timestamp separation and
  preservation of contributions/opened_at.
- Isolated MySQL 8.4 tests ran the exact conditional acceptance SQL now located
  in Manager::Persist. Passed reward requirement, atomic phase/journal update,
  idempotence, original READY epoch validation and contribution preservation.
  Existing convergent world SQL and ownership conflict tests also passed.
- Existing external collection regression harness passed all 60 quest IDs,
  quantities, event-22 synchronization, disabled/config validation, persistence,
  phase authority and overflow cases. Its admin-war timestamp expectation was
  updated to require that gong_rung_at remains unchanged.
- AzerothCore C++ style and whitespace checks passed.

Re-run portable tests from module root:

```sh
python3 tests/test_scarab_gong.py
python3 tests/test_scarab_gong_sql.py \
  --core /path/to/azerothcore-wotlk \
  --mysql-root /path/to/mysql-prefix
```

Tests create temporary files/databases only. SQL tests initialize an isolated
MySQL instance without listening for connections; they never connect to a live
game database. Python 3, a C++20 compiler and MySQL 8 are needed as applicable.

Before deployment, build/link your complete worldserver normally. Verify that
your deployed core retains the inspected post-reward callback ordering,
asynchronous reward save and end-of-update callback after map workers finish.
The journal assumes one worldserver owns a campaign ID; do not edit campaign rows
externally during a pending reward. Live-test the ordinary eligible character,
roots/runes/gate animations, collision and sounds 7114/7116/7115 using
SCARAB_GONG.md. Four-second stage timing remains provisional. No live client,
network packet injection or worldserver crash test was performed here.
