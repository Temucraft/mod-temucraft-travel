/*
 * mod-temucraft-travel: Stones and Scrolls of Homecoming.
 *
 * Each one takes you to a capital (spell_target_position) after a 10 second cast, like a hearthstone. The spells
 * (data/sql/db-world/base/mod_temucraft_travel_dbc.sql) already can't be cast in combat or in arenas; this script
 * also keeps them out of every dungeon, raid, battleground and arena.
 */

#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"

class spell_temucraft_homecoming : public SpellScript
{
    PrepareSpellScript(spell_temucraft_homecoming);

    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return SPELL_FAILED_DONT_REPORT;
        if (caster->IsInCombat())
            return SPELL_FAILED_AFFECTING_COMBAT;
        if (Map* map = caster->GetMap())
            if (map->Instanceable())   // dungeons, raids, battlegrounds and arenas
                return SPELL_FAILED_NOT_HERE;
        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_temucraft_homecoming::CheckCast);
    }
};

void AddTemucraftHomecomingScripts()
{
    RegisterSpellScript(spell_temucraft_homecoming);
}
