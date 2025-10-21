#pragma once
#include "model.h"
#include "collision_detector.h"
#include <chrono>
#include <unordered_map>
#include <memory>

namespace app {

class Application {
public:
    Application(model::Game& game, bool randomize_spawn_points = false)
        : game_(game), randomize_spawn_points_(randomize_spawn_points) {
        InitializeCollisionDetectors();
    }

    struct JoinGameResult {
        std::string auth_token;
        model::Player::Id player_id;
    };
    
    std::optional<JoinGameResult> JoinGame(const std::string& user_name, const std::string& map_id);
    std::vector<const model::Player*> GetPlayers(const std::string& auth_token);
    std::vector<const model::Player*> GetGameState(const std::string& auth_token);
    bool SetPlayerAction(const model::Player& player, const std::string& move);
    
    void Tick(std::chrono::milliseconds delta);
    void UpdateGameState(double delta_time_seconds);
    const model::Player* FindPlayerByToken(const std::string& auth_token);
    bool ShouldRandomizeSpawnPoints() const { return randomize_spawn_points_; }
    const model::Map* FindMap(const model::Map::Id& id) const;
    model::Dog* FindDog(const model::Dog::Id& id);

private:
    model::Game& game_;
    bool randomize_spawn_points_;
    std::unordered_map<model::Map::Id, std::unique_ptr<model::CollisionDetector>> collision_detectors_;
    
    void InitializeCollisionDetectors();
    void MoveDog(model::Dog& dog, double delta_time);
    double GetDogSpeedForMap(const model::Map::Id& map_id) const;
};

} // namespace app