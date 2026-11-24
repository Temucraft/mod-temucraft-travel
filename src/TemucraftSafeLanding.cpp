/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Safe landing: a player who drops through the world (a Charge or a leap into a gap in the map, a bad teleport) is put
// back on the last solid ground they stood on, instead of falling into the void and dying. Open world only (some
// dungeon and raid fights are meant to be fallen out of; battlegrounds have their own handling).
//
// While a player stands on the ground their spot is remembered. They only go back to it when all of these are true:
//   - they have been falling for a while (SafeLanding.MinFallTime) and dropped a long way (SafeLanding.MinDrop): a
//     jump or a short drop in a cave never counts;
//   - they are outdoors and no building or cave (WMO) is below them;
//   - there is no floor below them at all (map, vmap and objects), and the map's ground is above them.
// The old check acted on any jump with no floor found below. In caves whose collision the server is missing (vmaps not
// made from the same client files players use) that was every jump, and players were put back at the cave mouth.

#include "TemucraftLocked.h"
#include "Config.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <cmath>
#include <unordered_map>

namespace
{
    bool enabled = true;
    uint32 minFallTime = 2000;                              // ms of falling before we look at all
    float minDrop = 30.0f;                                  // yards below the highest point of the fall

    struct Spot
    {
        uint32 map = 0;
        uint32 instance = 0;
        Position pos;
        uint32 timer = 0;
        uint32 fallTime = 0;                                // ms falling so far (0 = on the ground)
        float fallTop = 0.0f;                               // highest z seen during this fall
        bool known = false;
    };
    Temucraft::LockedMap<uint32, Spot> spots;

    bool UnderMap(Player* player)
    {
        // inside a building or cave: its floor is there even when the server's collision data misses it
        if (!player->IsOutdoors())
            return false;
        Map* map = player->GetMap();
        float const x = player->GetPositionX(), y = player->GetPositionY(), z = player->GetPositionZ();
        uint32 mogpFlags = 0;
        int32 adtId = 0, rootId = 0, groupId = 0;
        if (map->GetAreaInfo(player->GetPhaseMask(), x, y, z, mogpFlags, adtId, rootId, groupId))
            return false;                                   // a WMO below: a normal fall onto it
        float const below = map->GetHeight(player->GetPhaseMask(), x, y, z + 2.0f, true, MAX_FALL_DISTANCE);
        if (below > INVALID_HEIGHT)
            return false;                                   // a floor somewhere below: a normal fall
        // the map's own ground (not a cave roof or a bridge) must be above us
        float const surface = map->GetGridHeight(x, y);
        return surface > INVALID_HEIGHT && surface > z + 5.0f;
    }
}

class TemucraftSafeLandingWorld : public WorldScript
{
public:
    TemucraftSafeLandingWorld() : WorldScript("TemucraftSafeLandingWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("Temucraft.Travel.SafeLanding", true);
        minFallTime = sConfigMgr->GetOption<uint32>("Temucraft.Travel.SafeLanding.MinFallTime", 2000);
        minDrop = sConfigMgr->GetOption<float>("Temucraft.Travel.SafeLanding.MinDrop", 30.0f);
    }
};

class TemucraftSafeLandingPlayer : public PlayerScript
{
public:
    TemucraftSafeLandingPlayer() : PlayerScript("TemucraftSafeLandingPlayer", { PLAYERHOOK_ON_UPDATE, PLAYERHOOK_ON_LOGOUT }) { }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!enabled || !player->IsInWorld() || player->IsBeingTeleported())
            return;
        Spot& spot = spots[player->GetGUID().GetCounter()];
        spot.timer += diff;
        if (spot.timer < 500)
            return;
        uint32 const elapsed = spot.timer;                  // can be more than 500 on a slow map update
        spot.timer = 0;

        // open world only: some dungeon and raid fights are meant to be fallen out of
        if (player->GetMap()->Instanceable())
        {
            spot.fallTime = 0;
            return;
        }
        if (!player->IsAlive() || player->IsInFlight() || player->GetTransport() || player->GetVehicle())
        {
            spot.fallTime = 0;
            return;
        }

        bool const falling = player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);
        if (!falling)
        {
            spot.fallTime = 0;
            if (player->IsFlying() || player->IsInWater())
                return;
            // standing on something: remember it
            float const ground = player->GetMap()->GetHeight(player->GetPhaseMask(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() + 2.0f);
            if (ground > INVALID_HEIGHT && std::fabs(player->GetPositionZ() - ground) < 3.0f)
            {
                spot.map = player->GetMapId();
                spot.instance = player->GetInstanceId();
                spot.pos.Relocate(player->GetPositionX(), player->GetPositionY(), ground + 0.5f, player->GetOrientation());
                spot.known = true;
            }
            return;
        }

        // count how long this fall has lasted and how far it went
        if (spot.fallTime == 0 || player->GetPositionZ() > spot.fallTop)
            spot.fallTop = player->GetPositionZ();
        spot.fallTime += elapsed;
        if (spot.fallTime < minFallTime || spot.fallTop - player->GetPositionZ() < minDrop)
            return;

        if (!spot.known || spot.map != player->GetMapId() || spot.instance != player->GetInstanceId() || !UnderMap(player))
            return;
        spot.fallTime = 0;
        player->NearTeleportTo(spot.pos.GetPositionX(), spot.pos.GetPositionY(), spot.pos.GetPositionZ(), spot.pos.GetOrientation());
    }

    void OnPlayerLogout(Player* player) override { spots.erase(player->GetGUID().GetCounter()); }
};

void AddTemucraftSafeLandingScripts()
{
    new TemucraftSafeLandingWorld();
    new TemucraftSafeLandingPlayer();
}
