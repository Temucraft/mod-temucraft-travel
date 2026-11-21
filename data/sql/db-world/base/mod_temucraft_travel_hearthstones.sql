-- mod-temucraft-travel: hearthstone keepsakes. Each one is a hearthstone with its own look (the stock spells of
-- hearthstone category 1176, so they share the hearthstone's cooldown and its chosen length). Account-bound,
-- one each; sold by every innkeeper who is also a vendor. Spells 97180-97186 are ours (tools/build_keepsakes.py). The Ethereal Portal (54452) is the stock item.
DELETE FROM `item_template` WHERE `entry` BETWEEN 610020 AND 610029;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`,
    `InventoryType`, `ItemLevel`, `RequiredLevel`, `maxcount`, `stackable`, `bonding`, `spellid_1`, `spelltrigger_1`, `spellcooldown_1`,
    `spellcategorycooldown_1`, `description`, `Material`, `VerifiedBuild`) VALUES
(610020, 15, 0, 'Pocket Portal',          6418, 7, 134217792, 1, 100000, 0, 0, 1, 1, 1, 1, 1, 70195,  0, -1, -1, 'A hole you can fold up and keep. It always opens at home.', -1, -1811),
(610021, 15, 0, 'Innkeeper''s Keepsake',  6418, 7, 134217792, 1, 100000, 0, 0, 1, 1, 1, 1, 1, 54401,  0, -1, -1, 'A token from the innkeeper''s family: there''s always a room for you.', -1, -1811),
(610022, 15, 0, 'Shard of the Dark Portal', 6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 97180, 0, -1, -1, 'Still humming with the portal''s fel light.', -1, -1811),
(610023, 15, 0, 'Lich-touched Stone',     6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 54403,  0, -1, -1, 'Cold to the touch. It remembers where you sleep.', -1, -1811),
(610024, 15, 0, 'Arcanist''s Waystone', 6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 97181, 0, -1, -1, 'Etched with the runes the magi use. Home is one word away.', -1, -1811),
(610025, 15, 0, 'Death Gate Shard', 6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 97182, 0, -1, -1, 'A sliver of the gate the knights of the Ebon Blade open.', -1, -1811),
(610026, 15, 0, 'Ember of the Imp', 6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 97183, 0, -1, -1, 'Still warm. Something small and rude lives inside.', -1, -1811),
(610027, 15, 0, 'Voidtouched Stone', 6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 97184, 0, -1, -1, 'Darker than it should be, and it hums when you think of home.', -1, -1811),
(610028, 15, 0, 'Lightbound Stone', 6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 97185, 0, -1, -1, 'A stone that glows softly with holy light.', -1, -1811),
(610029, 15, 0, 'Spiritwalker''s Stone', 6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 97186, 0, -1, -1, 'The spirits know the way home, even when you don''t.', -1, -1811);

-- sold by innkeepers that are vendors (and the Ethereal Portal too)
DELETE FROM `npc_vendor` WHERE (`item` BETWEEN 610020 AND 610029 OR `item` = 54452) AND `VerifiedBuild` = -1811;
INSERT IGNORE INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`)
SELECT ct.`entry`, 0, i.`item`, 0, 0, 0, -1811
FROM `creature_template` ct
JOIN (SELECT 610020 AS `item` UNION ALL SELECT 610021 UNION ALL SELECT 610022 UNION ALL SELECT 610023 UNION ALL SELECT 610024
      UNION ALL SELECT 610025 UNION ALL SELECT 610026 UNION ALL SELECT 610027 UNION ALL SELECT 610028 UNION ALL SELECT 610029 UNION ALL SELECT 54452) i
WHERE (ct.`npcflag` & 65536) AND (ct.`npcflag` & 128);
UPDATE `item_template` SET `BuyPrice` = 250000 WHERE `entry` = 54452 AND `BuyPrice` = 0;

-- the Vanity window (mod-temucraft-account-wide): every keepsake under "Travel"
CREATE TABLE IF NOT EXISTS `mod_temucraft_vanity_item` (
  `item` int unsigned NOT NULL COMMENT 'item_template.entry',
  `category` varchar(32) NOT NULL DEFAULT '' COMMENT 'heading in the Vanity window',
  `sort` smallint unsigned NOT NULL DEFAULT 0 COMMENT 'order inside its heading',
  PRIMARY KEY (`item`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Temucraft: vanity items (account collections)';
DELETE FROM `mod_temucraft_vanity_item` WHERE `item` BETWEEN 610020 AND 610029 OR `item` = 54452;
INSERT INTO `mod_temucraft_vanity_item` (`item`, `category`, `sort`)
SELECT `entry`, 'Travel', `entry` - 610000 FROM `item_template` WHERE `entry` BETWEEN 610020 AND 610029
UNION ALL SELECT 54452, 'Travel', 30;
