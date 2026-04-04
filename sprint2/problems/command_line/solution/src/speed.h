#pragma once

namespace model {

struct Speed {
    double vx = 0.0;
    double vy = 0.0;
    
    Speed() = default;
    Speed(double vx_, double vy_) : vx(vx_), vy(vy_) {}
};

}  // namespace model
