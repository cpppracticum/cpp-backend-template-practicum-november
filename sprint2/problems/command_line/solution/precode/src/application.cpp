#include "application.h"
#include <random>
#include <boost/json.hpp>

namespace app {

namespace {

std::string GenerateToken() {
    std::random_device rd;
    std::mt19937_64 gen1(rd());
    std::mt19937_64 gen2(rd());
    std::uniform_int_distribution<uint64_t> dist;
    
    uint64_t part1 = dist(gen1);
    uint64_t part2 = dist(gen2);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0') 
       << std::setw(16) << part1 
       << std::setw(16) << part2;
    return ss.str();
}

} // namespace

void Application::InitializeCollisionDetectors() {
    for (const auto& map : game_.GetMaps()) {
        collision_detectors_[map.GetId()] = 
            std::make_unique<model::CollisionDetector>(map);
    }
}

void Application::Tick(std::chrono::milliseconds delta) {
    double delta_seconds = delta.count() / 1000.0;
    UpdateGameState(delta_seconds);
}

void Application::UpdateGameState(double delta_time_seconds) {
    for (auto& dog : game_.GetDogs()) {
        MoveDog(dog, delta_time_seconds);
    }
}

void Application::MoveDog(model::Dog& dog, double delta_time) {
    if (dog.GetVelocity().vx == 0 && dog.GetVelocity().vy == 0) {
        return; 
    }
    
    auto map_id = dog.GetMapId();
    auto it = collision_detectors_.find(map_id);
    if (it == collision_detectors_.end()) {
        return; 
    }
    
    auto& detector = *it->second;
    auto current_pos = dog.GetPosition();
    auto velocity = dog.GetVelocity();
    
    auto movement_result = detector.CalculateMovement(current_pos, velocity, delta_time);
    
    dog.SetPosition(movement_result.new_position);
    
    if (movement_result.collision_occurred) {
        dog.SetVelocity({0.0, 0.0});
    }
}

std::optional<Application::JoinGameResult> Application::JoinGame(const std::string& user_name, const std::string& map_id) {
    model::Map::Id map_id_obj{map_id};
    const auto* map = game_.FindMap(map_id_obj);
    if (!map) {
        return std::nullopt;
    }
    
    if (user_name.empty()) {
        return std::nullopt;
    }
    
    static uint32_t next_dog_id = 0;
    model::Dog::Id dog_id{model::Dog::Id::value_type{next_dog_id++}};
    
    model::Position spawn_position;
    if (randomize_spawn_points_) {
        spawn_position = map->GetRandomDogPosition();
    } else {
        spawn_position = map->GetDefaultDogPosition();
    }
    
    model::Dog dog{dog_id, user_name, map_id_obj, spawn_position};
    game_.GetDogs().push_back(std::move(dog));
    
    static uint32_t next_player_id = 0;
    model::Player::Id player_id{model::Player::Id::value_type{next_player_id++}};
    std::string token = GenerateToken();
    
    model::Player player{player_id, user_name, dog_id, map_id_obj, token};
    game_.GetPlayers().push_back(std::move(player));
    
    auto& players = game_.GetPlayers();
    auto& dogs = game_.GetDogs();
    
    game_.GetTokenToPlayerIndex()[token] = players.size() - 1;
    game_.GetPlayerIdToIndex()[player_id] = players.size() - 1;
    game_.GetDogIdToIndex()[dog_id] = dogs.size() - 1;
    
    return JoinGameResult{token, player_id};
}

std::vector<const model::Player*> Application::GetPlayers(const std::string& auth_token) {
    auto player = FindPlayerByToken(auth_token);
    if (!player) {
        return {};
    }
    
    std::vector<const model::Player*> players_on_map;
    for (const auto& p : game_.GetPlayers()) {
        if (p.GetMapId() == player->GetMapId()) {
            players_on_map.push_back(&p);
        }
    }
    
    return players_on_map;
}

std::vector<const model::Player*> Application::GetGameState(const std::string& auth_token) {
    auto player = FindPlayerByToken(auth_token);
    if (!player) {
        return {};
    }
    
    std::vector<const model::Player*> players_on_map;
    for (const auto& p : game_.GetPlayers()) {
        if (p.GetMapId() == player->GetMapId()) {
            players_on_map.push_back(&p);
        }
    }
    
    return players_on_map;
}

bool Application::SetPlayerAction(const model::Player& player, const std::string& move) {
    auto* dog = FindDog(player.GetDogId());
    if (!dog) {
        return false;
    }
    
    const auto* map = FindMap(player.GetMapId());
    if (!map) {
        return false;
    }
    
    double speed = GetDogSpeedForMap(player.GetMapId());
    model::Velocity new_velocity{0.0, 0.0};
    model::Direction new_direction = model::Direction::North;
    
    if (move == "L") {
        new_velocity = {-speed, 0.0};
        new_direction = model::Direction::West;
    } else if (move == "R") {
        new_velocity = {speed, 0.0};
        new_direction = model::Direction::East;
    } else if (move == "U") {
        new_velocity = {0.0, -speed};
        new_direction = model::Direction::North;
    } else if (move == "D") {
        new_velocity = {0.0, speed};
        new_direction = model::Direction::South;
    } else if (move == "") {
        new_velocity = {0.0, 0.0};
        new_direction = dog->GetDirection();
    } else {
        return false;
    }
    
    dog->SetVelocity(new_velocity);
    dog->SetDirection(new_direction);
    
    return true;
}

const model::Player* Application::FindPlayerByToken(const std::string& auth_token) {
    const auto& index = game_.GetTokenToPlayerIndex();
    auto it = index.find(auth_token);
    if (it == index.end()) {
        return nullptr;
    }
    
    const auto& players = game_.GetPlayers();
    if (it->second >= players.size()) {
        return nullptr;
    }
    
    return &players[it->second];
}

const model::Map* Application::FindMap(const model::Map::Id& id) const {
    return game_.FindMap(id);
}

model::Dog* Application::FindDog(const model::Dog::Id& id) {
    auto& index = game_.GetDogIdToIndex();
    auto it = index.find(id);
    if (it == index.end()) {
        return nullptr;
    }
    
    auto& dogs = game_.GetDogs();
    if (it->second >= dogs.size()) {
        return nullptr;
    }
    
    return &dogs[it->second];
}

double Application::GetDogSpeedForMap(const model::Map::Id& map_id) const {
    const auto* map = FindMap(map_id);
    if (!map) {
        return 1.0; 
    }
    
    return map->GetDogSpeed();
}

} // namespace app