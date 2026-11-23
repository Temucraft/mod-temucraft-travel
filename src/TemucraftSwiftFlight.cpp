/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Swift flights: flight paths go faster for players with a flight speed bonus, on top of the server's flight speed
// (Rate: TaxiFlightSpeed). Bonuses come from spells or auras the player has (config: the guild perk Ride Like the Wind,
// 117983, +25% by default) and from other modules (mod-temucraft-legacy's Swift Flights perk, through
// TemucraftFlightBonus.h). A moment after a flight starts (or goes on after a loading screen) its path is set off again
// at the faster speed; the flight's stops, events and arrival work as before.

#include "TemucraftLocked.h"
#include "Config.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemucraftFlightBonus.h"
#include "WaypointMovementGenerator.h"
#include "World.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace
{
    bool enabled = true;
    float maxBonus = 100.0f;
    std::unordered_map<uint32, float> spellBonus;   // spell (known or aura) -> percent

    float Bonus(Player* p)
    {
        float pct = TemucraftGetFlightBonus(p);
        for (auto const& [spell, value] : spellBonus)
            if (p->HasSpell(spell) || p->HasAura(spell))
                pct += value;
        return std::min(pct, maxBonus);
    }

    // set the remaining flight off again at the wanted speed (as FlightPathMovementGenerator::DoReset does)
    void Relaunch(Player* p, float velocity)
    {
        MovementGenerator* top = p->GetMotionMaster()->top();
        if (!top || top->GetMovementGeneratorType() != FLIGHT_MOTION_TYPE)
            return;
        FlightPathMovementGenerator* flight = static_cast<FlightPathMovementGenerator*>(top);
        TaxiPathNodeList const& path = flight->GetPath();
        uint32 const end = flight->GetPathAtMapEnd();
        uint32 const current = flight->GetCurrentNode();
        if (current >= end || end > path.size())
            return;
        Movement::MoveSplineInit init(p);
        init.Path().push_back(G3D::Vector3(p->GetPositionX(), p->GetPositionY(), p->GetPositionZ()));
        for (uint32 i = current; i != end; ++i)
            init.Path().push_back(G3D::Vector3(path[i]->x, path[i]->y, path[i]->z));
        init.SetFirstPointId(current);
        init.SetFly();
        init.SetVelocity(velocity);
        init.Launch();
    }
}

class TemucraftSwiftFlightWorld : public WorldScript
{
public:
    TemucraftSwiftFlightWorld() : WorldScript("TemucraftSwiftFlightWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("Temucraft.Travel.SwiftFlights", true);
        maxBonus = sConfigMgr->GetOption<float>("Temucraft.Travel.SwiftFlights.Max", 100.0f);
        spellBonus.clear();
        std::string list = sConfigMgr->GetOption<std::string>("Temucraft.Travel.SwiftFlights.Spells", "117983:25");
        std::replace(list.begin(), list.end(), ',', ' ');
        std::istringstream in(list);
        std::string item;
        while (in >> item)
            if (std::size_t colon = item.find(':'); colon != std::string::npos)
                spellBonus[uint32(std::strtoul(item.substr(0, colon).c_str(), nullptr, 10))] = std::strtof(item.substr(colon + 1).c_str(), nullptr);
    }
};

class TemucraftSwiftFlightPlayer : public PlayerScript
{
public:
    TemucraftSwiftFlightPlayer() : PlayerScript("TemucraftSwiftFlightPlayer", { PLAYERHOOK_ON_UPDATE, PLAYERHOOK_ON_LOGOUT }) { }

    void OnPlayerUpdate(Player* p, uint32 diff) override
    {
        if (!enabled || !p->IsInFlight())
            return;
        uint32& t = timers[p->GetGUID().GetCounter()];
        t += diff;
        if (t < 250)
            return;
        t = 0;
        if (p->movespline->Finalized())
            return;
        float const pct = Bonus(p);
        if (pct <= 0.0f)
            return;
        float const wanted = sWorld->getFloatConfig(CONFIG_TAXI_FLIGHT_SPEED) * (1.0f + pct / 100.0f);
        if (p->movespline->Velocity() + 0.01f < wanted)
            Relaunch(p, wanted);
    }

    void OnPlayerLogout(Player* p) override { timers.erase(p->GetGUID().GetCounter()); }

private:
    Temucraft::LockedMap<uint32, uint32> timers;
};

void AddTemucraftSwiftFlightScripts()
{
    new TemucraftSwiftFlightWorld();
    new TemucraftSwiftFlightPlayer();
}
