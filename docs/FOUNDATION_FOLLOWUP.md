# Foundation follow-up: collectors, goals and material quantities

## Upgrade without losing progression

1. Stop worldserver and back up the character database. Do not run the old binary
   against converted counters, or run multiple writers during migration.
2. Apply `data/sql/db-characters/updates/2026_09_14_00_material_quantities.sql`.
   Execute module SQL serially. Fresh installs get the complete current schema
   from `base/001_aq_war_effort.sql`; applying the update afterward is harmless.
3. Update the active config, using the existing category-based keys. The template
   now supplies valid small goals. An existing value of 5 or 25 for Light Leather
   is rejected. Use `AQWarEffort.Goal.Alliance.Leather.01 = 10` (or another positive
   multiple of 10). There is no `AQWarEffort.Goal.Alliance.LightLeather` alias.
4. Build/install the module and restart. Check startup errors and
   `.aqwareffort status`, then test initial and repeatable quests.

`aq_war_effort_schema` explicitly records counter semantics. An unversioned
existing table is version 1; a fresh table is version 2. The new base SQL detects
an already existing contribution table, so rerunning base SQL before the update
cannot incorrectly label old counters as quantities. The update locks the marker,
converts all rows for all IDs/factions, and advances the version in the same
transaction. Version 2 is a no-op on re-execution. Unsupported versions and
quantity overflow abort without changing counters. The existing InnoDB schema is
required. No campaign phase or timestamp is updated by this SQL.

Historical non-leather columns are multiplied by 20, and leather by 10. These are
verified historical storage multipliers, independent of current config or future
quest customizations. Existing zeros remain zero. Missed historical contributions
cannot be reconstructed from these tables; no speculative backfill is performed.
The runtime refuses to load unmigrated counters. Never manually change the marker
to bypass the migration or rerun an old binary after conversion.

An existing READY/OPEN/DISABLED phase remains authoritative, even if it was set
using the earlier small test goals. This follow-up never resets it. Use the
existing administrative phase command deliberately if collection should resume.

## Verified contribution mappings

Audited from the local AzerothCore stock `quest_template.sql` and
`quest_template_addon.sql`, core commit
`06234df3d5ab26c93f4f1f06f3edb828b73ecd3c`. Each pair has the same single required
item and quantity. Of the 30 follow-ups, 29 have the repeatable flag; stock quest
8610 (Horde Runecloth) has SpecialFlags=0 in this checkout. The module counts its
legitimate reward but does not alter stock quest flags or availability. Confirm
this exception against the live world DB if repeated 8610 turn-ins are needed.
The checked-out
module already contained these IDs. No unsupported alternative IDs were invented.

| Faction | Material | Initial | Follow-up | Item ID | Quantity per reward |
|---|---|---:|---:|---:|---:|
| Alliance | Linen Bandages | 8517 | 8518 | 1251 | 20 |
| Alliance | Silk Bandages | 8520 | 8521 | 6450 | 20 |
| Alliance | Runecloth Bandages | 8522 | 8523 | 14529 | 20 |
| Alliance | Rainbow Fin Albacore | 8524 | 8525 | 5095 | 20 |
| Alliance | Roast Raptor | 8526 | 8527 | 12210 | 20 |
| Alliance | Spotted Yellowtail | 8528 | 8529 | 6887 | 20 |
| Alliance | Stranglekelp | 8503 | 8504 | 3820 | 20 |
| Alliance | Arthas' Tears | 8509 | 8510 | 8836 | 20 |
| Alliance | Purple Lotus | 8505 | 8506 | 8831 | 20 |
| Alliance | Iron Bars | 8494 | 8495 | 3575 | 20 |
| Alliance | Thorium Bars | 8499 | 8500 | 12359 | 20 |
| Alliance | Copper Bars | 8492 | 8493 | 2840 | 20 |
| Alliance | Light Leather | 8511 | 8512 | 2318 | 10 |
| Alliance | Medium Leather | 8513 | 8514 | 2319 | 10 |
| Alliance | Thick Leather | 8515 | 8516 | 4304 | 10 |
| Horde | Wool Bandages | 8604 | 8605 | 3530 | 20 |
| Horde | Mageweave Bandages | 8607 | 8608 | 8544 | 20 |
| Horde | Runecloth Bandages | 8609 | 8610 | 14529 | 20 |
| Horde | Lean Wolf Steaks | 8611 | 8612 | 12209 | 20 |
| Horde | Baked Salmon | 8615 | 8616 | 13935 | 20 |
| Horde | Spotted Yellowtail | 8613 | 8614 | 6887 | 20 |
| Horde | Peacebloom | 8549 | 8550 | 2447 | 20 |
| Horde | Firebloom | 8580 | 8581 | 4625 | 20 |
| Horde | Purple Lotus | 8582 | 8583 | 8831 | 20 |
| Horde | Tin Bars | 8542 | 8543 | 3576 | 20 |
| Horde | Mithril Bars | 8545 | 8546 | 3860 | 20 |
| Horde | Copper Bars | 8532 | 8533 | 2840 | 20 |
| Horde | Heavy Leather | 8588 | 8589 | 4234 | 10 |
| Horde | Rugged Leather | 8600 | 8601 | 8170 | 10 |
| Horde | Thick Leather | 8590 | 8591 | 4304 | 10 |

Stock Singed Corestone quests (8530/8531, 8617/8618) and commendation/help quests
are not contributions to these 30 materials and remain excluded. The module
validates every loaded pair's item/quantity rather than trusting these mappings
on a customized realm; mismatches disable tracking and log the quest/material.
Reward-time validation also detects changed quest requirements after startup.

The checked-out core calls `OnPlayerCompleteQuest` at the end of
`Player::RewardQuest`, after consuming items and issuing rewards. Retaining this
single hook avoids counting objective completion or double-counting via a second
hook. Initial 8511 and repeatable 8512 both add 10 Light Leather directly to
`leather01` for faction 0. Every other pair follows the same path. Classification
now uses the rewarded quest's faction rather than the player's current faction.

The exact live zero-count cause is **not proven** by the available checkout: its
old mapping/hook already handled 8511/8512. The cited live key `...LightLeather`
also differs from this checkout's `.Leather.01`. A mismatched deployed binary,
config, phase, or unavailable DB state requires live evidence to distinguish.
Recognized rewards now log and tell the player why they were rejected (disabled
module, unavailable data, or non-WAR_EFFORT phase). Successful contributions have
DEBUG logs containing quest, amount, material, campaign and total. Startup
validation rejects unknown goal keys instead of silently using defaults.

## Event 22 authority

With `AQWarEffort.Enable=1`, `SyncCollectionEvent` runs after successful startup
load and after committed changes, including natural READY and manual overrides.
It checks `IsActiveEvent(22)` before calling `StartEvent(22, false)` or
`StopEvent(22, false)`. WAR_EFFORT requires active; every other phase requires
inactive. No SQL date updates and no `overwrite=true` calls are used. Missing or
disabled event definitions produce errors. Failed loading/configuration closes
collectors so the realm does not silently collect while tracking is unavailable.

GameEventScript start/stop hooks also reconcile scheduler or external changes,
otherwise the stock calendar could undo the startup decision. A re-entrancy guard
prevents recursive event callbacks; matching event state is a no-op. Logs record
actual corrections, not repeated successful checks. Turning off the module in
config relinquishes event control; it does not alter persistent phase or dates.

This is only collector availability. No Scarab Wall, gong transition, battle
spawns, Ten Hour War lifecycle, or timing behavior is added.
