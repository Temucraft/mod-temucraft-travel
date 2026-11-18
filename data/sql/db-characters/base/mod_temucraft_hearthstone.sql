-- mod-temucraft-travel: each character's hearthstone cooldown in minutes (no row = the server default).
CREATE TABLE IF NOT EXISTS `mod_temucraft_hearthstone` (
    `guid`    INT UNSIGNED NOT NULL,
    `minutes` INT UNSIGNED NOT NULL DEFAULT 30,
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
