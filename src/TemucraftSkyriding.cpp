/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Skyriding (prototype): Dragonflight-style flying on normal flying mounts, done on the server. The 3.3.5 client
// only knows plain flying, so the server steers the flight speed from the player's pitch (it is in every movement
// packet): diving builds speed, climbing bleeds it, and the speed slowly settles back to the mount's own. Vigor
// (default 6) pays for Surge Forward (a burst of speed) and Skyward Ascent (a launch upwards); it comes back over
// time, faster while flying fast ("Thrill of the Skies"). Turned on per character with .temucraft skyriding.
// Speed changes go through the server, so with lag the speed feels stepped rather than smooth; the smooth version
// needs the client built from source (its movement code).

#include "Chat.h"
#include "ChatCommand.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TemucraftModuleText.h"
#include "WorldSession.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

using namespace Acore::ChatCommands;

namespace
{
    struct SkyridingConfig
    {
        bool enable = true;
        uint32 tickMs = 250;
        float dive = 1.2f;          // speed rate gained per second per sin(pitch) when diving (lost when climbing)
        float drag = 0.25f;         // share of the gap to the mount's own speed closed per second
        float minFactor = 0.6f;     // slowest, as a share of the mount's own speed
        float maxFactor = 2.2f;     // fastest
        float thrillFactor = 1.5f;  // above this share of the mount's speed, vigor comes back faster
        uint32 vigorMax = 6;
        uint32 regenMs = 15000;     // one vigor point, mounted
        uint32 thrillRegenMs = 5000;
        float surgeFactor = 0.6f;   // Surge Forward adds this share of the mount's speed
        float ascentSpeedZ = 18.0f;
        float ascentSpeedXY = 8.0f;
        float resendStep = 0.05f;   // only send a new speed when it changed by this share of the mount's speed
    } scfg;

    enum Texts : uint32
    {
        T_ON = 69, T_OFF = 70, T_NOT_FLYING = 71, T_NO_VIGOR = 72, T_VIGOR = 73, T_DISABLED = 74,
    };

    char const* Say(Player* player, uint32 id, char const* fallback)
    {
        return TemucraftModuleText("mod-temucraft-travel", player, id, fallback);
    }

    struct Rider
    {
        bool on = false;
        bool riding = false;        // skyriding now (mounted and flying)
        float base = 0.0f;          // the mount's own flight speed rate when the ride started
        float speed = 0.0f;         // current rate the server steers
        float sent = 0.0f;          // last rate sent to the client
        float vigor = 0.0f;
        uint32 timer = 0;
        uint32 regen = 0;
        uint32 shownVigor = 0;
    };

    std::unordered_map<uint32, Rider> riders;

    bool CanSkyride(Player* player)
    {
        return player->IsAlive() && player->IsMounted() && player->IsFlying() && !player->IsInFlight()
            && !player->GetVehicle() && !player->GetTransport() && player->GetSpeedRate(MOVE_FLIGHT) > 0.0f;
    }

    void SendSpeed(Player* player, Rider& r, float rate)
    {
        r.sent = rate;
        player->SetSpeed(MOVE_FLIGHT, rate, true);
    }

    void StopRiding(Player* player, Rider& r)
    {
        if (!r.riding)
            return;
        r.riding = false;
        player->UpdateSpeed(MOVE_FLIGHT, true);   // back to what the auras give
    }

    void ShowVigor(Player* player, Rider& r)
    {
        uint32 const now = uint32(r.vigor);
        if (now == r.shownVigor)
            return;
        r.shownVigor = now;
        std::string bar;
        for (uint32 i = 0; i < scfg.vigorMax; ++i)
            bar += i < now ? "|cff66ccff#|r" : "|cff555555#|r";
        player->GetSession()->SendAreaTriggerMessage(Acore::StringFormat(Say(player, T_VIGOR, "Vigor {} {}/{}"), bar, now, scfg.vigorMax));
    }

    void LoadRider(Player* player)
    {
        Rider& r = riders[player->GetGUID().GetCounter()];
        r = Rider();
        r.vigor = float(scfg.vigorMax);
        r.shownVigor = scfg.vigorMax;
        if (QueryResult result = CharacterDatabase.Query("SELECT `enabled` FROM `mod_temucraft_skyriding` WHERE `guid` = {}", player->GetGUID().GetCounter()))
            r.on = result->Fetch()[0].Get<uint8>() != 0;
    }
}

class TemucraftSkyriding_Player : public PlayerScript
{
public:
    TemucraftSkyriding_Player() : PlayerScript("TemucraftSkyriding_Player", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_UPDATE }) { }

    void OnPlayerLogin(Player* player) override { LoadRider(player); }
    void OnPlayerLogout(Player* player) override { riders.erase(player->GetGUID().GetCounter()); }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!scfg.enable || !player->IsInWorld())
            return;
        auto it = riders.find(player->GetGUID().GetCounter());
        if (it == riders.end() || !it->second.on)
            return;
        Rider& r = it->second;
        r.timer += diff;
        if (r.timer < scfg.tickMs)
            return;
        uint32 const elapsed = r.timer;
        r.timer = 0;
        float const dt = elapsed / 1000.0f;

        if (player->IsBeingTeleported() || !CanSkyride(player))
        {
            StopRiding(player, r);
            // vigor comes back while mounted on the ground too
            if (player->IsMounted() && r.vigor < scfg.vigorMax)
            {
                r.regen += elapsed;
                if (r.regen >= scfg.regenMs) { r.regen = 0; r.vigor = std::min<float>(scfg.vigorMax, r.vigor + 1.0f); ShowVigor(player, r); }
            }
            return;
        }

        if (!r.riding)
        {
            r.riding = true;
            r.base = player->GetSpeedRate(MOVE_FLIGHT);
            r.speed = r.sent = r.base;
        }

        // momentum: pitch is positive nose up; diving (negative) adds speed, climbing takes it away
        float const pitch = player->m_movementInfo.pitch;
        r.speed += -std::sin(pitch) * scfg.dive * dt;
        r.speed -= (r.speed - r.base) * std::min(1.0f, scfg.drag * dt);
        r.speed = std::clamp(r.speed, r.base * scfg.minFactor, r.base * scfg.maxFactor);
        if (std::fabs(r.speed - r.sent) >= r.base * scfg.resendStep)
            SendSpeed(player, r, r.speed);

        // vigor: faster while flying fast (Thrill of the Skies)
        if (r.vigor < scfg.vigorMax)
        {
            r.regen += elapsed;
            uint32 const need = r.speed >= r.base * scfg.thrillFactor ? scfg.thrillRegenMs : scfg.regenMs;
            if (r.regen >= need) { r.regen = 0; r.vigor = std::min<float>(scfg.vigorMax, r.vigor + 1.0f); ShowVigor(player, r); }
        }
    }
};

class TemucraftSkyriding_Commands : public CommandScript
{
public:
    TemucraftSkyriding_Commands() : CommandScript("TemucraftSkyriding_Commands") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable sub =
        {
            { "surge",  HandleSurge,  103354, Console::No },
            { "ascent", HandleAscent, 103354, Console::No },
            { "",       HandleToggle, 103354, Console::No },
        };
        static ChatCommandTable root = { { "temucraft skyriding", sub } };
        return root;
    }

    static Rider* Get(ChatHandler* handler, Player*& player)
    {
        player = handler->GetPlayer();
        if (!player)
            return nullptr;
        if (!scfg.enable)
        {
            handler->SendSysMessage(Say(player, T_DISABLED, "Skyriding is turned off on this server."));
            return nullptr;
        }
        auto it = riders.find(player->GetGUID().GetCounter());
        if (it == riders.end())
        {
            LoadRider(player);
            it = riders.find(player->GetGUID().GetCounter());
        }
        return &it->second;
    }

    static bool HandleToggle(ChatHandler* handler)
    {
        Player* player = nullptr;
        Rider* r = Get(handler, player);
        if (!r)
            return true;
        r->on = !r->on;
        if (!r->on)
            StopRiding(player, *r);
        CharacterDatabase.Execute("REPLACE INTO `mod_temucraft_skyriding` (`guid`, `enabled`) VALUES ({}, {})", player->GetGUID().GetCounter(), r->on ? 1 : 0);
        handler->SendSysMessage(r->on
            ? Say(player, T_ON, "|cff66ccffSkyriding on:|r dive to gain speed, climb to lose it. Bind .temucraft skyriding surge and .temucraft skyriding ascent to macros.")
            : Say(player, T_OFF, "|cff66ccffSkyriding off:|r your flying mounts fly as usual."));
        return true;
    }

    static bool Spend(ChatHandler* handler, Player* player, Rider& r)
    {
        if (!r.on || !r.riding || !CanSkyride(player))
        {
            handler->SendSysMessage(Say(player, T_NOT_FLYING, "Only while skyriding: turn it on with .temucraft skyriding and fly on a flying mount."));
            return false;
        }
        if (r.vigor < 1.0f)
        {
            player->GetSession()->SendAreaTriggerMessage(Say(player, T_NO_VIGOR, "Not enough vigor."));
            return false;
        }
        r.vigor -= 1.0f;
        ShowVigor(player, r);
        return true;
    }

    static bool HandleSurge(ChatHandler* handler)
    {
        Player* player = nullptr;
        Rider* r = Get(handler, player);
        if (!r || !Spend(handler, player, *r))
            return true;
        r->speed = std::min(r->speed + r->base * scfg.surgeFactor, r->base * scfg.maxFactor);
        SendSpeed(player, *r, r->speed);
        return true;
    }

    static bool HandleAscent(ChatHandler* handler)
    {
        Player* player = nullptr;
        Rider* r = Get(handler, player);
        if (!r || !Spend(handler, player, *r))
            return true;
        // a launch up and a little forward: the knockback source is just behind the player
        float const o = player->GetOrientation();
        player->KnockbackFrom(player->GetPositionX() - std::cos(o) * 2.0f, player->GetPositionY() - std::sin(o) * 2.0f,
                              scfg.ascentSpeedXY, scfg.ascentSpeedZ);
        return true;
    }
};

class TemucraftSkyriding_World : public WorldScript
{
public:
    TemucraftSkyriding_World() : WorldScript("TemucraftSkyriding_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        scfg.enable = sConfigMgr->GetOption<bool>("Temucraft.Travel.Skyriding.Enable", true);
        scfg.dive = sConfigMgr->GetOption<float>("Temucraft.Travel.Skyriding.Dive", 1.2f);
        scfg.drag = sConfigMgr->GetOption<float>("Temucraft.Travel.Skyriding.Drag", 0.25f);
        scfg.minFactor = sConfigMgr->GetOption<float>("Temucraft.Travel.Skyriding.MinSpeed", 0.6f);
        scfg.maxFactor = sConfigMgr->GetOption<float>("Temucraft.Travel.Skyriding.MaxSpeed", 2.2f);
        scfg.vigorMax = std::max<uint32>(1, sConfigMgr->GetOption<uint32>("Temucraft.Travel.Skyriding.Vigor", 6));
        scfg.regenMs = sConfigMgr->GetOption<uint32>("Temucraft.Travel.Skyriding.VigorRegen", 15) * IN_MILLISECONDS;
        scfg.thrillRegenMs = sConfigMgr->GetOption<uint32>("Temucraft.Travel.Skyriding.ThrillRegen", 5) * IN_MILLISECONDS;
        scfg.surgeFactor = sConfigMgr->GetOption<float>("Temucraft.Travel.Skyriding.Surge", 0.6f);
    }
};

void AddTemucraftSkyridingScripts()
{
    new TemucraftSkyriding_Player();
    new TemucraftSkyriding_Commands();
    new TemucraftSkyriding_World();
}
