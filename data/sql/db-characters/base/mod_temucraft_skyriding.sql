-- mod-temucraft-travel: who has skyriding turned on (.temucraft skyriding).
CREATE TABLE IF NOT EXISTS `mod_temucraft_skyriding` (
  `guid` INT UNSIGNED NOT NULL,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
