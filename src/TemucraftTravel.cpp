/*
 * mod-temucraft-travel: repair kits and the Cozy Sleeping Bag.
 *
 * Repair kits
 *   Using one repairs every equipped item by a share of its durability, up to an item level. Which items are kits,
 *   and how much and how high each repairs, is Temucraft.Travel.RepairKits ("item:percent:maxItemLevel,...";
 *   maxItemLevel 0 = any). The kits in data/sql: Field Repair Kit (25%, item level 100 and below),
 *   Reinforced Repair Kit (50%, 200), Master Repair Kit (100%, any).
 *
 * Cozy Sleeping Bag
 *   The item lays a bedroll at your feet (Temucraft.Travel.SleepingBag.Duration seconds). Click it to lie down. Every
 *   minute you lie in it you get a stack of Cozy (95020: +1% experience per stack, up to 3, for an hour) and some
 *   rested experience (Temucraft.Travel.SleepingBag.RestedPerMinute percent of your level, up to the usual rested cap).
 *   Standing up, moving away or a fight ends the nap; the stacks you have stay for their hour.
 */

#include "Chat.h"
#include "TemucraftModuleText.h"
#include "Config.h"
#include "GameObject.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "Tokenize.h"

#include <cmath>
#include <mutex>
#include <unordered_map>

namespace
{
    constexpr uint32 SPELL_COZY = 95020;

    struct RepairKit
    {
        uint32 percent = 0;
        uint32 maxItemLevel = 0;
    };

    struct Settings
    {
        bool enable = true;
        std::unordered_map<uint32, RepairKit> kits;
        uint32 bagEntry = 601620;
        uint32 bagDuration = 600;
        float restedPerMinute = 1.0f;
    } settings;

    struct Sleeper
    {
        ObjectGuid bag;
        uint32 timer = 0;
    };

    std::mutex sLock;
    std::unordered_map<ObjectGuid, Sleeper> sSleepers;

    void Tell(Player* player, std::string const& text)
    {
        ChatHandler(player->GetSession()).PSendSysMessage(TemucraftModuleText("mod-temucraft-travel", player->GetSession(), 1, "|cffffcc00{}|r"), text);
    }

    void LoadSettings()
    {
        settings.enable = sConfigMgr->GetOption<bool>("Temucraft.Travel.Enable", true);
        settings.kits.clear();
        std::string kits = sConfigMgr->GetOption<std::string>("Temucraft.Travel.RepairKits", "601621:25:100,601622:50:200,601623:100:0");
        for (std::string_view entry : Acore::Tokenize(kits, ',', false))
        {
            std::vector<std::string_view> parts = Acore::Tokenize(entry, ':', false);
            if (parts.size() < 2)
                continue;
            Optional<uint32> item = Acore::StringTo<uint32>(parts[0]);
            Optional<uint32> percent = Acore::StringTo<uint32>(parts[1]);
            Optional<uint32> maxLevel = parts.size() >= 3 ? Acore::StringTo<uint32>(parts[2]) : Optional<uint32>(0);
            if (item && percent && maxLevel)
                settings.kits[*item] = { std::min<uint32>(*percent, 100), *maxLevel };
        }
        settings.bagEntry = sConfigMgr->GetOption<uint32>("Temucraft.Travel.SleepingBag.Entry", 601620);
        settings.bagDuration = sConfigMgr->GetOption<uint32>("Temucraft.Travel.SleepingBag.Duration", 600);
        settings.restedPerMinute = sConfigMgr->GetOption<float>("Temucraft.Travel.SleepingBag.RestedPerMinute", 1.0f);
    }

    // Repairs the equipped items a kit covers; returns how many it touched
    uint32 UseRepairKit(Player* player, RepairKit const& kit)
    {
        uint32 repaired = 0;
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item)
                continue;
            uint32 maxDurability = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);
            uint32 durability = item->GetUInt32Value(ITEM_FIELD_DURABILITY);
            if (!maxDurability || durability >= maxDurability)
                continue;
            if (kit.maxItemLevel && item->GetTemplate()->ItemLevel > kit.maxItemLevel)
                continue;
            uint32 restored = std::max<uint32>(1, maxDurability * kit.percent / 100);
            // a negative loss mends it, and puts a broken item's stats back on
            player->DurabilityPointsLoss(item, -int32(std::min(restored, maxDurability - durability)));
            ++repaired;
        }
        return repaired;
    }

    void EndNap(Player* player)
    {
        std::lock_guard lock(sLock);
        sSleepers.erase(player->GetGUID());
    }
}

class item_temucraft_repair_kit : public ItemScript
{
public:
    item_temucraft_repair_kit() : ItemScript("item_temucraft_repair_kit") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        auto itr = settings.kits.find(item->GetEntry());
        if (!settings.enable || itr == settings.kits.end())
        {
            Tell(player, "This kit does nothing on this server.");
            return true;
        }
        if (player->IsInCombat())
        {
            Tell(player, "Not during a fight.");
            return true;
        }
        RepairKit const kit = itr->second;
        uint32 repaired = UseRepairKit(player, kit);
        if (!repaired)
        {
            Tell(player, kit.maxItemLevel ? Acore::StringFormat("Nothing this kit can mend: your damaged gear is above item level {}, or nothing is damaged.", kit.maxItemLevel)
                : std::string("Nothing of yours is damaged."));
            return true;
        }
        player->DestroyItemCount(item->GetEntry(), 1, true);
        Tell(player, Acore::StringFormat("Repaired {} item(s) by {}%.", repaired, kit.percent));
        return true;
    }
};

class item_temucraft_sleeping_bag : public ItemScript
{
public:
    item_temucraft_sleeping_bag() : ItemScript("item_temucraft_sleeping_bag") { }

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/) override
    {
        if (!settings.enable)
            return true;
        if (player->IsInCombat() || player->IsInFlight() || player->IsMounted() || player->IsInWater() || !player->IsOutdoors())
        {
            Tell(player, "Find a quiet spot outdoors, out of combat, to lay it down.");
            return true;
        }
        float angle = player->GetOrientation();
        float x = player->GetPositionX() + std::cos(angle) * 1.5f;
        float y = player->GetPositionY() + std::sin(angle) * 1.5f;
        float z = player->GetMap()->GetHeight(player->GetPhaseMask(), x, y, player->GetPositionZ() + 2.0f);
        if (z <= INVALID_HEIGHT)
            z = player->GetPositionZ();
        if (!player->SummonGameObject(settings.bagEntry, x, y, z, angle, 0.0f, 0.0f, std::sin(angle / 2), std::cos(angle / 2),
            settings.bagDuration, true, GO_SUMMON_TIMED_DESPAWN))
            Tell(player, "There's no room for it here.");
        return true;
    }
};

class go_temucraft_sleeping_bag : public GameObjectScript
{
public:
    go_temucraft_sleeping_bag() : GameObjectScript("go_temucraft_sleeping_bag") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (player->IsInCombat())
            return true;
        player->NearTeleportTo(go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), go->GetOrientation());
        player->SetStandState(UNIT_STAND_STATE_SLEEP);
        {
            std::lock_guard lock(sLock);
            sSleepers[player->GetGUID()] = { go->GetGUID(), 60000 };
        }
        Tell(player, "You settle into the sleeping bag. Each minute here makes you a little cozier.");
        return true;
    }
};

class TemucraftTravel_Player : public PlayerScript
{
public:
    TemucraftTravel_Player() : PlayerScript("TemucraftTravel_Player", { PLAYERHOOK_ON_UPDATE, PLAYERHOOK_ON_LOGOUT }) { }

    void OnPlayerLogout(Player* player) override
    {
        EndNap(player);
    }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        ObjectGuid bagGuid;
        {
            std::lock_guard lock(sLock);
            auto itr = sSleepers.find(player->GetGUID());
            if (itr == sSleepers.end())
                return;
            if (itr->second.timer > diff)
            {
                itr->second.timer -= diff;
                return;
            }
            itr->second.timer = 60000;
            bagGuid = itr->second.bag;
        }

        GameObject* bag = ObjectAccessor::GetGameObject(*player, bagGuid);
        if (!bag || !bag->isSpawned() || player->getStandState() != UNIT_STAND_STATE_SLEEP || player->IsInCombat() ||
            player->GetDistance(bag) > 3.0f)
        {
            EndNap(player);
            return;
        }

        if (Aura* cozy = player->GetAura(SPELL_COZY))
        {
            cozy->ModStackAmount(1);
            cozy->RefreshDuration();
        }
        else
            player->AddAura(SPELL_COZY, player);

        uint32 nextLevelXp = player->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
        if (nextLevelXp && settings.restedPerMinute > 0.0f)
            player->SetRestBonus(player->GetRestBonus() + nextLevelXp * settings.restedPerMinute / 100.0f);
    }
};

class TemucraftTravel_World : public WorldScript
{
public:
    TemucraftTravel_World() : WorldScript("TemucraftTravel_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadSettings();
    }
};

void AddTemucraftTravelScripts()
{
    new item_temucraft_repair_kit();
    new item_temucraft_sleeping_bag();
    new go_temucraft_sleeping_bag();
    new TemucraftTravel_Player();
    new TemucraftTravel_World();
}
