/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

/*
 * Talking to a client addon (3.3.5): the addon whispers itself with SendAddonMessage(prefix, text, "WHISPER",
 * UnitName("player")); the server sees "prefix\ttext" in LANG_ADDON and the module answers the same way, so the
 * addon gets CHAT_MSG_ADDON(prefix, text, "WHISPER", name). Needs AddonChannel = 1 in worldserver.conf (default).
 * Fields inside a message are separated by '~', lists by ','.
 */
#ifndef TEMUCRAFT_ADDON_MESSAGE_H
#define TEMUCRAFT_ADDON_MESSAGE_H

#include "Chat.h"
#include "Player.h"
#include "SharedDefines.h"
#include "Tokenize.h"
#include "WorldPacket.h"
#include <string>
#include <string_view>
#include <vector>

// true (and the text after the prefix in `body`) if this is one of our addon messages to ourselves
inline bool TemucraftAddonParse(Player* player, uint32 type, uint32 language, std::string const& msg, Player* receiver,
    std::string_view prefix, std::string& body)
{
    if (type != CHAT_MSG_WHISPER || language != LANG_ADDON || !player || receiver != player)
        return false;
    if (msg.size() <= prefix.size() || msg.compare(0, prefix.size(), prefix) != 0 || msg[prefix.size()] != '\t')
        return false;
    body = msg.substr(prefix.size() + 1);
    return true;
}

inline std::vector<std::string> TemucraftAddonFields(std::string const& body)
{
    std::vector<std::string> out;
    for (std::string_view part : Acore::Tokenize(body, '~', true))
        out.emplace_back(part);
    return out;
}

// names may not contain the separators
inline std::string TemucraftAddonClean(std::string s)
{
    for (char& c : s) if (c == '~' || c == '\t' || c == '|') c = ' ';
    return s;
}

inline void TemucraftAddonSend(Player* player, std::string_view prefix, std::string const& text)
{
    if (!player || !player->GetSession())
        return;
    std::string msg(prefix);
    msg += '\t';
    msg += text.size() > 250 - prefix.size() ? text.substr(0, 250 - prefix.size()) : text;
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, msg);
    player->SendDirectMessage(&data);
}

#endif
