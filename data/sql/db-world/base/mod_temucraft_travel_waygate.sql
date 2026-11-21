-- mod-temucraft-travel: Hearthbound Waygate (610150), Fel Waygate (610151) and Wayfarer's Beacon (610152).
-- A waygate places a portal (608950 or 608951) for 1 minute: everyone who clicks it goes to their own hearthstone spot.
-- 30 minute cooldown of its own (the hearthstone cooldown is not touched); both waygates share it.
-- The beacon places a tall beam of light (608952) for 5 minutes, to mark a meeting place; 5 minute cooldown.
-- The item spells (36177 and 49375, stock dummy spells, never cast) only carry the cooldown. Script: src/TemucraftWaygate.cpp.
-- Sold on the Temucraft Curator's Helpers list (vendor 608011): waygates 200 tokens, beacon 50 tokens. Re-runnable.
SET @ITEM := 610150; SET @GO := 608950; SET @HELPERS := 608011;

DELETE FROM `gameobject_template` WHERE `entry` BETWEEN @GO AND @GO+2;
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`, `AIName`, `ScriptName`, `VerifiedBuild`) VALUES
(@GO+0, 10, 9011, 'Hearthbound Waygate', '', '', '', 1, '', 'go_temucraft_waygate', 0),
(@GO+1, 10, 4713, 'Fel Waygate', '', '', '', 0.2, '', 'go_temucraft_waygate', 0),
(@GO+2, 5, 3272, 'Wayfarer''s Beacon', '', '', '', 1, '', '', 0);

DELETE FROM `item_template` WHERE `entry` BETWEEN @ITEM AND @ITEM+2;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `ItemLevel`, `maxcount`, `stackable`, `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `bonding`, `description`, `Material`, `ScriptName`) VALUES
(@ITEM+0, 15, 0, -1, 'Hearthbound Waygate', 31767, 3, 0, 1, 0, 0, 0, 1, 1, 1, 36177, 0, 0, 1800000, 1, 'Opens a way home for 1 minute. Everyone who steps through goes to their own hearthstone spot.', 7, 'item_temucraft_waygate'),
(@ITEM+1, 15, 0, -1, 'Fel Waygate', 50537, 3, 0, 1, 0, 0, 0, 1, 1, 1, 36177, 0, 0, 1800000, 1, 'Tears open a fel portal home for 1 minute. Everyone who steps through goes to their own hearthstone spot.', 7, 'item_temucraft_waygate'),
(@ITEM+2, 15, 0, -1, 'Wayfarer''s Beacon', 2746, 2, 0, 1, 0, 0, 0, 1, 1, 1, 49375, 0, 0, 300000, 1, 'A tall beam of light marks the spot for 5 minutes. Meet your friends there.', 7, 'item_temucraft_waygate');

DELETE FROM `npc_vendor` WHERE `entry` = @HELPERS AND `item` BETWEEN @ITEM AND @ITEM+2;
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`) VALUES
(@HELPERS, 10, @ITEM+0, 0, 0, 3210, 0), (@HELPERS, 11, @ITEM+1, 0, 0, 3210, 0), (@HELPERS, 12, @ITEM+2, 0, 0, 3201, 0);

CREATE TABLE IF NOT EXISTS `mod_temucraft_vanity_item` (
  `item` int unsigned NOT NULL COMMENT 'item_template.entry',
  `category` varchar(32) NOT NULL DEFAULT '' COMMENT 'heading in the Vanity window',
  `sort` smallint unsigned NOT NULL DEFAULT 0 COMMENT 'order inside its heading',
  PRIMARY KEY (`item`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Temucraft: vanity items (account collections)';
DELETE FROM `mod_temucraft_vanity_item` WHERE `item` BETWEEN @ITEM AND @ITEM+2;
INSERT INTO `mod_temucraft_vanity_item` (`item`, `category`, `sort`) VALUES
(@ITEM+0, 'Travel', 40), (@ITEM+1, 'Travel', 41), (@ITEM+2, 'Toys', 10);
