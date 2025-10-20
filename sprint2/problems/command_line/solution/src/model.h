#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>

#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct Position {
    double x;
    double y;
};

struct Velocity {
    double vx;
    double vy;
};

enum class Direction {
    North,  
    South,  
    West,   
    East    
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(1.0) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    double GetDogSpeed() const noexcept {
        return dog_speed_;
    }

    void SetDogSpeed(double speed) {
        dog_speed_ = speed;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    Position GetRandomDogPosition() const {
        if (roads_.empty()) {
            return {0.0, 0.0};
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> road_dist(0, roads_.size() - 1);
        
        const auto& road = roads_[road_dist(gen)];
        auto start = road.GetStart();
        if (road.IsHorizontal()) {
            std::uniform_real_distribution<double> pos_dist(
                std::min(start.x, road.GetEnd().x),
                std::max(start.x, road.GetEnd().x)
            );
            return {pos_dist(gen), static_cast<double>(start.y)};
        } else {
            std::uniform_real_distribution<double> pos_dist(
                std::min(start.y, road.GetEnd().y),
                std::max(start.y, road.GetEnd().y)
            );
            return {static_cast<double>(start.x), pos_dist(gen)};
        }
    }
    
    Position GetDefaultDogPosition() const {
        if (roads_.empty()) {
            return {0.0, 0.0};
        }
        
        const auto& first_road = roads_[0];
        auto start = first_road.GetStart();
        return {static_cast<double>(start.x), static_cast<double>(start.y)};
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    Offices offices_;
    OfficeIdToIndex office_id_to_index_;
    double dog_speed_;
};

class Dog {
public:
    using Id = util::Tagged<uint32_t, Dog>;
    
    Dog(Id id, std::string name, Map::Id map_id, Position position)
        : id_(std::move(id))
        , name_(std::move(name))
        , map_id_(std::move(map_id))
        , position_(position)
        , velocity_{0.0, 0.0}
        , direction_(Direction::North) {
    }
    
    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const Map::Id& GetMapId() const noexcept { return map_id_; }
    Position GetPosition() const noexcept { return position_; }
    Velocity GetVelocity() const noexcept { return velocity_; }
    Direction GetDirection() const noexcept { return direction_; }
    
    void SetPosition(Position position) { position_ = position; }
    void SetVelocity(Velocity velocity) { velocity_ = velocity; }
    void SetDirection(Direction direction) { direction_ = direction; }

private:
    Id id_;
    std::string name_;
    Map::Id map_id_;
    Position position_;
    Velocity velocity_;
    Direction direction_;
};

class Player {
public:
    using Id = util::Tagged<uint32_t, Player>;
    
    Player(Id id, std::string name, Dog::Id dog_id, Map::Id map_id, std::string token)
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_id_(std::move(dog_id))
        , map_id_(std::move(map_id))
        , token_(std::move(token)) {
    }
    
    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const Dog::Id& GetDogId() const noexcept { return dog_id_; }
    const Map::Id& GetMapId() const noexcept { return map_id_; }
    const std::string& GetToken() const noexcept { return token_; }

private:
    Id id_;
    std::string name_;
    Dog::Id dog_id_;
    Map::Id map_id_;
    std::string token_;
};

class TokenGenerator {
public:
    std::string GenerateToken() {
        std::uniform_int_distribution<uint64_t> dist;
        uint64_t part1 = dist(generator1_);
        uint64_t part2 = dist(generator2_);
        
        std::stringstream ss;
        ss << std::hex << std::setfill('0') 
           << std::setw(16) << part1 
           << std::setw(16) << part2;
        return ss.str();
    }

private:
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
};

class Game {
public:
    using Maps = std::vector<Map>;

    Game() : default_dog_speed_(1.0) {}

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    std::vector<Dog>& GetDogs() { return dogs_; }
    std::vector<Player>& GetPlayers() { return players_; }
    
    std::unordered_map<std::string, size_t>& GetTokenToPlayerIndex() { return token_to_player_index_; }
    std::unordered_map<Player::Id, size_t, util::TaggedHasher<Player::Id>>& GetPlayerIdToIndex() { return player_id_to_index_; }
    std::unordered_map<Dog::Id, size_t, util::TaggedHasher<Dog::Id>>& GetDogIdToIndex() { return dog_id_to_index_; }

    Dog* FindDog(const Dog::Id& id) {
        auto it = dog_id_to_index_.find(id);
        return it != dog_id_to_index_.end() ? &dogs_.at(it->second) : nullptr;
    }

    double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }
    void SetDefaultDogSpeed(double speed) { default_dog_speed_ = speed; }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    
    std::vector<Dog> dogs_;
    std::vector<Player> players_;
    double default_dog_speed_;
    
    using TokenToPlayerIndex = std::unordered_map<std::string, size_t>;
    using PlayerIdToIndex = std::unordered_map<Player::Id, size_t, util::TaggedHasher<Player::Id>>;
    using DogIdToIndex = std::unordered_map<Dog::Id, size_t, util::TaggedHasher<Dog::Id>>;
    
    TokenToPlayerIndex token_to_player_index_;
    PlayerIdToIndex player_id_to_index_;
    DogIdToIndex dog_id_to_index_;
};

}  // namespace model