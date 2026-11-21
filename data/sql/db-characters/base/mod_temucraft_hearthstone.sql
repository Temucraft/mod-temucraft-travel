-- mod-temucraft-travel: each character's hearthstone cooldown in minutes (no row = the server default).
CREATE TABLE IF NOT EXISTS `mod_temucraft_character_setting` (
  `guid` INT UNSIGNED NOT NULL COMMENT 'characters.guid',
  `setting` VARCHAR(32) NOT NULL COMMENT 'which choice (auras, autolearn, reagents, reagentbag, hearthstone, tankmode, shopmaxlevel)',
  `value` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`guid`, `setting`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Temucraft: small per-character choices of many modules (one row per choice)';
-- rows from the old `mod_temucraft_hearthstone` table, if this server still has it, then it goes
SET @old := (SELECT COUNT(*) FROM information_schema.TABLES WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'mod_temucraft_hearthstone');
SET @sql := IF(@old > 0, 'INSERT IGNORE INTO `mod_temucraft_character_setting` (`guid`, `setting`, `value`) SELECT `guid`, ''hearthstone'', `minutes` FROM `mod_temucraft_hearthstone`', 'DO 0');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
DROP TABLE IF EXISTS `mod_temucraft_hearthstone`;
