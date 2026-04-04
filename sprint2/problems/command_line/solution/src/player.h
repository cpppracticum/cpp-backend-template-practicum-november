#pragma once

#include "tagged.h"
#include "dog.h"
#include "model.h"
#include <string>

namespace model {

namespace detail {
struct PlayerTag {};
}  // namespace detail

using PlayerId = util::Tagged<uint64_t, detail::PlayerTag>;

class Player {
public:
    Player(PlayerId id, std::string name, const Map::Id& map_id, DogId dog_id)
        : id_(id), name_(std::move(name)), map_id_(map_id), dog_id_(dog_id) {}

    PlayerId GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const Map::Id& GetMapId() const { return map_id_; }
    DogId GetDogId() const { return dog_id_; }

private:
    PlayerId id_;
    std::string name_;
    Map::Id map_id_;
    DogId dog_id_;
};

}  // namespace model
