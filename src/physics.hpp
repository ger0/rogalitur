#ifndef RGL_PHYSICS_HPP
#define RGL_PHYSICS_HPP

#include "entity.hpp"
#include "types_utils.hpp"
#include "map.hpp"

enum MoveType {
    move,
    stop,
};

// direction
enum Direction : byte {
    none,
    left,
    right,
    jump,
};

enum Location : byte {
    air,
    ground,
};

struct Physics {
    Position pos;
    struct Velocity {
        float   x = 0;
        float   y = 0;
    } vel;
    struct Acceleration {
        float   x = 1.f;
        float   y = 1.5f;
        float   g = 0.02f;
    } accel;

    Direction   dir;
    Location    loc;
};

void update_move(Physics &component, Direction dir, MoveType type);
void update_tick(Physics &comp, const Map &map);

#endif // RGL_PHYSICS_HPP
