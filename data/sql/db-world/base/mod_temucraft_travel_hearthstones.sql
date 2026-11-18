-- mod-temucraft-travel: hearthstone keepsakes. Each one is a hearthstone with its own look (the stock spells of
-- hearthstone category 1176, so they share the hearthstone's cooldown and its chosen length). Account-bound,
-- one each; sold by every innkeeper who is also a vendor. The Ethereal Portal (54452) is the stock item.
DELETE FROM `item_template` WHERE `entry` BETWEEN 610020 AND 610029;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`,
    `InventoryType`, `ItemLevel`, `RequiredLevel`, `maxcount`, `stackable`, `bonding`, `spellid_1`, `spelltrigger_1`, `spellcooldown_1`,
    `spellcategorycooldown_1`, `description`, `Material`, `VerifiedBuild`) VALUES
(610020, 15, 0, 'Pocket Portal',          6418, 7, 134217792, 1, 100000, 0, 0, 1, 1, 1, 1, 1, 70195,  0, -1, -1, 'A hole you can fold up and keep. It always opens at home.', -1, -1811),
(610021, 15, 0, 'Innkeeper''s Keepsake',  6418, 7, 134217792, 1, 100000, 0, 0, 1, 1, 1, 1, 1, 94719,  0, -1, -1, 'A token from the innkeeper''s family: there''s always a room for you.', -1, -1811),
(610022, 15, 0, 'Shard of the Dark Portal', 6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 136508, 0, -1, -1, 'Still humming with the portal''s fel light.', -1, -1811),
(610023, 15, 0, 'Lich-touched Stone',     6418, 7, 134217792, 1, 250000, 0, 0, 1, 1, 1, 1, 1, 54403,  0, -1, -1, 'Cold to the touch. It remembers where you sleep.', -1, -1811);

-- sold by innkeepers that are vendors (and the Ethereal Portal too)
DELETE FROM `npc_vendor` WHERE `item` IN (610020, 610021, 610022, 610023, 54452) AND `VerifiedBuild` = -1811;
INSERT IGNORE INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`)
SELECT ct.`entry`, 0, i.`item`, 0, 0, 0, -1811
FROM `creature_template` ct
JOIN (SELECT 610020 AS `item` UNION ALL SELECT 610021 UNION ALL SELECT 610022 UNION ALL SELECT 610023 UNION ALL SELECT 54452) i
WHERE (ct.`npcflag` & 65536) AND (ct.`npcflag` & 128);
UPDATE `item_template` SET `BuyPrice` = 250000 WHERE `entry` = 54452 AND `BuyPrice` = 0;
