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