-- mod-temucraft-travel: its commands (all under .temucraft) as RBAC permissions 103350-103399. Every command is admin only by
-- default: linked to the administrator roles 192 and 1014. To give one to players, link it to 195 and 1010 here (or
-- grant it per account with .rbac account grant <account> <id>). "Temucraft: use ..." permissions are not
-- commands: they stay linked to the player roles so the items keep working.
DELETE FROM `rbac_linked_permissions` WHERE `linkedId` BETWEEN 103350 AND 103399;
DELETE FROM `rbac_permissions` WHERE `id` BETWEEN 103350 AND 103399;
INSERT INTO `rbac_permissions` (`id`, `name`) VALUES
(103350, 'Command: temucraft hearthstone'),
(103351, 'Temucraft: use Skyreader''s Atlas'),
(103352, 'Temucraft: use the Hearthbound and Fel Waygates'),
(103353, 'Temucraft: use Wayfarer''s Beacon'),
(103354, 'Command: temucraft skyriding');
INSERT INTO `rbac_linked_permissions` (`id`, `linkedId`) VALUES
(192, 103350),
(1014, 103350),
(195, 103351),
(1010, 103351),
(195, 103352),
(1010, 103352),
(195, 103353),
(1010, 103353),
(192, 103354),
(1014, 103354);
