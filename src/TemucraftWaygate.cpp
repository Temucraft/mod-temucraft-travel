/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Hearthbound Waygate, Fel Waygate and Wayfarer's Beacon.
//
// Waygate (two looks): places a portal in front of you for a short time (1 minute by default). Everyone who clicks it
// goes to their own hearthstone spot (their homebind), done here on the server, so no client spell is needed. Its
// cooldown (spellcooldown_1 of the item, 30 minutes) is its own: the hearthstone cooldown is not touched.
// Beacon: places a tall beam of light on the spot for a while (5 minutes by default), to mark a meeting place.
//
// The item's spell (a stock dummy spell that is never cast) only carries the cooldown, so the client greys the item out
// and shows the time left. Both waygates share one spell, so they share the cooldown too.

#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "GameObject.h"
#include "GameObjectScript.h"
#include "Item.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemucraftModuleText.h"
#include "TemucraftPlayerSettings.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <cmath>

namespace
{
    char const* const MODULE = "mod-temucraft-travel";
    constexpr uint32 PERM_USE_WAYGATE = 103352;   // RBAC permissions (db-auth SQL), linked to the player roles
    constexpr uint32 PERM_USE_BEACON = 103353;

    struct WaygateSettings
    {
        bool enable = true;
        uint32 waygateSeconds = 60;
        uint32 beaconSeconds = 300;
        uint32 waygateObject = 608950;      // Hearthbound Waygate; the Fel Waygate is the next one
        uint32 beaconObject = 608952;
        uint32 waygateItem = 610150;        // Hearthbound Waygate; then the Fel Waygate, then the beacon
    } wcfg;

    // Puts the cooldown of the item's spell on the player, on the server and in the client (and kept over a relog).
    void StartCooldown(Player* player, ItemTemplate const* proto)
    {
        uint32 spellId = proto->Spells[0].SpellId;
        int32 cooldown = proto->Spells[0].SpellCooldown;
        if (!spellId || cooldown <= 0)
            return;
        player->AddSpellCooldown(spellId, proto->ItemId, uint32(cooldown), true);
        WorldPacket data;
        player->BuildCooldownPacket(data, 0, spellId, uint32(cooldown));
        player->SendDirectMessage(&data);
    }
}

class item_temucraft_waygate : public ItemScript
{
public:
    item_temucraft_waygate() : ItemScript("item_temucraft_waygate") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        ChatHandler handler(player->GetSession());
        ItemTemplate const* proto = item->GetTemplate();
        uint32 index = item->GetEntry() - wcfg.waygateItem;   // 0 Hearthbound, 1 Fel, 2 beacon
        if (!wcfg.enable || index > 2)
            return true;
        bool beacon = index == 2;
        if (!player->GetSession()->HasPermission(beacon ? PERM_USE_BEACON : PERM_USE_WAYGATE)
            || player->GetMap()->IsBattlegroundOrArena())
        {
            handler.SendSysMessage(TemucraftModuleText(MODULE, player, 60, "You can't use this here."));
            return true;
        }
        if (player->IsInCombat())
        {
            handler.SendSysMessage(TemucraftModuleText(MODULE, player, 63, "Not during a fight."));
            return true;
        }
        if (proto->Spells[0].SpellId && player->HasSpellCooldown(proto->Spells[0].SpellId))
        {
            uint32 left = player->GetSpellCooldownDelay(proto->Spells[0].SpellId) / IN_MILLISECONDS;
            handler.PSendSysMessage(TemucraftModuleText(MODULE, player, 64, "That isn't ready yet ({} min left)."),
                std::max(1u, (left + 59) / 60));
            return true;
        }

        float angle = player->GetOrientation();
        float dist = beacon ? 0.0f : 3.0f;   // the beacon goes where you stand, the waygate a few steps ahead
        float x = player->GetPositionX() + std::cos(angle) * dist;
        float y = player->GetPositionY() + std::sin(angle) * dist;
        float z = player->GetMap()->GetHeight(player->GetPhaseMask(), x, y, player->GetPositionZ() + 2.0f);
        if (z <= INVALID_HEIGHT || std::fabs(z - player->GetPositionZ()) > 5.0f)
            z = player->GetPositionZ();
        float facing = angle + float(M_PI);   // the waygate faces you
        uint32 object = beacon ? wcfg.beaconObject : wcfg.waygateObject + index;
        uint32 seconds = beacon ? wcfg.beaconSeconds : wcfg.waygateSeconds;
        if (!player->SummonGameObject(object, x, y, z, facing, 0.0f, 0.0f, std::sin(facing / 2), std::cos(facing / 2),
            seconds, true, GO_SUMMON_TIMED_DESPAWN))
        {
            handler.SendSysMessage(TemucraftModuleText(MODULE, player, 65, "There's no room for it here."));
            return true;
        }
        StartCooldown(player, proto);
        if (beacon)
            handler.PSendSysMessage(TemucraftModuleText(MODULE, player, 67,
                "|cff66ccffWayfarer's Beacon:|r a beam of light marks this spot for {} minutes."), std::max(1u, seconds / 60));
        else
            handler.PSendSysMessage(TemucraftModuleText(MODULE, player, 66,
                "|cff66ccff{}:|r a way home stands open for {} seconds. Anyone who steps through goes to their own hearthstone spot."),
                proto->Name1, seconds);
        return true;
    }
};

class go_temucraft_waygate : public GameObjectScript
{
public:
    go_temucraft_waygate() : GameObjectScript("go_temucraft_waygate") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        ChatHandler handler(player->GetSession());
        if (player->IsInCombat())
        {
            handler.SendSysMessage(TemucraftModuleText(MODULE, player, 68, "The way home won't open while you fight."));
            return true;
        }
        // "Teleport to: <hearthstone spot>" first, unless the player turned prompts off (mod-temucraft answers Accept)
        std::string home = TemucraftModuleText("mod-temucraft", player, 136, "your hearthstone spot");
        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(player->m_homebindAreaId))
            if (char const* name = area->area_name[player->GetSession()->GetSessionDbcLocale()]; name && *name)
                home = name;
        if (TemucraftAskObjectTeleport(player, go, home))
            return true;
        player->TeleportTo(player->m_homebindMapId, player->m_homebindX, player->m_homebindY, player->m_homebindZ,
            player->GetOrientation());
        return true;
    }
};

class TemucraftWaygate_World : public WorldScript
{
public:
    TemucraftWaygate_World() : WorldScript("TemucraftWaygate_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        wcfg.enable = sConfigMgr->GetOption<bool>("Temucraft.Travel.Waygate.Enable", true);
        wcfg.waygateSeconds = std::max(5u, sConfigMgr->GetOption<uint32>("Temucraft.Travel.Waygate.Seconds", 60));
        wcfg.beaconSeconds = std::max(5u, sConfigMgr->GetOption<uint32>("Temucraft.Travel.Beacon.Seconds", 300));
    }
};

void AddTemucraftWaygateScripts()
{
    new item_temucraft_waygate();
    new go_temucraft_waygate();
    new TemucraftWaygate_World();
}
