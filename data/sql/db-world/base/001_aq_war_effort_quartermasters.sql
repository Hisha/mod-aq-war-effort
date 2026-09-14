-- mod-aq-war-effort owns reporting on the existing AQ quartermasters only.
-- No creature spawns, paths, quests, game events or gates are changed.
-- Remove/disable the old mod-war-effort before installing this module.
UPDATE `creature_template`
SET `ScriptName` = 'npc_aq_war_effort_quartermaster', `npcflag` = `npcflag` | 1
WHERE `entry` IN (15700, 15701);
