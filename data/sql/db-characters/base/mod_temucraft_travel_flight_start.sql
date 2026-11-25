-- mod-temucraft-travel: characters that have been given their starting flight points (race and faction capitals).
CREATE TABLE IF NOT EXISTS `mod_temucraft_travel_flight_start` (
  `guid` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
