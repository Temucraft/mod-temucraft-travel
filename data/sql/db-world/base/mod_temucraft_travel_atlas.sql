-- mod-temucraft-travel: Skyreader's Atlas, one per continent (610146 Eastern Kingdoms, 610147 Kalimdor, 610148 Outland,
-- 610149 Northrend). Sold on the Temucraft Curator's Helpers list (vendor 608011, 100 tokens each), in the Vanity
-- window under "Travel", and mailed for the Explore Eastern Kingdoms / Kalimdor / Outland achievements (42-44; 45
-- Explore Northrend keeps its stock tabard).
SET @ATLAS := 610146; SET @HELPERS := 608011; SET @SENDER := 10583;
DELETE FROM `item_template` WHERE `entry` BETWEEN @ATLAS AND @ATLAS+3;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `ItemLevel`, `maxcount`, `stackable`, `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `bonding`, `description`, `Material`, `ScriptName`) VALUES
(@ATLAS+0, 15, 0, -1, 'Skyreader''s Atlas: Eastern Kingdoms', 1322, 3, 0, 1, 0, 0, 0, 1, 1, 1, 18282, 0, 0, 1500, 1, 'Learn every flight path of your faction in the Eastern Kingdoms.', 7, 'item_temucraft_atlas'),
(@ATLAS+1, 15, 0, -1, 'Skyreader''s Atlas: Kalimdor', 1322, 3, 0, 1, 0, 0, 0, 1, 1, 1, 18282, 0, 0, 1500, 1, 'Learn every flight path of your faction in Kalimdor.', 7, 'item_temucraft_atlas'),
(@ATLAS+2, 15, 0, -1, 'Skyreader''s Atlas: Outland', 55351, 3, 0, 1, 0, 0, 0, 1, 1, 1, 18282, 0, 0, 1500, 1, 'Learn every flight path of your faction in Outland.', 7, 'item_temucraft_atlas'),
(@ATLAS+3, 15, 0, -1, 'Skyreader''s Atlas: Northrend', 55351, 3, 0, 1, 0, 0, 0, 1, 1, 1, 18282, 0, 0, 1500, 1, 'Learn every flight path of your faction in Northrend.', 7, 'item_temucraft_atlas');

DELETE FROM `npc_vendor` WHERE `entry` = @HELPERS AND `item` BETWEEN @ATLAS AND @ATLAS+3;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`) VALUES
(@HELPERS, 6, @ATLAS+0, 0, 0, 3205, 0), (@HELPERS, 7, @ATLAS+1, 0, 0, 3205, 0),
(@HELPERS, 8, @ATLAS+2, 0, 0, 3205, 0), (@HELPERS, 9, @ATLAS+3, 0, 0, 3205, 0);

CREATE TABLE IF NOT EXISTS `mod_temucraft_vanity_item` (
  `item` int unsigned NOT NULL COMMENT 'item_template.entry',
  `category` varchar(32) NOT NULL DEFAULT '' COMMENT 'heading in the Vanity window',
  `sort` smallint unsigned NOT NULL DEFAULT 0 COMMENT 'order inside its heading',
  PRIMARY KEY (`item`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Temucraft: vanity items (account collections)';
DELETE FROM `mod_temucraft_vanity_item` WHERE `item` BETWEEN @ATLAS AND @ATLAS+3;
INSERT INTO `mod_temucraft_vanity_item` (`item`, `category`, `sort`) VALUES
(@ATLAS+0, 'Travel', 20), (@ATLAS+1, 'Travel', 21), (@ATLAS+2, 'Travel', 22), (@ATLAS+3, 'Travel', 23);

-- achievement mail (only where the stock achievement has no reward of its own)
DELETE FROM `achievement_reward` WHERE `ID` IN (42, 43, 44) AND `ItemID` BETWEEN @ATLAS AND @ATLAS+3;
INSERT IGNORE INTO `achievement_reward` (`ID`, `TitleA`, `TitleH`, `ItemID`, `Sender`, `Subject`, `Body`, `MailTemplateID`) VALUES
(42, 0, 0, @ATLAS+0, @SENDER, 'The Eastern Kingdoms, charted', 'You have seen every corner of the Eastern Kingdoms, $n. This atlas holds every flight path there. Fly well.', 0),
(43, 0, 0, @ATLAS+1, @SENDER, 'Kalimdor, charted', 'You have seen every corner of Kalimdor, $n. This atlas holds every flight path there. Fly well.', 0),
(44, 0, 0, @ATLAS+2, @SENDER, 'Outland, charted', 'You have seen every corner of Outland, $n. This atlas holds every flight path there. Fly well.', 0);
