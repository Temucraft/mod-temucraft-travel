/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Flight masters and shapeshifted or mounted players: instead of "You can't do that while shapeshifted" (or
// "already mounted") when picking a destination, the flight master's menu gets a first option "Leave <form> and fly".
// It asks for confirmation ("Flying will remove your Cat Form. Continue?"), then removes the form (or dismounts) and
// opens the flight map. Flight masters with a script of their own are left alone.
// Druids in Flight Form get "Fly in Flight Form" instead: the flight costs half (Temucraft.Travel.FlightFormFare %) and
// they are back in Flight Form when they land.

#include "TemucraftLocked.h"
#include "Config.h"
#include "GameTime.h"
#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "StringFormat.h"
#include "TemucraftModuleText.h"
#include "WorldSession.h"
#include <unordered_map>

namespace
{
    constexpr char const* MODULE = "mod-temucraft-travel";
    constexpr uint32 SENDER_TAXI_FORM = 0x7A71;
    enum Text : uint32 { T_FORM_OPTION = 40, T_FORM_CONFIRM, T_MOUNT_OPTION, T_MOUNT_CONFIRM, T_FLIGHT_FORM };
    constexpr uint32 ACTION_LEAVE = 0, ACTION_FLIGHT_FORM = 1;
    char const* Say(Player* p, uint32 id, char const* fallback) { return TemucraftModuleText(MODULE, p, id, fallback); }

    bool enabled = true;
    float flightFormFare = 50.0f;   // % of the price paid in Flight Form

    // druids who chose "Fly in Flight Form": the form spell to give back on landing, until when the offer stands
    struct Fare { uint32 spell = 0; uint32 until = 0; bool flying = false; };
    Temucraft::LockedMap<uint32, Fare> fares;

    uint32 FlightFormSpell(Player* p)
    {
        if (p->getClass() != CLASS_DRUID)
            return 0;
        ShapeshiftForm const form = p->GetShapeshiftForm();
        if (form != FORM_FLIGHT && form != FORM_FLIGHT_EPIC)
            return 0;
        for (AuraEffect const* eff : p->GetAuraEffectsByType(SPELL_AURA_MOD_SHAPESHIFT))
            return eff->GetId();
        return 0;
    }

    // the form's name, from its shapeshift aura ("Cat Form", "Travel Form"...)
    std::string FormName(Player* p)
    {
        Unit::AuraEffectList const& auras = p->GetAuraEffectsByType(SPELL_AURA_MOD_SHAPESHIFT);
        for (AuraEffect const* eff : auras)
            if (SpellInfo const* info = eff->GetSpellInfo())
                if (char const* name = info->SpellName[p->GetSession()->GetSessionDbcLocale()]; name && *name)
                    return name;
        return "shapeshift form";
    }

    bool PlainFlightMaster(Creature* c)
    {
        return c && c->IsTaxi() && c->GetScriptName().empty();
    }
}

class TemucraftTaxiForms_World : public WorldScript
{
public:
    TemucraftTaxiForms_World() : WorldScript("TemucraftTaxiForms_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }
    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("Temucraft.Travel.TaxiLeavesForm", true);
        flightFormFare = sConfigMgr->GetOption<float>("Temucraft.Travel.FlightFormFare", 50.0f);
    }
};

class TemucraftTaxiForms_Creatures : public AllCreatureScript
{
public:
    TemucraftTaxiForms_Creatures() : AllCreatureScript("TemucraftTaxiForms_Creatures") { }

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        if (!enabled || !PlainFlightMaster(creature))
            return false;
        bool const form = player->IsInDisallowedMountForm();
        bool const mounted = player->IsMounted();
        if (!form && !mounted)
            return false;
        player->PrepareGossipMenu(creature, creature->GetCreatureTemplate()->GossipMenuId, true);
        if (form && FlightFormSpell(player) && flightFormFare < 100.0f)
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, Acore::StringFormat(Say(player, T_FLIGHT_FORM, "Fly in Flight Form ({}% of the price)."),
                uint32(flightFormFare)), SENDER_TAXI_FORM, ACTION_FLIGHT_FORM);
        else if (form)
        {
            std::string const name = FormName(player);
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, Acore::StringFormat(Say(player, T_FORM_OPTION, "Leave {} and fly."), name),
                SENDER_TAXI_FORM, ACTION_LEAVE, Acore::StringFormat(Say(player, T_FORM_CONFIRM, "Flying will remove your {}. Continue?"), name), 0, false);
        }
        else
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, Say(player, T_MOUNT_OPTION, "Dismount and fly."),
                SENDER_TAXI_FORM, ACTION_LEAVE, Say(player, T_MOUNT_CONFIRM, "Flying will dismount you. Continue?"), 0, false);
        player->SendPreparedGossip(creature);
        return true;
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sender != SENDER_TAXI_FORM)
            return false;
        CloseGossipMenuFor(player);
        if (action == ACTION_FLIGHT_FORM)
            if (uint32 spell = FlightFormSpell(player))
                fares[player->GetGUID().GetCounter()] = { spell, uint32(GameTime::GetGameTime().count() + 120), false };
        if (player->IsInDisallowedMountForm())
            player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
        if (player->IsMounted())
        {
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);
            player->Dismount();
        }
        player->GetSession()->SendTaxiMenu(creature);
        return true;
    }
};

// Flight Form fare: the discount, and the form back after landing
class TemucraftTaxiForms_Player : public PlayerScript
{
public:
    TemucraftTaxiForms_Player() : PlayerScript("TemucraftTaxiForms_Player",
        { PLAYERHOOK_ON_GET_REPUTATION_PRICE_DISCOUNT, PLAYERHOOK_ON_UPDATE, PLAYERHOOK_ON_LOGOUT }) { }

    void OnPlayerGetReputationPriceDiscount(Player const* player, Creature const* creature, float& discount) override
    {
        if (!creature || !creature->IsTaxi())
            return;
        auto it = fares.find(player->GetGUID().GetCounter());
        if (it != fares.end() && !it->second.flying && it->second.until >= uint32(GameTime::GetGameTime().count()))
            discount *= flightFormFare / 100.0f;
    }

    void OnPlayerUpdate(Player* player, uint32 /*diff*/) override
    {
        if (fares.empty())
            return;
        auto it = fares.find(player->GetGUID().GetCounter());
        if (it == fares.end())
            return;
        Fare& fare = it->second;
        if (player->IsInFlight())
            fare.flying = true;
        else if (fare.flying)
        {
            uint32 const spell = fare.spell;
            fares.erase(it);
            if (player->IsAlive() && player->IsOutdoors() && !player->IsInCombat())
                player->CastSpell(player, spell, TriggerCastFlags(TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD | TRIGGERED_IGNORE_POWER_AND_REAGENT_COST));
        }
        else if (fare.until < uint32(GameTime::GetGameTime().count()))
            fares.erase(it);
    }

    void OnPlayerLogout(Player* player) override { fares.erase(player->GetGUID().GetCounter()); }
};

void AddTemucraftTaxiFormsScripts()
{
    new TemucraftTaxiForms_Player();
    new TemucraftTaxiForms_World();
    new TemucraftTaxiForms_Creatures();
}
