#ifndef RGL_ENTITY_HPP
#define RGL_ENTITY_HPP

#include "types_utils.hpp"

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

class Entity {
public:
    void move(Direction m_dir, MoveType type);
    void update_pos();
    // position
    struct Position {
        int x = 0;
        int y = 400;
    } pos;
    // velocity
    struct Velocity {
        float x = 0;
        float y = 0;
    } vel;
    // acceleration
    struct Acceleration {
        float x = 2.f;
        float y = 5.f;
        float g = 2.f;
    } accel;

private:
    Direction dir;
    Location  loc;
};

#endif // RGL_ENTITY_HPP
