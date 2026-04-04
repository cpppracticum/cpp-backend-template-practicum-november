#pragma once

#include "tagged.h"
#include "player.h"
#include <random>
#include <unordered_map>
#include <optional>
#include <sstream>
#include <iomanip>

namespace model {

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class PlayerTokens {
public:
    PlayerTokens() {
        std::random_device rd;
        generator1_ = std::mt19937_64(rd());
        generator2_ = std::mt19937_64(rd());
    }

    Token GenerateToken(PlayerId player_id) {
        uint64_t part1 = generator1_();
        uint64_t part2 = generator2_();
        
        std::ostringstream oss;
        oss << std::hex << std::setw(16) << std::setfill('0') << part1
            << std::hex << std::setw(16) << std::setfill('0') << part2;
        
        Token token(oss.str());
        tokens_.emplace(token, player_id);
        return token;
    }

    std::optional<PlayerId> GetPlayerByToken(const Token& token) const {
        auto it = tokens_.find(token);
        if (it != tokens_.end()) return it->second;
        return std::nullopt;
    }

private:
    std::mt19937_64 generator1_;
    std::mt19937_64 generator2_;
    std::unordered_map<Token, PlayerId, util::TaggedHasher<Token>> tokens_;
};

}  // namespace model
