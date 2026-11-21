-- mod-temucraft-travel: its player commands (all under .temucraft) as RBAC permissions 103350-103399. Linked to the stock player
-- role 195 and the Temucraft player role 1010, so every account has them by default. To take one away, remove its
-- link here (or deny it per account with .rbac account deny <account> <id>); to give it to staff only, link it to
-- 1011-1014 instead.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 103350 AND 103399;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 103350 AND 103399;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(103350, 'Command: temucraft hearthstone'),
(103351, 'Temucraft: use Skyreader''s Atlas'),
(103352, 'Temucraft: use the Hearthbound and Fel Waygates'),
(103353, 'Temucraft: use Wayfarer''s Beacon');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(195, 103350),
(1010, 103350),
(195, 103351),
(1010, 103351),
(195, 103352),
(1010, 103352),
(195, 103353),
(1010, 103353);
