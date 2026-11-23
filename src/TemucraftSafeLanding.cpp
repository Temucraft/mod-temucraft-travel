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
// While a player stands on the ground their spot is remembered (every second). When they are falling with no floor
// anywhere below them but ground above them, they are under the map: they go back to that spot.

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

    struct Spot
    {
        uint32 map = 0;
        uint32 instance = 0;
        Position pos;
        uint32 timer = 0;
        bool known = false;
    };
    Temucraft::LockedMap<uint32, Spot> spots;

    bool UnderMap(Player* player)
    {
        Map* map = player->GetMap();
        float const x = player->GetPositionX(), y = player->GetPositionY(), z = player->GetPositionZ();
        float const below = map->GetHeight(player->GetPhaseMask(), x, y, z + 2.0f, true, MAX_FALL_DISTANCE);
        if (below > INVALID_HEIGHT)
            return false;                                   // a floor somewhere below: a normal fall
        float const surface = map->GetHeight(player->GetPhaseMask(), x, y, MAX_HEIGHT, true, MAX_FALL_DISTANCE);
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
        spot.timer = 0;

        // open world only: some dungeon and raid fights are meant to be fallen out of
        if (player->GetMap()->Instanceable())
            return;
        if (!player->IsAlive() || player->IsInFlight() || player->GetTransport() || player->GetVehicle())
            return;

        bool const falling = player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);
        if (!falling)
        {
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

        if (!spot.known || spot.map != player->GetMapId() || spot.instance != player->GetInstanceId() || !UnderMap(player))
            return;
        player->NearTeleportTo(spot.pos.GetPositionX(), spot.pos.GetPositionY(), spot.pos.GetPositionZ(), spot.pos.GetOrientation());
    }

    void OnPlayerLogout(Player* player) override { spots.erase(player->GetGUID().GetCounter()); }
};

void AddTemucraftSafeLandingScripts()
{
    new TemucraftSafeLandingWorld();
    new TemucraftSafeLandingPlayer();
}
