#pragma once
#include <string>

namespace model {

enum class Direction {
    North,  // U
    South,  // D
    West,   // L
    East    // R
};

inline std::string DirectionToString(Direction dir) {
    switch (dir) {
        case Direction::North: return "U";
        case Direction::South: return "D";
        case Direction::West:  return "L";
        case Direction::East:  return "R";
    }
    return "U";
}

}  // namespace model
