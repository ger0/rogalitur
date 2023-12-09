#include "types_utils.hpp"
#include "physics.hpp"

void update_move(Physics &comp, Direction dir, MoveType type) {
    if (type == MoveType::move) {
        switch (dir) {

        case left:
            if (dir == right) comp.dir = none;
            else    comp.dir = left;
            break;

        case right:
            if (dir == left)  comp.dir = none;
            else    comp.dir = right;
            break;

        case jump:
            if (comp.loc == ground) {
                comp.vel.y += comp.accel.y;
                comp.loc = air;
            }
            break;
        default:
            break;
        }
    } else if (type == MoveType::stop) {
        switch (dir) {

        case left:
            if (dir == left)  comp.dir = none;
            else    comp.dir = right;
            break;

        case right:
            if (dir == right) comp.dir = none;
            else    comp.dir = left;
            break;
        }
    }
}

void update_tick(Physics &comp) {
    // Handle movements:
    auto& vel   = comp.vel;
    auto& accel = comp.accel;
    auto& pos   = comp.pos;

    switch (comp.dir) {

    case left:
        vel.x = vel.x <= -accel.x ? -accel.x : vel.x - accel.x;
        break;
    case right:
        vel.x = vel.x <= accel.x ? accel.x : vel.x + accel.x;
        break;
    case none:
        if (vel.x > -0.01f && vel.x < 0.01f) {
            vel.x = 0.f;
        } else {
            vel.x *= 0.75f;
        }
        break;
    };
    pos.x += vel.x;
    pos.y += vel.y;
}
