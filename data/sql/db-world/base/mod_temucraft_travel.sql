-- mod-temucraft-travel: repair kits and the Cozy Sleeping Bag. Re-runnable (deletes its rows first).
SET @KitField    := 601621;
SET @KitReinf    := 601622;
SET @KitMaster   := 601623;
SET @BagItem     := 601620;
SET @BagObject   := 601620;
SET @DummySpell  := 18282; -- the client needs a spell to use an item; the module handles the use

DELETE FROM `item_template` WHERE `entry` IN (@KitField, @KitReinf, @KitMaster, @BagItem);
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`,
    `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `ItemLevel`, `RequiredLevel`, `maxcount`, `stackable`,
    `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`,
    `bonding`, `description`, `Material`, `ScriptName`, `VerifiedBuild`) VALUES
(@KitField,  0, 8, -1, 'Field Repair Kit',      7450,  1, 1,   5000,  1250, 0, 20, 5,  0, 20, @DummySpell, 0, 0, 1000, 0, 0, 0,
    'Mends all equipped gear of item level 100 or lower by 25% of its durability.', -1, 'item_temucraft_repair_kit', 0),
(@KitReinf,  0, 8, -1, 'Reinforced Repair Kit', 13692, 2, 1,  25000,  6250, 0, 45, 30, 0, 20, @DummySpell, 0, 0, 1000, 0, 0, 0,
    'Mends all equipped gear of item level 200 or lower by 50% of its durability.', -1, 'item_temucraft_repair_kit', 0),
(@KitMaster, 0, 8, -1, 'Master Repair Kit',     22924, 3, 1, 100000, 25000, 0, 70, 60, 0, 20, @DummySpell, 0, 0, 1000, 0, 0, 0,
    'Mends all equipped gear completely.', -1, 'item_temucraft_repair_kit', 0),
(@BagItem,  15, 0, -1, 'Cozy Sleeping Bag',     1625,  2, 1,  50000,     0, 0, 40, 14, 1, 1, @DummySpell, 0, 0, 300000, 0, 0, 1,
    'Lay it down outdoors and sleep in it: each minute gives +1% experience (up to 3%) for an hour, and some rested experience.', -1, 'item_temucraft_sleeping_bag', 0);

-- The bedroll: a clickable object (goober) the script handles
DELETE FROM `gameobject_template` WHERE `entry` = @BagObject;
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`,
    `Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, `Data6`, `Data7`, `Data8`, `Data9`, `Data10`, `Data11`,
    `Data12`, `Data13`, `Data14`, `Data15`, `Data16`, `Data17`, `Data18`, `Data19`, `Data20`, `Data21`, `Data22`,
    `Data23`, `AIName`, `ScriptName`, `VerifiedBuild`) VALUES
(@BagObject, 10, 8700, 'Cozy Sleeping Bag', 'Interact', '', '', 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '', 'go_temucraft_sleeping_bag', 0);

-- Where to get them: repair kits from blacksmithing and general goods vendors
DELETE FROM `npc_vendor` WHERE `item` IN (@KitField, @KitReinf, @KitMaster, @BagItem);
INSERT IGNORE INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`)
SELECT ct.`entry`, 0, k.`item`, 0, 0, 0, 0
FROM `creature_template` ct
JOIN (SELECT @KitField AS `item` UNION ALL SELECT @KitReinf UNION ALL SELECT @KitMaster) k
WHERE (ct.`npcflag` & 128) <> 0 AND (ct.`subname` LIKE '%Blacksmith%' OR ct.`subname` LIKE '%General Goods%'
    OR ct.`subname` LIKE '%General Supplies%');
-- The sleeping bag comes from the innkeepers' quest chain (mod_temucraft_travel_sleeping_bag.sql)
