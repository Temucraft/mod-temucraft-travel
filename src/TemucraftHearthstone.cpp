/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Hearthstone cooldown as each player likes it: none, 15, 30 (the game's own), 45 or 60 minutes. Applies to every
// spell that shares the hearthstone's cooldown (Hearthstone and the items like it). Chosen at any innkeeper or with
// .temucraft hearthstone <minutes>; the server picks the default and which choices are offered.

#include "Chat.h"
#include "CommandScript.h"
#include "Config.h"
#include "Creature.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringConvert.h"
#include "TemucraftModuleText.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace Acore::ChatCommands;

namespace
{
    constexpr char const* MODULE = "mod-temucraft-travel";
    constexpr uint32 SPELL_HEARTHSTONE = 8690;
    constexpr uint32 SENDER_HEARTH = 0x4EA7;
    constexpr uint32 ACTION_MENU = 1000;   // other actions are a number of minutes

    enum Text : uint32
    {
        T_SET = 2, T_STATUS, T_NOT_OFFERED, T_NO_CHOICE, T_GOSSIP_MENU, T_GOSSIP_CHOICE, T_NONE, T_MINUTES, T_DISABLED
    };
    char const* Say(Player* player, uint32 id, char const* fallback) { return TemucraftModuleText(MODULE, player, id, fallback); }

    struct Settings
    {
        bool enable = true;
        bool allowChoice = true;
        uint32 defaultMinutes = 30;
        std::vector<uint32> choices = { 0, 15, 30, 45, 60 };
    } hearth;

    std::unordered_map<uint32, uint32> chosen;   // player guid -> minutes

    void LoadHearthSettings()
    {
        hearth.enable = sConfigMgr->GetOption<bool>("Temucraft.Travel.Hearthstone.Enable", true);
        hearth.allowChoice = sConfigMgr->GetOption<bool>("Temucraft.Travel.Hearthstone.AllowChoice", true);
        hearth.defaultMinutes = sConfigMgr->GetOption<uint32>("Temucraft.Travel.Hearthstone.Default", 30);
        hearth.choices.clear();
        std::string list = sConfigMgr->GetOption<std::string>("Temucraft.Travel.Hearthstone.Choices", "0,15,30,45,60");
        std::replace(list.begin(), list.end(), ',', ' ');
        std::istringstream in(list);
        uint32 minutes;
        while (in >> minutes)
            if (std::find(hearth.choices.begin(), hearth.choices.end(), minutes) == hearth.choices.end())
                hearth.choices.push_back(minutes);
        std::sort(hearth.choices.begin(), hearth.choices.end());
    }

    bool Offered(uint32 minutes) { return std::find(hearth.choices.begin(), hearth.choices.end(), minutes) != hearth.choices.end(); }

    uint32 MinutesOf(Player* player)
    {
        if (hearth.allowChoice)
            if (auto itr = chosen.find(player->GetGUID().GetCounter()); itr != chosen.end() && Offered(itr->second))
                return itr->second;
        return hearth.defaultMinutes;
    }

    std::string Label(Player* player, uint32 minutes)
    {
        return minutes ? Acore::StringFormat(Say(player, T_MINUTES, "{} minutes"), minutes) : std::string(Say(player, T_NONE, "no cooldown"));
    }

    uint32 HearthCategory()
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(SPELL_HEARTHSTONE);
        return info ? info->GetCategory() : 0;
    }

    // After a hearthstone-like spell: its cooldown (and that of every spell sharing it) becomes the player's choice.
    void ApplyHearthCooldown(Player* player, SpellInfo const* spellInfo)
    {
        uint32 const category = HearthCategory();
        if (!category || (spellInfo->Id != SPELL_HEARTHSTONE && spellInfo->GetCategory() != category))
            return;
        uint32 const stock = spellInfo->CategoryRecoveryTime ? spellInfo->CategoryRecoveryTime : spellInfo->RecoveryTime;
        uint32 const wanted = MinutesOf(player) * MINUTE * IN_MILLISECONDS;
        if (!stock || wanted == stock)
            return;
        int32 const delta = int32(wanted) - int32(stock);
        // the cooldown is set during the cast (and the client starts its own): change both a moment later
        player->m_Events.AddEventAtOffset([player, category, wanted, delta]()
        {
            auto itr = sSpellsByCategoryStore.find(category);
            if (itr == sSpellsByCategoryStore.end())
                return;
            for (auto const& [byItem, spellId] : itr->second)
            {
                if (!player->HasSpellCooldown(spellId))
                    continue;
                if (!wanted)
                    player->RemoveSpellCooldown(spellId, true);
                else
                    player->ModifySpellCooldown(spellId, delta);
            }
        }, 300ms);
    }

    void SetChoice(Player* player, uint32 minutes)
    {
        uint32 const guid = player->GetGUID().GetCounter();
        chosen[guid] = minutes;
        CharacterDatabase.Execute("REPLACE INTO `mod_temucraft_character_setting` (`guid`, `setting`, `value`) VALUES ({}, 'hearthstone', {})", guid, minutes);
        ChatHandler(player->GetSession()).PSendSysMessage(Say(player, T_SET,
            "|cffffcc00Hearthstone:|r cooldown set to {} (from its next use)."), Label(player, minutes));
    }

    bool IsPlainInnkeeper(Creature* creature)
    {
        return creature->HasNpcFlag(UNIT_NPC_FLAG_INNKEEPER) && !creature->GetScriptId();
    }
}

class TemucraftHearthstone_Player : public PlayerScript
{
public:
    TemucraftHearthstone_Player() : PlayerScript("TemucraftHearthstone_Player", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT,
        PLAYERHOOK_ON_DELETE, PLAYERHOOK_ON_SPELL_CAST }) { }

    void OnPlayerLogin(Player* player) override
    {
        uint32 const guid = player->GetGUID().GetCounter();
        if (QueryResult r = CharacterDatabase.Query("SELECT `value` FROM `mod_temucraft_character_setting` WHERE `guid` = {} AND `setting` = 'hearthstone'", guid))
            chosen[guid] = uint32(r->Fetch()[0].Get<int32>());
    }

    void OnPlayerLogout(Player* player) override { chosen.erase(player->GetGUID().GetCounter()); }

    void OnPlayerDelete(ObjectGuid guid, uint32 /*accountId*/) override
    {
        CharacterDatabase.Execute("DELETE FROM `mod_temucraft_character_setting` WHERE `guid` = {} AND `setting` = 'hearthstone'", guid.GetCounter());
    }

    void OnPlayerSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (hearth.enable && player && spell && spell->GetSpellInfo())
            ApplyHearthCooldown(player, spell->GetSpellInfo());
    }
};

// Innkeepers: "Hearthstone cooldown" and its choices, above the innkeeper's own menu (innkeepers with a script of
// their own are left alone).
class TemucraftHearthstone_Innkeepers : public AllCreatureScript
{
public:
    TemucraftHearthstone_Innkeepers() : AllCreatureScript("TemucraftHearthstone_Innkeepers") { }

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        if (!hearth.enable || !hearth.allowChoice || hearth.choices.size() < 2 || !IsPlainInnkeeper(creature))
            return false;
        player->PrepareGossipMenu(creature, creature->GetCreatureTemplate()->GossipMenuId, true);
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, Acore::StringFormat(Say(player, T_GOSSIP_MENU,
            "Hearthstone cooldown ({})."), Label(player, MinutesOf(player))), SENDER_HEARTH, ACTION_MENU);
        player->SendPreparedGossip(creature);
        return true;
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sender != SENDER_HEARTH)
            return false;
        if (action == ACTION_MENU)
        {
            ClearGossipMenuFor(player);
            uint32 const current = MinutesOf(player);
            for (uint32 minutes : hearth.choices)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, Acore::StringFormat(Say(player, T_GOSSIP_CHOICE, "{}{}"),
                    minutes == current ? "> " : "", Label(player, minutes)), SENDER_HEARTH, minutes);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }
        CloseGossipMenuFor(player);
        if (hearth.enable && hearth.allowChoice && Offered(action))
            SetChoice(player, action);
        return true;
    }
};

class TemucraftHearthstone_Commands : public CommandScript
{
public:
    TemucraftHearthstone_Commands() : CommandScript("TemucraftHearthstone_Commands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable root = { { "temucraft hearthstone", Handle, 103350, Console::No } };
        return root;
    }

    static bool Handle(ChatHandler* handler, Optional<uint32> minutes)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;
        if (!hearth.enable)
        {
            handler->SendSysMessage(Say(player, T_DISABLED, "Hearthstone cooldowns can't be changed on this server."));
            return true;
        }
        if (!minutes)
        {
            std::string list;
            for (uint32 m : hearth.choices)
                list += (list.empty() ? "" : ", ") + std::to_string(m);
            handler->PSendSysMessage(Say(player, T_STATUS, "|cffffcc00Hearthstone:|r cooldown {}. Choose with .temucraft hearthstone <minutes>: {}."),
                Label(player, MinutesOf(player)), list);
            return true;
        }
        if (!hearth.allowChoice)
        {
            handler->SendSysMessage(Say(player, T_NO_CHOICE, "This server decides that for everyone."));
            return true;
        }
        if (!Offered(*minutes))
        {
            handler->SendSysMessage(Say(player, T_NOT_OFFERED, "That cooldown isn't one of the choices."));
            return true;
        }
        SetChoice(player, *minutes);
        return true;
    }
};

class TemucraftHearthstone_World : public WorldScript
{
public:
    TemucraftHearthstone_World() : WorldScript("TemucraftHearthstone_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override { LoadHearthSettings(); }
};

void AddTemucraftHearthstoneScripts()
{
    new TemucraftHearthstone_Player();
    new TemucraftHearthstone_Innkeepers();
    new TemucraftHearthstone_Commands();
    new TemucraftHearthstone_World();
}
