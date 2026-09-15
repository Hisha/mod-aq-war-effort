-- Apply with worldserver stopped. Reapplying preserves pending and accepted attempts.
-- One worldserver owns each campaign ID; do not share an ID between running realms.
CREATE TABLE IF NOT EXISTS `aq_war_effort_gong` (
    `id` INT UNSIGNED NOT NULL,
    `player_guid` INT UNSIGNED NOT NULL,
    `ready_started_at` BIGINT UNSIGNED NOT NULL,
    `prepared_at` BIGINT UNSIGNED NOT NULL,
    `accepted` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
