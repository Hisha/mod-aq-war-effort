# mod-aq-war-effort

A new AzerothCore module for persistent, realm-wide Ahn'Qiraj campaign progress.
This is an independent implementation, not the original upstream mod-war-effort.

The current foundation tracks all 15 Alliance and 15 Horde War Effort materials,
recognizes initial and repeatable resource quests, and reports totals through
quartermasters and administrator commands. Campaign state and contributions are
persisted together on every accepted turn-in. No collection or READY countdown
exists.

## Installation

1. Place this repository at `azerothcore-wotlk/modules/mod-aq-war-effort`.
   Remove/disable the old `mod-war-effort` module to avoid duplicate hooks and
   competing quartermaster scripts. Its database tables are not modified.
2. With worldserver stopped, apply the canonical SQL files to the appropriate
   databases (or use the core's configured module SQL updater):
   - Characters: `data/sql/db-characters/base/001_aq_war_effort.sql`
   - World: `data/sql/db-world/base/001_aq_war_effort_quartermasters.sql`
3. Regenerate your existing AzerothCore build, then build worldserver:
   ```sh
   cmake -S /path/to/azerothcore-wotlk -B /path/to/build -DMODULES=static
   cmake --build /path/to/build --target worldserver -j2
   ```
   Preserve the rest of your installation's normal CMake options. The modern core
   discovers both source files and `Addmod_aq_war_effortScripts()` automatically.
4. Copy the installed `mod_aq_war_effort.conf.dist` to
   `mod_aq_war_effort.conf` in your module config directory. Set goals, set
   `AQWarEffort.Enable = 1`, and restart worldserver.
5. Check `.aqwareffort status` and the startup log.

The module binds reporting to existing quartermasters 15700 (Horde) and 15701
(Alliance). It does not spawn them or the resource quest givers, enable game
events, or modify quest availability. Use your realm's existing War Effort NPC
and quest availability configuration (the reference installation associates
resource NPCs with game event 22). Applying the old quests.sql or warevent.sql
is not an installation step for this module. Existing old-module world changes
are not automatically undone.

## Configuration

The only template is `conf/mod_aq_war_effort.conf.dist`.

- `AQWarEffort.Enable`: defaults to 0; enables turn-in tracking and custom reports.
- `AQWarEffort.Id`: defaults to 1; independent contribution/state rows per ID.
- `AQWarEffort.Goal.{Alliance|Horde}.{Bandages|Food|Herbs|Metal|Leather}.{01|02|03}`:
  thirty goals measured in item quantities. These are three material requirements,
  not five visual progression thresholds.

All settings are read at startup and require restart after changes. Config reload
logs this requirement and does not switch campaigns or partially reload goals.
The scaffold's supplied 5/10/15 goals are retained; they are tiny test values,
not suggested production goals. Zero means that material is already satisfied.

Material order for each faction is:

| Category | Alliance .01 / .02 / .03 | Horde .01 / .02 / .03 |
|---|---|---|
| Bandages | Linen / Silk / Runecloth | Wool / Mageweave / Runecloth |
| Food | Rainbow Fin Albacore / Roast Raptor / Spotted Yellowtail | Lean Wolf Steaks / Baked Salmon / Spotted Yellowtail |
| Herbs | Stranglekelp / Arthas' Tears / Purple Lotus | Peacebloom / Firebloom / Purple Lotus |
| Metal | Iron / Thorium / Copper | Tin / Mithril / Copper |
| Leather | Light / Medium / Thick | Heavy / Rugged / Thick |

Each matching rewarded quest adds one turn-in: 20 items for bandages, food, herbs
and metal; 10 for leather. Goals are compared without overflow. Counts may exceed
a material's goal while other materials are still needed. Initial and repeat
quest IDs are explicit in the material definitions. The core's
OnPlayerCompleteQuest hook is invoked from Player::RewardQuest, after reward.

## Campaign phases

| Value | Phase | Current behavior |
|---|---|---|
| 0 | DISABLED | Administratively inactive; no contributions counted. |
| 1 | WAR_EFFORT | Material collection active indefinitely. |
| 2 | READY | All resources collected; waits indefinitely. |
| 3 | TEN_HOUR_WAR | Stored phase only; lifecycle is future work. |
| 4 | OPEN | Stored completed state; gate control is future work. |

Only WAR_EFFORT accepts contributions, and only while the config is enabled.
Other phases do not block quest rewards; those rewards do not alter campaign
totals. A rewarded contribution that satisfies both factions atomically saves
the totals and READY, logs the transition, and notifies the contributor. There
are no automatic transitions beyond READY. An administrator may override any
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
  fifteen unsigned BIGINT counters store numbers of rewarded turn-ins.
- `aq_war_effort_campaign`: primary key `id`; phase and unsigned BIGINT Unix epoch
  seconds `phase_started_at`, `gong_rung_at`, `opened_at`.

Both tables use InnoDB. Schema SQL is rerunnable and contains no destructive
resets. Runtime inserts use non-destructive duplicate-key handling. No automatic
import from legacy `wareffort` or `wareffort_campaign` is performed. If importing
old data manually, stop worldserver and copy counters before the new campaign's
first startup if you want first-initialization READY inference.

Every turn-in and manual phase change uses a synchronous transaction followed by
snapshot readback. A failed verification marks campaign data unavailable and stops
tracking until restart; status and logs expose the failure. No success is announced
for an unverified write. This requires one worldserver writer per campaign ID;
do not edit active rows concurrently. The transaction covers these module tables,
not core player inventory/quest persistence. Live crash/reward recovery remains
an integration test requirement.

Entering any different phase sets phase_started_at. Entering TEN_HOUR_WAR also
sets gong_rung_at; entering OPEN sets opened_at. Same-phase overrides preserve
timestamps. Backward overrides retain historical gong/open timestamps; explicit
re-entry replaces the corresponding timestamp. No timer uses these fields yet.

## Commands

| Command | Security | Result |
|---|---|---|
| `.aqwareffort status` | Moderator | ID, phase, availability, enabled state and supply completion. |
| `.aqwareffort scores` | Moderator | Both factions' detailed item totals and goals. |
| `.aqwareffort phase disabled` | Administrator | Set DISABLED. |
| `.aqwareffort phase effort` | Administrator | Set WAR_EFFORT. |
| `.aqwareffort phase ready` | Administrator | Set READY. |
| `.aqwareffort phase war` | Administrator | Set TEN_HOUR_WAR; does not start an event. |
| `.aqwareffort phase open` | Administrator | Set OPEN; does not open gates. |

All commands support console use. Missing/invalid/extra phase arguments print
usage. Changes report previous and new phase. Admin state overrides remain
available with the config disabled, provided campaign data loaded successfully.

## Scope and attribution

Quest 8743 (Bang a Gong!) is recognized and logged, but has no phase, spawn or
availability effect. No gong boolean or duplicate campaign state is maintained.
No Scarab Wall objects, AQ20/AQ40 AreaTriggers, opening animations/sounds, visual
supply piles, event armies, crystals, loot, or ten-hour timers are installed.

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

Planned lifecycle: WAR_EFFORT → READY → valid Scepter bearer rings gong →
TEN_HOUR_WAR → ten restart-safe real hours → OPEN. Scarab Wall and event lifecycle
implementation are future milestones.

The original repository's MIT license and copyright are retained alongside the
new repository's MIT notice in LICENSE. Source/config files retain the reference's
AGPL-3.0-or-later attribution; those notices are not replaced by the scaffold's
MIT notice. See the file-level notices and
[GNU AGPL version 3](https://www.gnu.org/licenses/agpl-3.0.html).
