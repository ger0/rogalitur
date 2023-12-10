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
                LOG_DBG("JUMPED!");
                comp.vel.y = comp.accel.y;
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

// check for collisions between a bytearray and a rectangle
bool check_collision(const Map& map, Vec2i &pos, Vec2i &bnd)  {
    for (int h = 0; h < bnd.y; h++) {
        for (int w = 0; w < bnd.x; w++) {
            const auto& pix = map.at(Vec2i{w + pos.x, h + pos.y});
            if (pix) return true;
        }
    }
    return false;
}

void update_tick(Physics &comp, const Map &map, Vec2i &pos, Vec2i &bnd) {
    // Handle movements:
    auto vel   = comp.vel;
    auto accel = comp.accel;

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

    if (comp.loc == air) {
        vel.y -= accel.g;
    }

    // calc collisions

    if (check_collision(map, pos, bnd)){
        if (comp.loc == air) { 
            LOG_DBG("LANDED ON GROUND!");
            comp.loc = ground;
            return;
        }
    }

    pos.x += vel.x;
    pos.y += vel.y;

    comp.vel = vel;
    comp.accel = accel;
}
