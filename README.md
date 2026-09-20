# mod-aq-war-effort

A new AzerothCore module for persistent, realm-wide Ahn'Qiraj campaign progress.
This is an independent implementation, not the original upstream mod-war-effort.

The current foundation tracks all 15 Alliance and 15 Horde War Effort materials,
recognizes initial and repeatable resource quests, and reports totals through
quartermasters and administrator commands. Campaign state and contributions are
persisted together on every accepted turn-in. No collection or READY countdown
exists. The stock Scarab Wall is now phase-controlled; see
[Scarab Wall installation and tests](docs/SCARAB_WALL.md). The authorized Scarab Gong
now accepts the stock finale in READY and opens those same wall spawns; see
[Scarab Gong installation, recovery and live tests](docs/SCARAB_GONG.md).
The restart-safe war duration now advances TEN_HOUR_WAR to OPEN; see
[Ten Hour War timing and live tests](docs/TEN_HOUR_WAR_TIMER.md).
A separate content controller now reconciles one temporary stock red crystal
outside the wall during an active war; see
[content inventory, lifecycle and live tests](docs/TEN_HOUR_WAR_CONTENT.md).
Hive'Ashi, Hive'Regal and Hive'Zora share a four-stage runtime battlefront controller;
see [Zora inventory, implementation and live tests](docs/HIVE_ZORA_BATTLEFRONT.md).
All three use the existing durable named-boss table, with at most 18 active
battlefront creatures. The Zora milestone adds no SQL or rewards.

## Installation

1. Place this repository at `azerothcore-wotlk/modules/mod-aq-war-effort`.
   Remove/disable the old `mod-war-effort` module to avoid duplicate hooks and
   competing quartermaster scripts. Its database tables are not modified.
2. With worldserver stopped, apply the canonical SQL files to the appropriate
   databases (or use the core's configured module SQL updater):
   - Characters: `data/sql/db-characters/base/001_aq_war_effort.sql`
   - Characters: `data/sql/db-characters/base/002_aq_scarab_gong.sql` (existing installs too)
   - Characters: `data/sql/db-characters/base/003_aq_named_war_boss_kills.sql` (named bosses)
   - World: `data/sql/db-world/base/001_aq_war_effort_quartermasters.sql`
   - World: `data/sql/db-world/base/002_aq_scarab_wall.sql` (also apply to existing installs)
   - World: `data/sql/db-world/base/003_aq_scarab_gong.sql` (existing installs too)
3. Regenerate your existing AzerothCore build, then build worldserver:
   ```sh
   cmake -S /path/to/azerothcore-wotlk -B /path/to/build -DMODULES=static
   cmake --build /path/to/build --target worldserver -j2
   ```
   Preserve the rest of your installation's normal CMake options. The modern core
   discovers source files and `Addmod_aq_war_effortScripts()` automatically.
4. Copy the installed `mod_aq_war_effort.conf.dist` to
   `mod_aq_war_effort.conf` in your module config directory. Set goals, set
   `AQWarEffort.Enable = 1`, and restart worldserver.
5. Check `.aqwareffort status` and the startup log.

The module binds reporting to existing quartermasters 15700 (Horde) and 15701
(Alliance). When enabled, it controls stock collection game event 22: active only
in WAR_EFFORT, inactive in every other phase. It does not add collector spawns or
change event dates. Existing stock event membership controls which NPCs appear.
Applying the old quests.sql or warevent.sql is not an installation step. Existing
old-module world changes are not automatically undone.

**Existing installations:** stop worldserver and apply
`data/sql/db-characters/updates/2026_09_14_00_material_quantities.sql` before
running this version. Review [the follow-up and upgrade notes](docs/FOUNDATION_FOLLOWUP.md).
Do not reset campaign rows or timestamps. Update your active config goals too;
changing only the distributed template does not change your running configuration.

## Configuration

The only template is `conf/mod_aq_war_effort.conf.dist`.

- `AQWarEffort.Enable`: defaults to 0; enables contribution tracking, event 22 control and custom reports.
- `AQWarEffort.Id`: defaults to 1; independent contribution/state rows per ID.
- `AQWarEffort.Goal.{Alliance|Horde}.{Bandages|Food|Herbs|Metal|Leather}.{01|02|03}`:
  thirty goals measured in item quantities. These are three material requirements,
  not five visual progression thresholds.

`AQWarEffort.TenHourWar.Duration` defaults to 36000 real seconds and supports
`.reload config`. The new duration applies against the original persisted war start;
shortening it can immediately advance an existing war to OPEN. Invalid durations
disable automatic expiration only. All other settings require restart; config reload
does not switch campaigns or partially reload goals.
Defaults are one/two/three full turn-ins: 20/40/60 for non-leather and 10/20/30
for leather. Each goal must be at least one turn-in and an exact multiple of its
quantity. Zero, negative, malformed, overflowing and non-multiple values are
errors, not rounded. Unknown goal keys also fail validation. For example, Light
Leather uses `AQWarEffort.Goal.Alliance.Leather.01`, not `...Alliance.LightLeather`.
Invalid configuration leaves progression data untouched, disables tracking and
stops event 22 until corrected and restarted (when this module is enabled).

Material order for each faction is:

| Category | Alliance .01 / .02 / .03 | Horde .01 / .02 / .03 |
|---|---|---|
| Bandages | Linen / Silk / Runecloth | Wool / Mageweave / Runecloth |
| Food | Rainbow Fin Albacore / Roast Raptor / Spotted Yellowtail | Lean Wolf Steaks / Baked Salmon / Spotted Yellowtail |
| Herbs | Stranglekelp / Arthas' Tears / Purple Lotus | Peacebloom / Firebloom / Purple Lotus |
| Metal | Iron / Thorium / Copper | Tin / Mithril / Copper |
| Leather | Light / Medium / Thick | Heavy / Rugged / Thick |

Each matching rewarded quest adds actual item quantity: 20 items for bandages,
food, herbs and metal; 10 for leather. Database values and scores are item totals,
with no display multiplier. Counts may exceed a material's goal while other materials are still needed. Initial and repeat
quest IDs, required item IDs and quantities are explicit and validated against all
60 loaded quest templates at startup. The rewarded quest determines the faction
counter, including in cross-faction play. The core's
OnPlayerCompleteQuest hook is invoked from Player::RewardQuest, after reward.

## Campaign phases

| Value | Phase | Current behavior |
|---|---|---|
| 0 | DISABLED | Administratively inactive; no contributions counted. |
| 1 | WAR_EFFORT | Material collection active indefinitely; wall closed. |
| 2 | READY | Wall closed; an eligible stock 8743 reward at the authorized gong starts opening. |
| 3 | TEN_HOUR_WAR | Live ceremony finishes with wall absent; restart opens immediately without replay. |
| 4 | OPEN | Stored completed state; wall absent. |

Only WAR_EFFORT accepts contributions, and only while the config is enabled.
Other phases do not block collection quest rewards; those rewards do not alter campaign
totals. A rewarded contribution that satisfies both factions atomically saves
the totals and READY, logs the transition, and notifies the contributor. The gong can then advance READY to TEN_HOUR_WAR. The configured real-time duration then advances TEN_HOUR_WAR to OPEN. An administrator may override any
phase, including OPEN, without resetting material counts.

Startup initializes absent faction rows without changing existing counts, then
creates a missing campaign row as WAR_EFFORT or READY according to those counts.
This happens even when tracking is disabled in configuration. Once the campaign
row exists, its phase always wins, regardless of totals or config changes.
Restarting or enabling the module does not rewrite an existing DISABLED or OPEN.
A manual return to WAR_EFFORT with already-complete totals stays there until the
next accepted contribution; startup does not undo the override.

## Database and durability

- `aq_war_effort`: primary key `(id, faction)`, factions 0=Alliance and 1=Horde;
  fifteen unsigned BIGINT counters store actual material quantities.
- `aq_war_effort_campaign`: primary key `id`; phase and unsigned BIGINT Unix epoch
  seconds `phase_started_at`, `gong_rung_at`, `opened_at`.
- `aq_war_effort_schema`: singleton key `id=1`; `material_data_version=2` means
  material quantities (historical version 1 means turn-ins).

All three tables use InnoDB. Schema SQL is rerunnable and contains no destructive
resets. Runtime inserts use non-destructive duplicate-key handling. No automatic
import from legacy `wareffort` or `wareffort_campaign` is performed. Any manual
import must use version-2 material quantities. Never insert legacy
turn-in counts into an already migrated table. See the upgrade notes for the
versioned conversion of this module's historical rows.

Every turn-in and manual phase change uses a synchronous transaction followed by
snapshot readback. A failed verification marks campaign data unavailable and stops
tracking until restart; status and logs expose the failure. No success is announced
for an unverified write. This requires one worldserver writer per campaign ID;
do not edit active rows concurrently. The transaction covers these module tables,
not core player inventory/quest persistence. Live crash/reward recovery remains
an integration test requirement.

Entering a different phase sets phase_started_at. Only a real, accepted 8743 gong
reward sets gong_rung_at; administrative `phase war` preserves its previous value.
Entering OPEN sets opened_at. Same-phase overrides preserve timestamps. Neither
real gong acceptance nor an administrative return to TEN_HOUR_WAR changes opened_at.
The war timer uses the current gong timestamp when it equals phase_started_at;
a later administrative war uses its own phase_started_at. Shutdown time counts.

## Commands

| Command | Security | Result |
|---|---|---|
| `.aqwareffort status` | Moderator | ID, phase, availability, enabled state, supplies and active war timing. |
| `.aqwareffort scores` | Moderator | Both factions' detailed item totals and goals. |
| `.aqwareffort phase disabled` | Administrator | Set DISABLED. |
| `.aqwareffort phase effort` | Administrator | Set WAR_EFFORT. |
| `.aqwareffort phase ready` | Administrator | Set READY. |
| `.aqwareffort phase war` | Administrator | Set TEN_HOUR_WAR; start its timer without fabricating a gong acceptance. |
| `.aqwareffort phase open` | Administrator | Set OPEN; wall becomes absent without ceremony. |

All commands support console use. Missing/invalid/extra phase arguments print
usage. Changes report previous and new phase. Admin state overrides remain
available with the config disabled, provided campaign data loaded successfully and no gong reward/recovery is pending.

## Scope and attribution

Quest 8743 remains stock-owned for eligibility, item consumption, rewards, reputation
and achievement/quest history. The module authorizes one dedicated 180717 gong,
persists READY -> TEN_HOUR_WAR with a durable recovery journal, and presents
roots -> runes -> gate on its existing wall spawns. No AQ20/AQ40 AreaTrigger changes,
quest 8519 changes, Jonathan/Rajaxx, visual supply piles, event armies, crystals,
or loot are installed. The war timer controls campaign state only.
Only the stock resource collection event (22) is synchronized, not battle events.

[AzerothCore mod-war-effort](https://github.com/azerothcore/mod-war-effort) is the
source/reference for resource quest IDs, multipliers, quartermaster IDs and
reporting language. Inspected local reference commit:
`69f3f0bcc2095b75c3ed3825df5a26a8d4b79530`.
The manager, database design, transactions, commands, config lifecycle and gossip
implementation are intentionally rewritten.

For later milestones, the reference's `src/mod_aq_war_effort_scripts.cpp` contains
Jonathan (15693) dialogue and temporary Rajaxx (15341) behavior. Its
`data/sql/db-world/updates/warevent.sql` contains Colossus of Zora/Regal/Ashi
(15740/15741/15742), paths, combat groups, crystal, loot and dialogue data. These
are documented sources for future selective migration, not active SQL here.
Legacy permanent spawns, helper shell scripts and unfinished visual code are
intentionally excluded.

Implemented lifecycle: WAR_EFFORT → READY → successful authorized gong reward →
TEN_HOUR_WAR, including the ordered wall-opening ceremony → timed OPEN.
Ten Hour War battle content remains a future milestone.

The original repository's MIT license and copyright are retained alongside the
new repository's MIT notice in LICENSE. Source/config files retain the reference's
AGPL-3.0-or-later attribution; those notices are not replaced by the scaffold's
MIT notice. See the file-level notices and
[GNU AGPL version 3](https://www.gnu.org/licenses/agpl-3.0.html).
