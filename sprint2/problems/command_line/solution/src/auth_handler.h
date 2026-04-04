#pragma once

#include "player_tokens.h"
#include "players.h"
#include <string>
#include <optional>

namespace http_handler {

class AuthHandler {
public:
    AuthHandler(model::PlayerTokens& tokens, model::Players& players)
        : tokens_(tokens), players_(players) {}

    std::optional<model::PlayerId> Authenticate(const std::string& auth_header) const {
        const std::string prefix = "Bearer ";
        if (auth_header.size() < prefix.size() ||
            auth_header.substr(0, prefix.size()) != prefix) {
            return std::nullopt;
        }

        std::string token_str = auth_header.substr(prefix.size());
        model::Token token(token_str);
        return tokens_.GetPlayerByToken(token);
    }

private:
    model::PlayerTokens& tokens_;
    model::Players& players_;
};

}  // namespace http_handler
