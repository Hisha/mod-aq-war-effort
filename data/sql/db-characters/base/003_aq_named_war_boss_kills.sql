-- Named Ten Hour War bosses: one durable death per campaign, war epoch and boss.
-- Historical rows are retained when a later war starts.
CREATE TABLE IF NOT EXISTS `aq_war_effort_boss_kill` (
    `campaign_id` INT UNSIGNED NOT NULL,
    `war_started_at` BIGINT UNSIGNED NOT NULL,
    `boss_id` INT UNSIGNED NOT NULL,
    `killed_at` BIGINT UNSIGNED NOT NULL,
    PRIMARY KEY (`campaign_id`, `war_started_at`, `boss_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
