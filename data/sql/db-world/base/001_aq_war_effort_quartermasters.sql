-- mod-aq-war-effort owns reporting on the existing AQ quartermasters only.
-- No creature spawns, paths, quests, game events or gates are changed.
-- Remove/disable the old mod-war-effort before installing this module.
UPDATE `creature_template`
SET `ScriptName` = 'npc_aq_war_effort_quartermaster', `npcflag` = `npcflag` | 1
WHERE `entry` IN (15700, 15701);

-- Fix Event 22 creatures configured for waypoint movement without a path.
UPDATE `creature` c
JOIN `game_event_creature` gec
    ON gec.`guid` = c.`guid`
LEFT JOIN `creature_addon` ca
    ON ca.`guid` = c.`guid`
SET c.`MovementType` = 0
WHERE gec.`eventEntry` = 22
  AND c.`id` IN (15663, 15696, 15701)
  AND c.`MovementType` = 2
  AND (ca.`path_id` IS NULL OR ca.`path_id` = 0);

-- Stock Horde War Effort quest 8610 "The Horde Needs More Runecloth Bandages!" is missing its repeatable flag.
UPDATE `quest_template_addon`
SET `SpecialFlags` = `SpecialFlags` | 1
WHERE `ID` = 8610
  AND (`SpecialFlags` & 1) = 0;
