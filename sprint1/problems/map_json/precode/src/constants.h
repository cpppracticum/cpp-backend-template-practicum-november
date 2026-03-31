#pragma once

#include <string_view>

namespace endpoints {
    constexpr std::string_view MAPS = "/api/v1/maps";
    constexpr std::string_view MAP_BY_ID = "/api/v1/maps/";
    constexpr std::string_view GAME = "/api/v1/game";
    constexpr std::string_view GAME_JOIN = "/api/v1/game/join";
    constexpr std::string_view GAME_PLAYERS = "/api/v1/game/players";
    constexpr std::string_view GAME_STATE = "/api/v1/game/state";
    constexpr std::string_view GAME_ACTION = "/api/v1/game/action";
    constexpr std::string_view GAME_TICK = "/api/v1/game/tick";
}

namespace http_methods {
    constexpr std::string_view GET = "GET";
    constexpr std::string_view POST = "POST";
    constexpr std::string_view PUT = "PUT";
    constexpr std::string_view DELETE = "DELETE";
}
