/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Flight path speed bonuses other modules hand to mod-temucraft-travel (which speeds the flight up): each module puts
// its own percentage under its own name in the player's CustomData. Copy this header as is into a module that grants
// one; without mod-temucraft-travel the bonus simply does nothing.

#ifndef TEMUCRAFT_FLIGHT_BONUS_H
#define TEMUCRAFT_FLIGHT_BONUS_H

#include "DataMap.h"
#include "Player.h"
#include <map>
#include <string>

struct TemucraftFlightBonus : public DataMap::Base
{
    std::map<std::string, float> sources;   // module -> percent

    float Total() const
    {
        float total = 0.0f;
        for (auto const& [name, pct] : sources)
            total += pct;
        return total;
    }
};

inline void TemucraftSetFlightBonus(Player* player, std::string const& source, float pct)
{
    TemucraftFlightBonus* bonus = player->CustomData.GetDefault<TemucraftFlightBonus>("TemucraftFlightBonus");
    if (pct > 0.0f)
        bonus->sources[source] = pct;
    else
        bonus->sources.erase(source);
}

inline float TemucraftGetFlightBonus(Player* player)
{
    TemucraftFlightBonus* bonus = player->CustomData.Get<TemucraftFlightBonus>("TemucraftFlightBonus");
    return bonus ? bonus->Total() : 0.0f;
}

#endif
