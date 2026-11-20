-- mod-temucraft-travel: Stones and Scrolls of Homecoming. Each one takes you to a capital (10 second cast, like a
-- hearthstone; not in combat, dungeons, raids, battlegrounds or arenas). Stones: 15 minute cooldown shared by every
-- stone. Scrolls: 24 hour cooldown shared by every scroll. Neither shares the hearthstone's cooldown. Both are kept
-- (not used up). Alliance and Horde ones are for their own faction only (FlagsExtra 2 / 1, so every race of the
-- faction, custom races too); Dalaran and Shattrath are for both. Sold by every innkeeper who is also a vendor:
-- a player only sees the ones of their faction. The spells and their cooldown groups are in mod_temucraft_travel_dbc.sql.
-- Re-runnable.
SET @StoneItem := 610030, @ScrollItem := 610040;   -- 610030-610039 stones, 610040-610049 scrolls
SET @StoneSpell := 97151, @ScrollSpell := 97161; -- same order as the items

DELETE FROM `item_template` WHERE `entry` BETWEEN @StoneItem AND @StoneItem+9 OR `entry` BETWEEN @ScrollItem AND @ScrollItem+9;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`, `FlagsExtra`,
    `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `maxcount`,
    `stackable`, `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`,
    `bonding`, `description`, `Material`, `VerifiedBuild`) VALUES
(@StoneItem+0, 15, 0, -1, 'Stone of Homecoming: Stormwind', 20984, 1, 64, 2, 1, 100000, 0, 0, -1, -1, 1, 1, 1, 1, @StoneSpell+0, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+1, 15, 0, -1, 'Stone of Homecoming: Ironforge', 20984, 1, 64, 2, 1, 100000, 0, 0, -1, -1, 1, 1, 1, 1, @StoneSpell+1, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+2, 15, 0, -1, 'Stone of Homecoming: Darnassus', 20984, 1, 64, 2, 1, 100000, 0, 0, -1, -1, 1, 1, 1, 1, @StoneSpell+2, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+3, 15, 0, -1, 'Stone of Homecoming: The Exodar', 20984, 1, 64, 2, 1, 100000, 0, 0, -1, -1, 1, 1, 1, 1, @StoneSpell+3, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+4, 15, 0, -1, 'Stone of Homecoming: Orgrimmar', 20984, 1, 64, 1, 1, 100000, 0, 0, -1, -1, 1, 1, 1, 1, @StoneSpell+4, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+5, 15, 0, -1, 'Stone of Homecoming: Undercity', 20984, 1, 64, 1, 1, 100000, 0, 0, -1, -1, 1, 1, 1, 1, @StoneSpell+5, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+6, 15, 0, -1, 'Stone of Homecoming: Thunder Bluff', 20984, 1, 64, 1, 1, 100000, 0, 0, -1, -1, 1, 1, 1, 1, @StoneSpell+6, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+7, 15, 0, -1, 'Stone of Homecoming: Silvermoon', 20984, 1, 64, 1, 1, 100000, 0, 0, -1, -1, 1, 1, 1, 1, @StoneSpell+7, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+8, 15, 0, -1, 'Stone of Homecoming: Dalaran', 20984, 1, 64, 0, 1, 100000, 0, 0, -1, -1, 1, 68, 1, 1, @StoneSpell+8, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@StoneItem+9, 15, 0, -1, 'Stone of Homecoming: Shattrath', 20984, 1, 64, 0, 1, 100000, 0, 0, -1, -1, 1, 58, 1, 1, @StoneSpell+9, 0, 0, -1, 0, -1, 1, 'Smooth and warm. It always knows the way home.', -1, 0),
(@ScrollItem+0, 15, 0, -1, 'Scroll of Homecoming: Stormwind', 1097, 1, 64, 2, 1, 10000, 0, 0, -1, -1, 1, 1, 1, 1, @ScrollSpell+0, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+1, 15, 0, -1, 'Scroll of Homecoming: Ironforge', 1097, 1, 64, 2, 1, 10000, 0, 0, -1, -1, 1, 1, 1, 1, @ScrollSpell+1, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+2, 15, 0, -1, 'Scroll of Homecoming: Darnassus', 1097, 1, 64, 2, 1, 10000, 0, 0, -1, -1, 1, 1, 1, 1, @ScrollSpell+2, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+3, 15, 0, -1, 'Scroll of Homecoming: The Exodar', 1097, 1, 64, 2, 1, 10000, 0, 0, -1, -1, 1, 1, 1, 1, @ScrollSpell+3, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+4, 15, 0, -1, 'Scroll of Homecoming: Orgrimmar', 1097, 1, 64, 1, 1, 10000, 0, 0, -1, -1, 1, 1, 1, 1, @ScrollSpell+4, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+5, 15, 0, -1, 'Scroll of Homecoming: Undercity', 1097, 1, 64, 1, 1, 10000, 0, 0, -1, -1, 1, 1, 1, 1, @ScrollSpell+5, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+6, 15, 0, -1, 'Scroll of Homecoming: Thunder Bluff', 1097, 1, 64, 1, 1, 10000, 0, 0, -1, -1, 1, 1, 1, 1, @ScrollSpell+6, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+7, 15, 0, -1, 'Scroll of Homecoming: Silvermoon', 1097, 1, 64, 1, 1, 10000, 0, 0, -1, -1, 1, 1, 1, 1, @ScrollSpell+7, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+8, 15, 0, -1, 'Scroll of Homecoming: Dalaran', 1097, 1, 64, 0, 1, 10000, 0, 0, -1, -1, 1, 68, 1, 1, @ScrollSpell+8, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0),
(@ScrollItem+9, 15, 0, -1, 'Scroll of Homecoming: Shattrath', 1097, 1, 64, 0, 1, 10000, 0, 0, -1, -1, 1, 58, 1, 1, @ScrollSpell+9, 0, 0, -1, 0, -1, 1, 'The ink still smells of the city.', -1, 0);

-- Where each one takes you: in front of the bank or at the heart of the city, facing in
DELETE FROM `spell_target_position` WHERE `ID` BETWEEN @StoneSpell AND @StoneSpell+9 OR `ID` BETWEEN @ScrollSpell AND @ScrollSpell+9;
INSERT INTO `spell_target_position` (`ID`, `EffectIndex`, `MapID`, `PositionX`, `PositionY`, `PositionZ`, `Orientation`, `VerifiedBuild`) VALUES
(@StoneSpell+0, 0, 0, -8833.38, 628.63, 94.01, 1.07, 0),
(@StoneSpell+1, 0, 0, -4918.88, -940.41, 501.56, 5.42, 0),
(@StoneSpell+2, 0, 1, 9949.56, 2284.21, 1341.40, 1.60, 0),
(@StoneSpell+3, 0, 530, -3965.70, -11653.60, -138.84, 0.85, 0),
(@StoneSpell+4, 0, 1, 1616.80, -4378.20, 13.00, 3.55, 0),
(@StoneSpell+5, 0, 0, 1584.14, 240.31, -52.15, 0.04, 0),
(@StoneSpell+6, 0, 1, -1272.00, 40.00, 129.30, 5.40, 0),
(@StoneSpell+7, 0, 530, 9487.69, -7279.20, 14.29, 6.16, 0),
(@StoneSpell+8, 0, 571, 5807.98, 588.49, 660.94, 1.67, 0),
(@StoneSpell+9, 0, 530, -1838.16, 5301.79, -12.43, 5.95, 0),
(@ScrollSpell+0, 0, 0, -8833.38, 628.63, 94.01, 1.07, 0),
(@ScrollSpell+1, 0, 0, -4918.88, -940.41, 501.56, 5.42, 0),
(@ScrollSpell+2, 0, 1, 9949.56, 2284.21, 1341.40, 1.60, 0),
(@ScrollSpell+3, 0, 530, -3965.70, -11653.60, -138.84, 0.85, 0),
(@ScrollSpell+4, 0, 1, 1616.80, -4378.20, 13.00, 3.55, 0),
(@ScrollSpell+5, 0, 0, 1584.14, 240.31, -52.15, 0.04, 0),
(@ScrollSpell+6, 0, 1, -1272.00, 40.00, 129.30, 5.40, 0),
(@ScrollSpell+7, 0, 530, 9487.69, -7279.20, 14.29, 6.16, 0),
(@ScrollSpell+8, 0, 571, 5807.98, 588.49, 660.94, 1.67, 0),
(@ScrollSpell+9, 0, 530, -1838.16, 5301.79, -12.43, 5.95, 0);

-- The script keeps them out of dungeons, raids, battlegrounds and arenas (src/TemucraftHomecoming.cpp)
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_temucraft_homecoming';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(@StoneSpell+0, 'spell_temucraft_homecoming'),
(@StoneSpell+1, 'spell_temucraft_homecoming'),
(@StoneSpell+2, 'spell_temucraft_homecoming'),
(@StoneSpell+3, 'spell_temucraft_homecoming'),
(@StoneSpell+4, 'spell_temucraft_homecoming'),
(@StoneSpell+5, 'spell_temucraft_homecoming'),
(@StoneSpell+6, 'spell_temucraft_homecoming'),
(@StoneSpell+7, 'spell_temucraft_homecoming'),
(@StoneSpell+8, 'spell_temucraft_homecoming'),
(@StoneSpell+9, 'spell_temucraft_homecoming'),
(@ScrollSpell+0, 'spell_temucraft_homecoming'),
(@ScrollSpell+1, 'spell_temucraft_homecoming'),
(@ScrollSpell+2, 'spell_temucraft_homecoming'),
(@ScrollSpell+3, 'spell_temucraft_homecoming'),
(@ScrollSpell+4, 'spell_temucraft_homecoming'),
(@ScrollSpell+5, 'spell_temucraft_homecoming'),
(@ScrollSpell+6, 'spell_temucraft_homecoming'),
(@ScrollSpell+7, 'spell_temucraft_homecoming'),
(@ScrollSpell+8, 'spell_temucraft_homecoming'),
(@ScrollSpell+9, 'spell_temucraft_homecoming');

-- Sold by innkeepers who are also vendors (a player only sees their own faction's and the neutral ones)
DELETE FROM `npc_vendor` WHERE `item` BETWEEN @StoneItem AND @StoneItem+9 OR `item` BETWEEN @ScrollItem AND @ScrollItem+9;
INSERT IGNORE INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`)
SELECT ct.`entry`, 0, i.`item`, 0, 0, 0, 0
FROM `creature_template` ct
JOIN (SELECT @StoneItem+0 AS `item` UNION ALL SELECT @StoneItem+1 UNION ALL SELECT @StoneItem+2 UNION ALL SELECT @StoneItem+3 UNION ALL SELECT @StoneItem+4 UNION ALL SELECT @StoneItem+5 UNION ALL SELECT @StoneItem+6 UNION ALL SELECT @StoneItem+7 UNION ALL SELECT @StoneItem+8 UNION ALL SELECT @StoneItem+9 UNION ALL SELECT @ScrollItem+0 UNION ALL SELECT @ScrollItem+1 UNION ALL SELECT @ScrollItem+2 UNION ALL SELECT @ScrollItem+3 UNION ALL SELECT @ScrollItem+4 UNION ALL SELECT @ScrollItem+5 UNION ALL SELECT @ScrollItem+6 UNION ALL SELECT @ScrollItem+7 UNION ALL SELECT @ScrollItem+8 UNION ALL SELECT @ScrollItem+9) i
WHERE (ct.`npcflag` & 65536) AND (ct.`npcflag` & 128);

-- A faction change swaps them for the other side's (Stormwind <> Orgrimmar, Ironforge <> Undercity,
-- Darnassus <> Thunder Bluff, The Exodar <> Silvermoon)
DELETE FROM `player_factionchange_items` WHERE `alliance_id` BETWEEN @StoneItem AND @StoneItem+9 OR `alliance_id` BETWEEN @ScrollItem AND @ScrollItem+9;
INSERT INTO `player_factionchange_items` (`alliance_id`, `alliance_comment`, `horde_id`, `horde_comment`) VALUES
(@StoneItem+0, 'Stone of Homecoming: Stormwind', @StoneItem+4, 'Stone of Homecoming: Orgrimmar'),
(@StoneItem+1, 'Stone of Homecoming: Ironforge', @StoneItem+5, 'Stone of Homecoming: Undercity'),
(@StoneItem+2, 'Stone of Homecoming: Darnassus', @StoneItem+6, 'Stone of Homecoming: Thunder Bluff'),
(@StoneItem+3, 'Stone of Homecoming: The Exodar', @StoneItem+7, 'Stone of Homecoming: Silvermoon'),
(@ScrollItem+0, 'Scroll of Homecoming: Stormwind', @ScrollItem+4, 'Scroll of Homecoming: Orgrimmar'),
(@ScrollItem+1, 'Scroll of Homecoming: Ironforge', @ScrollItem+5, 'Scroll of Homecoming: Undercity'),
(@ScrollItem+2, 'Scroll of Homecoming: Darnassus', @ScrollItem+6, 'Scroll of Homecoming: Thunder Bluff'),
(@ScrollItem+3, 'Scroll of Homecoming: The Exodar', @ScrollItem+7, 'Scroll of Homecoming: Silvermoon');

-- Vanity items: what mod-temucraft-account-wide shares across the account and lets you summon again from the
-- Vanity window. The table is made here too, so this module still works alone.
CREATE TABLE IF NOT EXISTS `mod_temucraft_vanity_item` (
  `item` int unsigned NOT NULL COMMENT 'item_template.entry',
  `category` varchar(32) NOT NULL DEFAULT '' COMMENT 'heading in the Vanity window',
  `sort` smallint unsigned NOT NULL DEFAULT 0 COMMENT 'order inside its heading',
  PRIMARY KEY (`item`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Temucraft: vanity items (account collections)';
DELETE FROM `mod_temucraft_vanity_item` WHERE `item` BETWEEN @StoneItem AND @StoneItem+9 OR `item` BETWEEN @ScrollItem AND @ScrollItem+9;
INSERT INTO `mod_temucraft_vanity_item` (`item`, `category`, `sort`) VALUES
(@StoneItem+0, 'Homecoming', 0),
(@StoneItem+1, 'Homecoming', 1),
(@StoneItem+2, 'Homecoming', 2),
(@StoneItem+3, 'Homecoming', 3),
(@StoneItem+4, 'Homecoming', 4),
(@StoneItem+5, 'Homecoming', 5),
(@StoneItem+6, 'Homecoming', 6),
(@StoneItem+7, 'Homecoming', 7),
(@StoneItem+8, 'Homecoming', 8),
(@StoneItem+9, 'Homecoming', 9),
(@ScrollItem+0, 'Homecoming', 10),
(@ScrollItem+1, 'Homecoming', 11),
(@ScrollItem+2, 'Homecoming', 12),
(@ScrollItem+3, 'Homecoming', 13),
(@ScrollItem+4, 'Homecoming', 14),
(@ScrollItem+5, 'Homecoming', 15),
(@ScrollItem+6, 'Homecoming', 16),
(@ScrollItem+7, 'Homecoming', 17),
(@ScrollItem+8, 'Homecoming', 18),
(@ScrollItem+9, 'Homecoming', 19);
