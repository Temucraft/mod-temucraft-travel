/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Retail-style flight paths.
//
// Fly through: a flight master's map lets you fly to any flight point you know on this continent, without knowing
// the ones in between. The client builds a multi-hop route itself, but only through the nodes in the mask the server
// sends it (SMSG_SHOWTAXINODES). So when the map opens (core hook OnPlayerSendTaxiMenu), the nodes on the shortest
// way from here to each known node of this map, through every node of the player's faction, are added to the mask
// sent (never to the character). The client is told which ones they are (addon message, prefix TTX: "B", then
// "N~name~name...", then "E"), and the flight map (TemucraftTaxi.lua) shows them as small route dots that can't be
// picked. A route may go through them (core hook OnPlayerCanFlyThroughTaxiNode): its first and last node must still
// be known, and flying through a node never teaches it (only talking to its flight master, or a spell, does).
//
// Starting flight points: every character knows its race's capital and its faction's main capital (Stormwind or
// Orgrimmar), given at its first login; existing characters get them once at their next login.

#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemucraftAddonMessage.h"
#include "TemucraftLocked.h"
#include "WorldSession.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
    constexpr char const* PREFIX = "TTX";
    constexpr std::size_t CHUNK = 200;   // characters of names per message

    bool flyThrough = true;
    bool startNodes = true;
    bool startNodesExisting = true;
    uint32 allianceNode = 2;   // Stormwind
    uint32 hordeNode = 23;     // Orgrimmar
    std::unordered_map<uint8, std::vector<uint32>> raceNodes;

    // player guid -> the nodes the flight map last opened with let them fly through
    Temucraft::LockedMap<uint32, std::unordered_set<uint32>> transit;

    bool InMask(TaxiMask const& mask, uint32 node)
    {
        if (!node || (node - 1) / 32 >= TaxiMaskSize)
            return false;
        return (mask[(node - 1) / 32] & (1u << ((node - 1) % 32))) != 0;
    }

    void AddToMask(TaxiMask& mask, uint32 node)
    {
        if (node && (node - 1) / 32 < TaxiMaskSize)
            mask[(node - 1) / 32] |= 1u << ((node - 1) % 32);
    }

    float Distance(TaxiNodesEntry const* a, TaxiNodesEntry const* b)
    {
        float const dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // The unknown nodes on the shortest way (by distance) from `current` to each known node of its map, through the
    // faction's flight network there.
    std::unordered_set<uint32> TransitNodes(TeamId team, uint32 current, TaxiMask const& known)
    {
        std::unordered_set<uint32> result;
        TaxiNodesEntry const* start = sTaxiNodesStore.LookupEntry(current);
        if (!start)
            return result;
        TaxiMask const& faction = team == TEAM_ALLIANCE ? sAllianceTaxiNodesMask : sHordeTaxiNodesMask;

        std::unordered_map<uint32, float> dist;
        std::unordered_map<uint32, uint32> prev;
        using Item = std::pair<float, uint32>;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
        dist[current] = 0.0f;
        queue.push({ 0.0f, current });
        while (!queue.empty())
        {
            auto const [d, node] = queue.top();
            queue.pop();
            if (d > dist[node])
                continue;
            TaxiNodesEntry const* from = sTaxiNodesStore.LookupEntry(node);
            auto const paths = sTaxiPathSetBySource.find(node);
            if (!from || paths == sTaxiPathSetBySource.end())
                continue;
            for (auto const& [to, path] : paths->second)
            {
                if (!path || !InMask(faction, to) || !InMask(sTaxiNodesMask, to))
                    continue;
                TaxiNodesEntry const* next = sTaxiNodesStore.LookupEntry(to);
                if (!next || next->map_id != start->map_id)
                    continue;
                float const nd = d + Distance(from, next);
                auto const it = dist.find(to);
                if (it == dist.end() || nd < it->second)
                {
                    dist[to] = nd;
                    prev[to] = node;
                    queue.push({ nd, to });
                }
            }
        }

        for (auto const& [node, d] : dist)
        {
            if (node == current || !InMask(known, node))
                continue;
            // walk back from the known node to here; every unknown node on the way is one to fly through
            for (auto it = prev.find(node); it != prev.end() && it->second != current; it = prev.find(it->second))
                if (!InMask(known, it->second))
                    result.insert(it->second);
        }
        return result;
    }

    char const* NodeName(TaxiNodesEntry const* node, LocaleConstant locale)
    {
        if (locale < TOTAL_LOCALES && node->name[locale] && *node->name[locale])
            return node->name[locale];
        return node->name[LOCALE_enUS] ? node->name[LOCALE_enUS] : "";
    }

    // Tell the client which nodes of its map are only to fly through (by name: the client's taxi API has no ids).
    // A name a known node of the same map also has is left out (that node then looks like a destination, and the
    // server refuses it as one).
    void SendTransit(Player* player, uint32 current, std::unordered_set<uint32> const& nodes)
    {
        TemucraftAddonSend(player, PREFIX, "B");
        TaxiNodesEntry const* start = sTaxiNodesStore.LookupEntry(current);
        if (start && !nodes.empty())
        {
            LocaleConstant const locale = player->GetSession()->GetSessionDbcLocale();
            std::unordered_set<std::string> knownNames;
            TaxiMask const& known = player->m_taxi.GetTaxiMask();
            for (uint32 i = 1; i <= TaxiMaskSize * 32; ++i)
                if (InMask(known, i))
                    if (TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i); node && node->map_id == start->map_id)
                        knownNames.insert(TemucraftAddonClean(NodeName(node, locale)));

            std::string chunk;
            for (uint32 id : nodes)
            {
                TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(id);
                if (!node)
                    continue;
                std::string const name = TemucraftAddonClean(NodeName(node, locale));
                if (name.empty() || knownNames.count(name))
                    continue;
                if (!chunk.empty() && chunk.size() + name.size() > CHUNK)
                {
                    TemucraftAddonSend(player, PREFIX, "N" + chunk);
                    chunk.clear();
                }
                chunk += "~" + name;
            }
            if (!chunk.empty())
                TemucraftAddonSend(player, PREFIX, "N" + chunk);
        }
        TemucraftAddonSend(player, PREFIX, "E");
    }

    void GiveStartNodes(Player* player)
    {
        std::vector<uint32> nodes;
        if (auto it = raceNodes.find(player->getRace(true)); it != raceNodes.end())
            nodes = it->second;
        nodes.push_back(player->GetTeamId(true) == TEAM_ALLIANCE ? allianceNode : hordeNode);
        for (uint32 node : nodes)
            if (InMask(sTaxiNodesMask, node))
                player->m_taxi.SetTaximaskNode(node);
    }
}

class TemucraftFlightPaths_World : public WorldScript
{
public:
    TemucraftFlightPaths_World() : WorldScript("TemucraftFlightPaths_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        flyThrough = sConfigMgr->GetOption<bool>("Temucraft.Travel.Flight.FlyThrough", true);
        startNodes = sConfigMgr->GetOption<bool>("Temucraft.Travel.Flight.StartingNodes", true);
        startNodesExisting = sConfigMgr->GetOption<bool>("Temucraft.Travel.Flight.StartingNodes.Existing", true);
        allianceNode = sConfigMgr->GetOption<uint32>("Temucraft.Travel.Flight.StartingNodes.Alliance", 2);
        hordeNode = sConfigMgr->GetOption<uint32>("Temucraft.Travel.Flight.StartingNodes.Horde", 23);
        raceNodes.clear();
        std::string list = sConfigMgr->GetOption<std::string>("Temucraft.Travel.Flight.StartingNodes.Races",
            "1:2,3:6,4:27,5:11,6:22,7:6,10:82,11:94");
        std::replace(list.begin(), list.end(), ',', ' ');
        std::istringstream in(list);
        std::string item;
        while (in >> item)
            if (std::size_t colon = item.find(':'); colon != std::string::npos)
            {
                uint32 const race = uint32(std::strtoul(item.substr(0, colon).c_str(), nullptr, 10));
                uint32 const node = uint32(std::strtoul(item.substr(colon + 1).c_str(), nullptr, 10));
                if (race && race < 256 && node)
                    raceNodes[uint8(race)].push_back(node);
            }
        if (!flyThrough)
            transit.clear();
    }
};

class TemucraftFlightPaths_Player : public PlayerScript
{
public:
    TemucraftFlightPaths_Player() : PlayerScript("TemucraftFlightPaths_Player",
        { PLAYERHOOK_ON_SEND_TAXI_MENU, PLAYERHOOK_CAN_FLY_THROUGH_TAXI_NODE, PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT,
          PLAYERHOOK_ON_DELETE }) { }

    void OnPlayerSendTaxiMenu(Player* player, Creature* /*flightMaster*/, uint32 currentNode, TaxiMask& mask) override
    {
        uint32 const guid = player->GetGUID().GetCounter();
        if (!flyThrough || player->isTaxiCheater())
        {
            transit.erase(guid);
            if (flyThrough)
                SendTransit(player, currentNode, {});
            return;
        }
        std::unordered_set<uint32> nodes = TransitNodes(player->GetTeamId(true), currentNode, player->m_taxi.GetTaxiMask());
        for (uint32 node : nodes)
            AddToMask(mask, node);
        SendTransit(player, currentNode, nodes);
        transit[guid] = std::move(nodes);
    }

    bool OnPlayerCanFlyThroughTaxiNode(Player* player, uint32 nodeId) override
    {
        if (!flyThrough)
            return false;
        auto it = transit.find(player->GetGUID().GetCounter());
        return it != transit.end() && it->second.count(nodeId) != 0;
    }

    void OnPlayerLogin(Player* player) override
    {
        if (!startNodes)
            return;
        uint32 const guid = player->GetGUID().GetCounter();
        if (!player->HasAtLoginFlag(AT_LOGIN_FIRST))
        {
            if (!startNodesExisting)
                return;
            if (CharacterDatabase.Query("SELECT 1 FROM `mod_temucraft_travel_flight_start` WHERE `guid` = {}", guid))
                return;
        }
        GiveStartNodes(player);
        CharacterDatabase.Execute("INSERT IGNORE INTO `mod_temucraft_travel_flight_start` (`guid`) VALUES ({})", guid);
    }

    void OnPlayerLogout(Player* player) override { transit.erase(player->GetGUID().GetCounter()); }

    void OnPlayerDelete(ObjectGuid guid, uint32 /*accountId*/) override
    {
        CharacterDatabase.Execute("DELETE FROM `mod_temucraft_travel_flight_start` WHERE `guid` = {}", guid.GetCounter());
    }
};

void AddTemucraftFlightPathsScripts()
{
    new TemucraftFlightPaths_World();
    new TemucraftFlightPaths_Player();
}
