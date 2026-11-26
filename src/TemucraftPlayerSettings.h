/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Choices a player made in mod-temucraft-player-settings that other Temucraft modules read.
// Stored on the player (CustomData), so a module that reads them still builds and works without the settings module.
// One header, copied into every module that uses it (each module builds on its own): mod-temucraft-player-settings,
// mod-temucraft, mod-temucraft-classes and every module that asks before a teleport. The copies must stay identical
// (all modules share one include path): change the one in mod-temucraft-player-settings, then run
// _tools\check_shared_headers.py --fix.
#pragma once

#include "DataMap.h"
#include "GameObject.h"
#include "GameTime.h"
#include "GossipDef.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "TemucraftModuleText.h"
#include "WorldSession.h"
#include <string>

// Spell pushback: -1 = the server's Temucraft.Pushback.Mode, 0 = off (today's game), 1 = Wrath, 2 = Vanilla/TBC.
struct TemucraftPushbackChoice : DataMap::Base
{
    int32 mode = -1;
};

inline int32 TemucraftPlayerPushbackMode(Player const* player, int32 serverMode)
{
    if (player)
        if (TemucraftPushbackChoice const* choice = player->CustomData.Get<TemucraftPushbackChoice>("TemucraftPushback"))
            if (choice->mode >= 0)
                return choice->mode;
    return serverMode;
}

inline void TemucraftSetPlayerPushbackMode(Player* player, int32 mode)
{
    player->CustomData.GetDefault<TemucraftPushbackChoice>("TemucraftPushback")->mode = mode;
}

// ---------------------------------------------------------------------------------------------------------------------
// Prompts: "Teleport to: <where>  [Accept] [Cancel]" before a gossip line, portal or other object moves the player, and
// "Open: <chest>" before a chest is opened. The player picks in the settings window (Ask before teleports and portals,
// Ask before opening chests, Remind me that prompts can be turned off); mod-temucraft-player-settings stores the
// result here at login and on every change. The server switches (Temucraft.Portals.Confirm,
// Temucraft.Chests.Confirm) are read by mod-temucraft, which also answers the prompts on objects (TemucraftPrompts.cpp).
// Without mod-temucraft every prompt is off; without the settings module players get: teleports on, chests off,
// reminder on. Bots (sessions without a socket) never get a prompt.

struct TemucraftPromptChoice : DataMap::Base
{
    int8 teleport = -1;             // -1 = not set (the settings module is off): ask
    int8 chest = -1;                // -1 = not set: don't ask
    int8 remind = -1;               // -1 = not set: remind
    bool remindedTeleport = false;  // the chat reminder was shown this login
    bool remindedChest = false;
};

// The player said Accept on an object's prompt: the next use of that object (within a few seconds) goes ahead.
struct TemucraftPromptPass : DataMap::Base
{
    ObjectGuid object;
    uint64 untilMs = 0;
};

constexpr uint32 TEMUCRAFT_PROMPT_MENU = 609060;        // gossip menu id of an object's prompt (no gossip_menu row)
constexpr uint32 TEMUCRAFT_PROMPT_SENDER = 609060;      // sender of its Accept line: action 1 = teleport, 2 = chest
constexpr uint32 TEMUCRAFT_PROMPT_TEXT_PORTAL = 609010; // "A shimmering portal. Do you wish to step through?"
constexpr uint32 TEMUCRAFT_PROMPT_TEXT_CHEST = 609060;  // npc_text of the chest prompt (temucraft_prompts.sql)

// Server switches, set by mod-temucraft from its config (off until it loads, so nothing asks without it).
inline bool& TemucraftPromptTeleportServer() { static bool on = false; return on; }
inline bool& TemucraftPromptChestServer() { static bool on = false; return on; }

// A real client is attached (bots and characters whose client is gone never get a prompt).
inline bool TemucraftPromptHasClient(Player const* player)
{
    return player && player->GetSession() && !player->GetSession()->IsSocketClosed();
}

inline bool TemucraftAskBeforeTeleport(Player const* player)
{
    if (!TemucraftPromptTeleportServer() || !TemucraftPromptHasClient(player))
        return false;
    TemucraftPromptChoice const* choice = player->CustomData.Get<TemucraftPromptChoice>("TemucraftPrompts");
    return !choice || choice->teleport != 0;
}

inline bool TemucraftAskBeforeChest(Player const* player)
{
    if (!TemucraftPromptChestServer() || !TemucraftPromptHasClient(player))
        return false;
    TemucraftPromptChoice const* choice = player->CustomData.Get<TemucraftPromptChoice>("TemucraftPrompts");
    return choice && choice->chest == 1;
}

inline void TemucraftSetPromptChoices(Player* player, bool teleport, bool chest, bool remind)
{
    TemucraftPromptChoice* choice = player->CustomData.GetDefault<TemucraftPromptChoice>("TemucraftPrompts");
    choice->teleport = teleport ? 1 : 0;
    choice->chest = chest ? 1 : 0;
    choice->remind = remind ? 1 : 0;
}

// Once per login (per kind): says in chat that the prompts can be turned off, unless the player turned that off too.
inline void TemucraftPromptReminder(Player* player, bool chest)
{
    if (!TemucraftPromptHasClient(player))
        return;
    TemucraftPromptChoice* choice = player->CustomData.GetDefault<TemucraftPromptChoice>("TemucraftPrompts");
    if (choice->remind == 0)
        return;
    bool& done = chest ? choice->remindedChest : choice->remindedTeleport;
    if (done)
        return;
    done = true;
    ChatHandler(player->GetSession()).SendSysMessage(chest
        ? TemucraftModuleText("mod-temucraft", player, 129, "|cff66ccff[Temucraft]|r Chest prompts can be turned off in .temucraft settings (or at the Temucraft Concierge).")
        : TemucraftModuleText("mod-temucraft", player, 128, "|cff66ccff[Temucraft]|r Teleport and portal prompts can be turned off in .temucraft settings (or at the Temucraft Concierge)."));
}

// The confirmation text for a gossip line that moves the player ("Teleport to: Orgrimmar"), or "" (no popup) for a
// player who turned the prompts off. Use it as the popup text of AddGossipItemFor.
inline std::string TemucraftTeleportBox(Player* player, std::string const& where)
{
    if (!TemucraftAskBeforeTeleport(player))
        return "";
    TemucraftPromptReminder(player, false);
    return std::string(TemucraftModuleText("mod-temucraft", player, 11, "Teleport to: ")) + where;
}

// After PrepareGossipMenu of one of our teleport menus from the database (gossip_menu_option.BoxText = "Teleport to:
// ..."): a player who turned the prompts off gets the lines without the popup (paid and typed lines keep theirs).
inline void TemucraftStripTeleportBoxes(Player* player)
{
    GossipMenuItemContainer& items = const_cast<GossipMenuItemContainer&>(player->PlayerTalkClass->GetGossipMenu().GetMenuItems());
    bool ask = TemucraftAskBeforeTeleport(player);
    for (auto& [id, item] : items)
    {
        if (item.BoxMessage.empty() || item.BoxMoney || item.IsCoded)
            continue;
        if (ask)
        {
            TemucraftPromptReminder(player, false);
            return;
        }
        item.BoxMessage.clear();
    }
}

// Shows a one-line prompt for an object ("<where>" with the "Teleport to: <where>" / "Open: <where>" popup). Accept is
// answered by mod-temucraft, which lets the next use of the object through (TemucraftPromptPassed) and uses it again.
inline void TemucraftSendObjectPrompt(Player* player, GameObject* go, std::string const& where, bool chest)
{
    ClearGossipMenuFor(player);
    std::string box = chest
        ? std::string(TemucraftModuleText("mod-temucraft", player, 130, "Open: ")) + where
        : std::string(TemucraftModuleText("mod-temucraft", player, 11, "Teleport to: ")) + where;
    AddGossipItemFor(player, chest ? GOSSIP_ICON_INTERACT_1 : GOSSIP_ICON_TAXI, where, TEMUCRAFT_PROMPT_SENDER, chest ? 2 : 1, box, 0, false);
    player->PlayerTalkClass->GetGossipMenu().SetMenuId(TEMUCRAFT_PROMPT_MENU);
    SendGossipMenuFor(player, chest ? TEMUCRAFT_PROMPT_TEXT_CHEST : TEMUCRAFT_PROMPT_TEXT_PORTAL, go->GetGUID());
    TemucraftPromptReminder(player, chest);
}

// True (and used up, unless keep) when the player said Accept on this object's prompt in the last few seconds.
inline bool TemucraftPromptPassed(Player* player, GameObject* go, bool keep = false)
{
    TemucraftPromptPass* pass = player->CustomData.Get<TemucraftPromptPass>("TemucraftPromptPass");
    if (!pass || pass->object != go->GetGUID() || uint64(GameTime::GetGameTimeMS().count()) > pass->untilMs)
        return false;
    if (!keep)
        pass->object.Clear();
    return true;
}

inline void TemucraftPromptAllow(Player* player, GameObject* go, uint32 ms)
{
    TemucraftPromptPass* pass = player->CustomData.GetDefault<TemucraftPromptPass>("TemucraftPromptPass");
    pass->object = go->GetGUID();
    pass->untilMs = uint64(GameTime::GetGameTimeMS().count()) + ms;
}

// For a GameObject script whose use moves the player (call it first in OnGossipHello): shows the prompt and returns
// true (the click is answered) when the player wants to be asked; false when he doesn't or just said Accept.
inline bool TemucraftAskObjectTeleport(Player* player, GameObject* go, std::string const& where)
{
    if (!TemucraftAskBeforeTeleport(player) || TemucraftPromptPassed(player, go))
        return false;
    TemucraftSendObjectPrompt(player, go, where, false);
    return true;
}
