# mod-temucraft-travel

Repair kits, the Cozy Sleeping Bag and a hearthstone cooldown of your choice.

- **Repair kits:** Field (25% durability, item level 100 and below), Reinforced (50%, 200), Master (100%, any). Sold by blacksmithing and general goods vendors.
- **Cozy Sleeping Bag:** lay it down outdoors and sleep in it: +1% experience per minute (up to 3%) for an hour, and some rested experience. It comes from a two-quest chain at any innkeeper (level 14+): 8 Wool Cloth, then 4 Light Leather and 2 Coarse Thread.

- **Hearthstone cooldown:** each player picks theirs at any innkeeper ("Hearthstone cooldown") or with
  `.temucraft hearthstone <minutes>`: no cooldown, 15, 30 (the game's own, and the default), 45 or 60 minutes. It applies from
  the next use, to the Hearthstone and every item that shares its cooldown (The Innkeeper's Daughter, Ethereal
  Portal...). The choices and the default are in the config. The tooltip still says 30 minutes; the cooldown shown on
  the button is the chosen one.
- **Safe landing:** fall through the world (a Charge into a gap in the map, a bad teleport) and you are put back on
  the last ground you stood on, instead of dying in the void; open world only (`Temucraft.Travel.SafeLanding`).
- **Swift flights:** flight paths go faster with a flight speed bonus: the guild perk Ride Like the Wind (+25%), the
  Legacy perk Swift Flights, or any spell you list (`Temucraft.Travel.SwiftFlights.Spells`), on top of the server's
  flight speed. Other modules can add a bonus through `src/TemucraftFlightBonus.h`.
- **Hearthstone keepsakes:** hearthstones with their own look, sold by innkeepers (account-bound, one each): Pocket
  Portal, Innkeeper's Keepsake, Shard of the Dark Portal, Lich-touched Stone, Arcanist's Waystone, Death Gate Shard,
  Ember of the Imp, Voidtouched Stone, Lightbound Stone, Spiritwalker's Stone, and the Ethereal Portal. They share the
  hearthstone's cooldown (and the length each player picked), and are in the Vanity window under "Travel". The new ones
  are copies of Hearthstone with another class spell's casting look (spells 97180-97186, `tools/build_keepsakes.py`
  writes the server `spell_dbc` rows and the client rows in `client/patch-T.MPQ`).
- **Stones and Scrolls of Homecoming:** one of each for every capital: Stormwind, Ironforge, Darnassus, The Exodar
  (Alliance), Orgrimmar, Undercity, Thunder Bluff, Silvermoon (Horde), and Dalaran (level 68) and Shattrath (level 58)
  for both. Use one and after 10 seconds (like a hearthstone: moving or being hit stops it) you are in that city, by
  its bank or at its heart. Not in combat, dungeons, raids, battlegrounds or arenas. Both are kept, not used up:
  a Stone costs 10 gold and has a 15 minute cooldown, a Scroll costs 1 gold and has a 24 hour cooldown. All Stones
  share one cooldown, all Scrolls another, and neither touches the hearthstone's. Sold by every innkeeper who is also
  a vendor; each player only sees their own faction's (every race of the faction, our custom races too) and the two
  for both. A faction change swaps them for the other side's. They are vanity items: with mod-temucraft-account-wide,
  one bought on any character is unlocked for the whole account and can be summoned again from the Vanity window.

## Install

1. Put this folder in `modules/` of your AzerothCore source, re-run CMake and build.
2. The SQL in `data/sql/` is applied by the worldserver on its next start (AzerothCore's module updater). Every file deletes its own rows before inserting them, so running one again is safe.
   Copy `conf/mod-temucraft-travel.conf.dist` to your `configs/modules/` as `mod-temucraft-travel.conf` to change its settings.
3. **Client.** `client/patch-T.MPQ/` is a patch folder, not a packed MPQ: pack it yourself (for example with Ladik's MPQ Editor: new MPQ, add the folder contents keeping the `DBFilesClient\` path) and put the `.MPQ` in the game's `Data/` folder. Each DBC in it is the stock 3.3.5a (12340) file plus only this module's rows.
   **Using more than one Temucraft module, or other DBC changes?** Two patches that both carry `Spell.dbc` overwrite each other (the later letter wins). Use the all-in-one patch from the Temucraft configuration repo (`client/patch-5.MPQ/`, every Temucraft change merged), or merge the rows yourself. The server side never conflicts: it reads this module's rows from the `*_dbc` tables in `data/sql/`.

## What it adds, and where

| Database | Table | IDs |
|---|---|---|
| world | `item_template` | 601620 (sleeping bag), 601621-601623 (repair kits) |
| world | `item_template` | 610030-610039 (Stones of Homecoming), 610040-610049 (Scrolls of Homecoming) |
| world | `gameobject_template` | 601620 (the laid-down bag) |
| world | `quest_template`, `quest_template_addon` | 602100-602101 |
| world | `creature_queststarter`, `creature_questender` | 602100-602101 on every innkeeper |
| world | `creature_template` | every innkeeper gets the quest giver flag (`npcflag \| 2`) |
| world | `npc_vendor` | the kits at blacksmithing and general goods vendors |
| world | `spell_dbc` | 95020 (Cozy) |
| world | `spell_dbc` | 97151-97160 (Stones), 97161-97170 (Scrolls of Homecoming) |
| world | `spellcategory_dbc` | 1820 (Stones' cooldown), 1821 (Scrolls' cooldown) |
| world | `spell_target_position`, `spell_script_names` | 97151-97170 (where each goes; `spell_temucraft_homecoming`) |
| world | `npc_vendor` | the Stones and Scrolls at every innkeeper who is also a vendor |
| world | `player_factionchange_items` | Alliance and Horde Stones and Scrolls swap on a faction change |
| world | `mod_temucraft_vanity_item` | 610030-610049 (made here if missing; read by mod-temucraft-account-wide) |
| characters | `mod_temucraft_character_setting` (setting `hearthstone`) | per character: hearthstone cooldown |
| client | `Spell.dbc` | 95020, 97151-97170 |
| client | `SpellCategory.dbc` | 1820, 1821 |

## Works with other modules

- Every innkeeper becomes a quest giver: another module that changes innkeepers' `npcflag` may undo it, and vice versa.
- Innkeepers with a script of their own keep their own menu (no hearthstone option there; `.temucraft hearthstone` works).
- Uses existing reagents (Wool Cloth 2592, Light Leather 2318, Coarse Thread 2320).
- Stones and Scrolls of Homecoming are on every innkeeper vendor: a module that rebuilds innkeepers' `npc_vendor` lists may drop them.
- The client needs this module's `Spell.dbc` and `SpellCategory.dbc` rows (or the all-in-one patch with them): without them the items still work on the server, but the client shows no cooldown and no spell text.

## Independence

This module doesn't need any other module, nor any other Temucraft module. It uses the stock spell 18282 as the item's use spell (the script does the work).

## Customising IDs

The entries above are a block reserved for this module. Kit items and the bag item are set in the config (`Temucraft.Travel.RepairKits`, `Temucraft.Travel.SleepingBag.Entry`); the bag object (601620), the Cozy spell (95020) and the quests are fixed in the SQL and the script.
The Homecoming spells (97151-97170) and their cooldown groups (1820, 1821) are in the client DBCs too, so keep them
the same in the SQL and the patch; the items (`@StoneItem`, `@ScrollItem`) can move with the `SET` at the top of
`mod_temucraft_travel_homecoming.sql`. To add a city: a new spell (copy a row in both places), its
`spell_target_position` and `spell_script_names` rows, and an item.

## Texts and translations

The 10 player-facing texts (chat messages, gossip options) are in the `module_string` table under the module name
`mod-temucraft-travel`, so they can be changed without rebuilding. Translations go in `module_string_locale` with the same
module and id plus the locale (`deDE`, `esES`, `frFR`, ...). If a row is missing the English text in the code is used.

Part of the Temucraft modules for AzerothCore 3.3.5a.

## Flight masters and shapeshifted players

At a flight master while shapeshifted (or mounted), the menu's first option is "Leave Cat Form and fly" (the form's
own name), with a confirmation ("Flying will remove your Cat Form. Continue?"). Yes removes the form (or dismounts)
and opens the flight map, instead of the "can't do that while shapeshifted" error. Flight masters with a script of
their own, and flight masters the client opens without a menu, are left as they are. `Temucraft.Travel.TaxiLeavesForm`.

## Skyreader's Atlas

Four items (610146 Eastern Kingdoms, 610147 Kalimdor, 610148 Outland, 610149 Northrend). Using one learns every flight
path of your faction on that continent (quest and transport nodes stay out). Sold on the Temucraft Curator's Helpers
list for 100 tokens, in the Vanity window under "Travel", and mailed for the Explore Eastern Kingdoms, Kalimdor and
Outland achievements. RBAC permission 103351 (linked to the player roles).

## Waygates and Wayfarer's Beacon

- **Hearthbound Waygate** (610150) and **Fel Waygate** (610151, a fel portal look): place a portal (objects 608950
  and 608951) for 1 minute. Everyone who clicks it goes to their own hearthstone spot (done on the server, no client
  spell). 30 minute cooldown of their own, shared by both; the hearthstone cooldown is not touched. 200 tokens each.
- **Wayfarer's Beacon** (610152): a tall beam of light (object 608952) on the spot for 5 minutes, to mark a meeting
  place. 5 minute cooldown. 50 tokens.

Sold on the Temucraft Curator's Helpers list (vendor 608011); in the Vanity window under "Travel" (waygates) and
"Toys" (beacon). Not in battlegrounds, arenas or fights. RBAC permissions 103352 (waygates) and 103353 (beacon),
linked to the player roles. `Temucraft.Travel.Waygate.Enable`, `Temucraft.Travel.Waygate.Seconds`,
`Temucraft.Travel.Beacon.Seconds`. The item spells (stock dummy spells 36177 and 49375) are never cast: they only
carry the cooldown so the client shows it.
