-- mod-aq-war-effort: fresh schema uses MATERIAL QUANTITIES (data version 2).
-- On an existing unversioned table, record version 1 WITHOUT relabeling its counters.
-- Existing installs must also apply updates/2026_09_14_00_material_quantities.sql.
-- Stop worldserver before applying base or update SQL.
CREATE TABLE IF NOT EXISTS `aq_war_effort_schema` (
    `id` TINYINT UNSIGNED NOT NULL,
    `material_data_version` INT UNSIGNED NOT NULL,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `aq_war_effort_schema` (`id`, `material_data_version`)
SELECT 1, IF(EXISTS (
    SELECT 1 FROM `information_schema`.`tables`
    WHERE `table_schema` = DATABASE() AND `table_name` = 'aq_war_effort'
), 1, 2)
ON DUPLICATE KEY UPDATE `id` = `id`;

-- mod-aq-war-effort: non-destructive canonical character schema.
-- Runtime initializes both faction rows and the campaign row for the configured ID.
CREATE TABLE IF NOT EXISTS `aq_war_effort` (
    `id` INT UNSIGNED NOT NULL,
    `faction` TINYINT UNSIGNED NOT NULL,
    `bandages01` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `bandages02` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `bandages03` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `food01` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `food02` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `food03` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `herbs01` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `herbs02` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `herbs03` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `metals01` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `metals02` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `metals03` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `leather01` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `leather02` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `leather03` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`, `faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `aq_war_effort_campaign` (
    `id` INT UNSIGNED NOT NULL,
    `phase` TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `phase_started_at` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `gong_rung_at` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `opened_at` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
