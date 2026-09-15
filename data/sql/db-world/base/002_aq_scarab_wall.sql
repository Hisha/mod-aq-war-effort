-- Apply serially with worldserver stopped. Own ONLY these three reserved spawns.
-- Canonical placement: zone_silithus.cpp quest 8519 steps 37, 38, 41.
-- Script summons use zero quaternion; the core derives orientation 0 => (0,0,0,1).
-- No template, quest, AreaTrigger, event date, or progression changes.
DROP PROCEDURE IF EXISTS `aq_war_effort_restore_wall`;
DELIMITER $$
CREATE PROCEDURE `aq_war_effort_restore_wall`()
BEGIN
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        RESIGNAL;
    END;
    START TRANSACTION;
    IF EXISTS (SELECT 1 FROM `gameobject`
        WHERE `guid` IN (9100146, 9100147, 9100148)
        AND (`id` <> `guid` - 8924000 OR `map` <> 1
            OR `ScriptName` IS NULL OR BINARY `ScriptName` <> 'go_aq_war_effort_wall')) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: wall GUID collision; no world data changed';
    END IF;
    IF EXISTS (SELECT 1 FROM `gameobject`
        WHERE `id` IN (176146, 176147, 176148) AND `map` = 1
        AND ABS(`position_x` + 8130) < 10 AND ABS(`position_y` - 1525) < 10
        AND `guid` NOT IN (9100146, 9100147, 9100148)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: existing wall near canonical position; review duplicate placements first';
    END IF;
    IF EXISTS (SELECT 1 FROM `game_event_gameobject` WHERE `guid` IN (9100146, 9100147, 9100148))
        OR EXISTS (SELECT 1 FROM `pool_gameobject` WHERE `guid` IN (9100146, 9100147, 9100148)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: owned wall linked to an event/pool; review before installation';
    END IF;
    INSERT INTO `gameobject`
        (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`,
         `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`,
         `spawntimesecs`, `animprogress`, `state`, `ScriptName`, `VerifiedBuild`, `Comment`)
    VALUES
        (9100146, 176146, 1, 1, 1, -8130, 1525, 17.5, 0, 0, 0, 0, 1, 0, 100, 1, 'go_aq_war_effort_wall', 0, 'mod-aq-war-effort: Scarab Wall'),
        (9100147, 176147, 1, 1, 1, -8130, 1525, 17.5, 0, 0, 0, 0, 1, 0, 100, 1, 'go_aq_war_effort_wall', 0, 'mod-aq-war-effort: Scarab Wall'),
        (9100148, 176148, 1, 1, 1, -8130, 1525, 17.5, 0, 0, 0, 0, 1, 0, 100, 1, 'go_aq_war_effort_wall', 0, 'mod-aq-war-effort: Scarab Wall')
    ON DUPLICATE KEY UPDATE
        `spawnMask` = 1, `phaseMask` = 1,
        `position_x` = -8130, `position_y` = 1525, `position_z` = 17.5,
        `orientation` = 0, `rotation0` = 0, `rotation1` = 0, `rotation2` = 0, `rotation3` = 1,
        `spawntimesecs` = 0, `animprogress` = 100, `state` = 1;
    COMMIT;
END$$
DELIMITER ;
CALL `aq_war_effort_restore_wall`();
DROP PROCEDURE IF EXISTS `aq_war_effort_restore_wall`;
