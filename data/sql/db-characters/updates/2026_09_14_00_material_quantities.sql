-- Stop worldserver and back up the character DB before applying this update.
-- Historical v1 counters are TURN-INS: all twelve non-leather columns x20,
-- all three leather columns x10, for BOTH factions and EVERY campaign ID.
-- These are the historical storage multipliers, not configurable/current quest values.
-- Conversion and version advancement commit together. Re-execution cannot multiply twice.
CREATE TABLE IF NOT EXISTS `aq_war_effort_schema` (
    `id` TINYINT UNSIGNED NOT NULL,
    `material_data_version` INT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `aq_war_effort_schema` (`id`, `material_data_version`)
VALUES (1, 1) ON DUPLICATE KEY UPDATE `id` = `id`;

DROP PROCEDURE IF EXISTS `aq_war_effort_migrate_quantities`;
DELIMITER $$
CREATE PROCEDURE `aq_war_effort_migrate_quantities`()
BEGIN
    DECLARE current_version INT UNSIGNED;
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        RESIGNAL;
    END;

    START TRANSACTION;
    SELECT `material_data_version` INTO current_version
    FROM `aq_war_effort_schema` WHERE `id` = 1 FOR UPDATE;
    IF current_version NOT IN (1, 2) THEN
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: unsupported material data version; no counters changed';
    END IF;
    IF current_version = 1 THEN
        -- Explicit overflow guard also protects sessions using a permissive SQL mode.
        IF EXISTS (SELECT 1 FROM `aq_war_effort` WHERE
            `bandages01` > 922337203685477580
            OR `bandages02` > 922337203685477580
            OR `bandages03` > 922337203685477580
            OR `food01` > 922337203685477580
            OR `food02` > 922337203685477580
            OR `food03` > 922337203685477580
            OR `herbs01` > 922337203685477580
            OR `herbs02` > 922337203685477580
            OR `herbs03` > 922337203685477580
            OR `metals01` > 922337203685477580
            OR `metals02` > 922337203685477580
            OR `metals03` > 922337203685477580
            OR `leather01` > 1844674407370955161
            OR `leather02` > 1844674407370955161
            OR `leather03` > 1844674407370955161) THEN
            SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'AQWarEffort: counter conversion would overflow; no counters changed';
        END IF;
        UPDATE `aq_war_effort` SET
            `bandages01` = `bandages01` * 20,
            `bandages02` = `bandages02` * 20,
            `bandages03` = `bandages03` * 20,
            `food01` = `food01` * 20,
            `food02` = `food02` * 20,
            `food03` = `food03` * 20,
            `herbs01` = `herbs01` * 20,
            `herbs02` = `herbs02` * 20,
            `herbs03` = `herbs03` * 20,
            `metals01` = `metals01` * 20,
            `metals02` = `metals02` * 20,
            `metals03` = `metals03` * 20,
            `leather01` = `leather01` * 10,
            `leather02` = `leather02` * 10,
            `leather03` = `leather03` * 10;
        UPDATE `aq_war_effort_schema` SET `material_data_version` = 2 WHERE `id` = 1;
    END IF;
    COMMIT;
END$$
DELIMITER ;
CALL `aq_war_effort_migrate_quantities`();
DROP PROCEDURE IF EXISTS `aq_war_effort_migrate_quantities`;
