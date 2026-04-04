#pragma once

#include "player.h"
#include <unordered_map>
#include <optional>

namespace model {

class Players {
public:
    PlayerId AddPlayer(const std::string& name, const Map::Id& map_id, DogId dog_id) {
        PlayerId id(next_id_++);
        players_.emplace(id, Player(id, name, map_id, dog_id));
        return id;
    }

    const Player* GetPlayer(PlayerId id) const {
        auto it = players_.find(id);
        if (it != players_.end()) return &it->second;
        return nullptr;
    }

    const std::unordered_map<PlayerId, Player>& GetAllPlayers() const { return players_; }

private:
    PlayerId::value_type next_id_ = 0;
    std::unordered_map<PlayerId, Player> players_;
};

}  // namespace model
