-- mod-temucraft-travel: the Cozy Sleeping Bag comes from a short quest chain at any innkeeper (level 14+),
-- instead of being sold. Re-runnable.
SET @Bag := 601620;

-- Sleeping bag chain: quests 602100-602101
DELETE FROM `quest_template` WHERE `ID` IN (602100, 602101);
DELETE FROM `quest_template_addon` WHERE `ID` IN (602100, 602101);
DELETE FROM `creature_queststarter` WHERE `quest` IN (602100, 602101);
DELETE FROM `creature_questender` WHERE `quest` IN (602100, 602101);
INSERT INTO `quest_template` (`ID`, `QuestType`, `QuestLevel`, `MinLevel`, `RewardMoney`, `Flags`, `RewardItem1`, `RewardAmount1`,
    `LogTitle`, `LogDescription`, `QuestDescription`, `QuestCompletionLog`, `RequiredItemId1`, `RequiredItemCount1`,
    `RequiredItemId2`, `RequiredItemCount2`, `VerifiedBuild`) VALUES
(602100, 2, 16, 14, 500, 0, 0, 0, 'Comforts of the Road',
    'Bring 8 Wool Cloth to an innkeeper.',
    'You look like you sleep on rocks, $c. Out there in the wilds, a good night''s rest is worth more than gold.$B$BI can stitch you a proper bedroll, the kind that keeps the cold out. Bring me some wool for the lining and we''ll get started.',
    'Return to an innkeeper.', 2592, 8, 0, 0, 0),
(602101, 2, 16, 14, 0, 0, @Bag, 1, 'A Cozy Bedroll',
    'Bring 4 Light Leather and 2 Coarse Thread to an innkeeper.',
    'The lining''s done. Now it needs a tough outer shell that won''t soak through, and thread to hold it all together.$B$BBring me the leather and the thread and the bedroll is yours. Lay it down anywhere outdoors and sleep in it: you''ll wake up sharper, and well rested.',
    'Return to an innkeeper.', 2318, 4, 2320, 2, 0);
INSERT INTO `quest_template_addon` (`ID`, `PrevQuestID`) VALUES (602100, 0), (602101, 602100);
-- every innkeeper gives and takes them
UPDATE `creature_template` SET `npcflag` = `npcflag` | 2 WHERE (`npcflag` & 65536) <> 0;
INSERT INTO `creature_queststarter` (`id`, `quest`)
SELECT ct.`entry`, q.`id` FROM `creature_template` ct JOIN (SELECT 602100 AS `id` UNION ALL SELECT 602101) q
WHERE (ct.`npcflag` & 65536) <> 0;
INSERT INTO `creature_questender` (`id`, `quest`)
SELECT ct.`entry`, q.`id` FROM `creature_template` ct JOIN (SELECT 602100 AS `id` UNION ALL SELECT 602101) q
WHERE (ct.`npcflag` & 65536) <> 0;
-- no longer sold
DELETE FROM `npc_vendor` WHERE `item` = @Bag;

