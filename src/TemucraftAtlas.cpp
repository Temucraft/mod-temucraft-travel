/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Skyreader's Atlas: one item per continent (Eastern Kingdoms, Kalimdor, Outland, Northrend). Using it learns every
// flight path of your faction on that continent (nodes that have a flight master mount for your side). Quest-only
// and transport nodes stay out. With account-wide flight paths the other characters share them too.

#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemucraftModuleText.h"
#include "WorldSession.h"
#include <string>

namespace
{
    char const* const MODULE = "mod-temucraft-travel";
    constexpr uint32 PERM_USE_ATLAS = 103351;   // RBAC permission (db-auth SQL), linked to the player roles
    constexpr uint32 CONTINENTS[4] = { 0, 1, 530, 571 };

    struct AtlasSettings { bool enable = true; uint32 firstItem = 610146; } acfg;

    bool SkipNode(TaxiNodesEntry const* node)
    {
        std::string name = node->name[0] ? node->name[0] : "";
        return name.empty() || name.find("Quest") != std::string::npos || name.find("Transport") != std::string::npos
            || name.find("(test)") != std::string::npos || name.find("[") != std::string::npos;
    }
}

class item_temucraft_atlas : public ItemScript
{
public:
    item_temucraft_atlas() : ItemScript("item_temucraft_atlas") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        ChatHandler handler(player->GetSession());
        uint32 index = item->GetEntry() - acfg.firstItem;
        if (!acfg.enable || index >= 4)
            return true;
        if (!player->GetSession()->HasPermission(PERM_USE_ATLAS))
        {
            handler.SendSysMessage(TemucraftModuleText(MODULE, player, 60, "You can't use this here."));
            return true;
        }
        uint8 side = player->GetTeamId() == TEAM_ALLIANCE ? 0 : 1;
        uint32 learned = 0;
        for (uint32 i = 0; i < sTaxiNodesStore.GetNumRows(); ++i)
        {
            TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);
            if (!node || node->map_id != CONTINENTS[index] || !node->MountCreatureID[side] || SkipNode(node))
                continue;
            if (player->m_taxi.SetTaximaskNode(node->ID))
                ++learned;
        }
        if (learned)
        {
            player->GetSession()->SendTaxiStatus(player->GetGUID());
            handler.PSendSysMessage(TemucraftModuleText(MODULE, player, 61, "|cff66ccffSkyreader's Atlas:|r {} new flight paths learned."), learned);
        }
        else
            handler.SendSysMessage(TemucraftModuleText(MODULE, player, 62, "|cff66ccffSkyreader's Atlas:|r you know every flight path there already."));
        return true;
    }
};

class TemucraftAtlas_World : public WorldScript
{
public:
    TemucraftAtlas_World() : WorldScript("TemucraftAtlas_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        acfg.enable = sConfigMgr->GetOption<bool>("Temucraft.Travel.Atlas.Enable", true);
        acfg.firstItem = sConfigMgr->GetOption<uint32>("Temucraft.Travel.Atlas.FirstItem", 610146);
    }
};

void AddTemucraftAtlasScripts()
{
    new item_temucraft_atlas();
    new TemucraftAtlas_World();
}
