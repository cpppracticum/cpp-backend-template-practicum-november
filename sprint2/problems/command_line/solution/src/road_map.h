#pragma once

#include "model.h"
#include <vector>

namespace model {

class RoadMap {
public:
    explicit RoadMap(const Map& map) {
        for (const auto& road : map.GetRoads()) {
            roads_.push_back(&road);
        }
    }
    
    const Road* FindRoadAtPoint(const Point& point) const {
        const double epsilon = 0.4;
        
        for (const auto* road : roads_) {
            auto start = road->GetStart();
            auto end = road->GetEnd();
            
            if (road->IsHorizontal()) {
                if (std::fabs(point.y - start.y) <= epsilon &&
                    point.x >= std::min(start.x, end.x) - epsilon &&
                    point.x <= std::max(start.x, end.x) + epsilon) {
                    return road;
                }
            } else {
                if (std::fabs(point.x - start.x) <= epsilon &&
                    point.y >= std::min(start.y, end.y) - epsilon &&
                    point.y <= std::max(start.y, end.y) + epsilon) {
                    return road;
                }
            }
        }
        return nullptr;
    }

private:
    std::vector<const Road*> roads_;
};

}  // namespace model
