#ifndef RGL_PLAYER_HPP
#define RGL_PLAYER_HPP

#include "types_utils.hpp"

enum MoveType {
    press,
    release,
};

struct Player {
    // position
    struct Position {
        int x = 0;
        int y = 0;
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
    
    // direction
    enum Direction : byte {
        left,
        right,
        none,
        jump,
    } dir;

    enum Location : byte {
        air,
        ground,
    } loc;

    void move(Direction m_dir, MoveType type);
};

#endif // RGL_PLAYER_HPP
