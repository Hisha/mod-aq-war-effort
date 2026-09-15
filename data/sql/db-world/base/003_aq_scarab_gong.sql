-- Convergent pre-release base SQL. Apply serially with worldserver STOPPED.
-- Verified stock decorative spawn 49451 supplies the placement; use questgiver 180717.
-- Leave both gameobject templates unchanged. Runtime clears INTERACT_COND on our instance only.
DROP PROCEDURE IF EXISTS `aq_war_effort_restore_gong`;
DELIMITER $$
CREATE PROCEDURE `aq_war_effort_restore_gong`()
BEGIN
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        RESIGNAL;
    END;
    START TRANSACTION;
    IF NOT EXISTS (SELECT 1 FROM `gameobject_template` WHERE `entry` = 180717 AND `type` = 2
        AND `AIName` = '' AND `ScriptName` = '')
        OR NOT EXISTS (SELECT 1 FROM `gameobject_template` WHERE `entry` = 180718 AND `type` = 5) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: non-stock gong templates; review legacy module changes first';
    END IF;
    IF EXISTS (SELECT 1 FROM `gameobject` WHERE `guid` = 9100717
        AND (`id` <> 180717 OR `map` <> 1 OR COALESCE(`ScriptName`, '') <> 'go_aq_war_effort_gong')) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: reserved gong GUID collision';
    END IF;
    IF EXISTS (SELECT 1 FROM `gameobject` WHERE `id` = 180717 AND `guid` <> 9100717)
        OR EXISTS (SELECT 1 FROM `gameobject_queststarter` WHERE `quest` = 8743 AND `id` <> 180717)
        OR EXISTS (SELECT 1 FROM `gameobject_questender` WHERE `quest` = 8743 AND `id` <> 180717)
        OR EXISTS (SELECT 1 FROM `creature_queststarter` WHERE `quest` = 8743)
        OR EXISTS (SELECT 1 FROM `creature_questender` WHERE `quest` = 8743) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: another finale giver exists; resolve legacy installation first';
    END IF;
    IF EXISTS (SELECT 1 FROM `gameobject` WHERE `guid` = 49451 AND
        (`id` <> 180718 OR `map` <> 1 OR `spawnMask` <> 1 OR `phaseMask` <> 1
         OR ABS(`position_x` + 8069.05) > 0.01 OR ABS(`position_y` - 1641.72) > 0.01
         OR ABS(`position_z` - 27.03) > 0.01 OR ABS(`orientation` + 1.53589) > 0.001
         OR ABS(`rotation0`) > 0.00001 OR ABS(`rotation1`) > 0.00001
         OR ABS(`rotation2` + 0.694658) > 0.00001 OR ABS(`rotation3` - 0.71934) > 0.00001
         OR `spawntimesecs` <> 900 OR `animprogress` <> 100 OR `state` <> 1
         OR COALESCE(`ScriptName`, '') <> '')) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: decorative spawn 49451 customized; no replacement performed';
    END IF;
    IF EXISTS (SELECT 1 FROM `game_event_gameobject` WHERE `guid` IN (49451, 9100717))
        OR EXISTS (SELECT 1 FROM `pool_gameobject` WHERE `guid` IN (49451, 9100717)) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: gong linked to event/pool; review before installation';
    END IF;
    -- Replace only the checked decorative instance, never convert entry 180718's template.
    DELETE FROM `gameobject` WHERE `guid` = 49451;
    INSERT INTO `gameobject`
        (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`,
         `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`,
         `spawntimesecs`, `animprogress`, `state`, `ScriptName`, `VerifiedBuild`, `Comment`)
    VALUES
        (9100717, 180717, 1, 1, 1, -8069.05, 1641.72, 27.03, -1.53589, 0, 0, -0.694658, 0.71934,
         0, 100, 1, 'go_aq_war_effort_gong', 0, 'mod-aq-war-effort: Scarab Gong')
    ON DUPLICATE KEY UPDATE
        `spawnMask` = 1, `phaseMask` = 1,
        `position_x` = -8069.05, `position_y` = 1641.72, `position_z` = 27.03,
        `orientation` = -1.53589, `rotation0` = 0, `rotation1` = 0,
        `rotation2` = -0.694658, `rotation3` = 0.71934,
        `spawntimesecs` = 0, `animprogress` = 100, `state` = 1;
    INSERT INTO `gameobject_queststarter` (`id`, `quest`) VALUES (180717, 8743)
        ON DUPLICATE KEY UPDATE `quest` = VALUES(`quest`);
    INSERT INTO `gameobject_questender` (`id`, `quest`) VALUES (180717, 8743)
        ON DUPLICATE KEY UPDATE `quest` = VALUES(`quest`);
    COMMIT;
END$$
DELIMITER ;
CALL `aq_war_effort_restore_gong`();
DROP PROCEDURE IF EXISTS `aq_war_effort_restore_gong`;
