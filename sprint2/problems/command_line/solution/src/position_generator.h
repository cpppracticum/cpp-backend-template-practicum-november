#pragma once

#include "point.h"
#include "model.h"
#include <random>

namespace model {

class PositionGenerator {
public:
    PositionGenerator() : rng_(std::random_device{}()) {}

    void SetRandomize(bool randomize) { randomize_ = randomize; }

    Point GeneratePosition(const Map& map) {
        const auto& roads = map.GetRoads();
        if (roads.empty()) return Point(0, 0);
        
        if (randomize_) {
            std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
            const auto& road = roads[road_dist(rng_)];
            
            std::uniform_real_distribution<double> coord_dist(0.0, 1.0);
            double t = coord_dist(rng_);
            
            auto start = road.GetStart();
            auto end = road.GetEnd();
            
            if (road.IsHorizontal()) {
                double x = start.x + t * (end.x - start.x);
                return Point(x, start.y);
            } else {
                double y = start.y + t * (end.y - start.y);
                return Point(start.x, y);
            }
        } else {
            const auto& road = roads[0];
            auto start = road.GetStart();
            return Point(start.x, start.y);
        }
    }

private:
    std::mt19937 rng_;
    bool randomize_ = false;
};

}  // namespace model
