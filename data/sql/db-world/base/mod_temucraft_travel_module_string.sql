-- mod-temucraft-travel: player-facing texts. Translations go in module_string_locale (same module and id, plus the locale).
DELETE FROM `module_string` WHERE `module` = 'mod-temucraft-travel';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-temucraft-travel', 1, '|cffffcc00{}|r'),
('mod-temucraft-travel', 2, '|cffffcc00Hearthstone:|r cooldown set to {} (from its next use).'),
('mod-temucraft-travel', 3, '|cffffcc00Hearthstone:|r cooldown {}. Choose with .temucraft hearthstone <minutes>: {}.'),
('mod-temucraft-travel', 4, 'That cooldown isn''t one of the choices.'),
('mod-temucraft-travel', 5, 'This server decides that for everyone.'),
('mod-temucraft-travel', 6, 'Hearthstone cooldown ({}).'),
('mod-temucraft-travel', 7, '{}{}'),
('mod-temucraft-travel', 8, 'no cooldown'),
('mod-temucraft-travel', 9, '{} minutes'),
('mod-temucraft-travel', 10, 'Hearthstone cooldowns can''t be changed on this server.'),
('mod-temucraft-travel', 40, 'Leave {} and fly.'),
('mod-temucraft-travel', 41, 'Flying will remove your {}. Continue?'),
('mod-temucraft-travel', 42, 'Dismount and fly.'),
('mod-temucraft-travel', 43, 'Flying will dismount you. Continue?'),
('mod-temucraft-travel', 44, 'Fly in Flight Form ({}% of the price).'),
('mod-temucraft-travel', 60, 'You can''t use this here.'),
('mod-temucraft-travel', 61, '|cff66ccffSkyreader''s Atlas:|r {} new flight paths learned.'),
('mod-temucraft-travel', 62, '|cff66ccffSkyreader''s Atlas:|r you know every flight path there already.');
