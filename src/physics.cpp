#include "types_utils.hpp"
#include "physics.hpp"

constexpr int max_step = 14;

void update_move(Physics &comp, Direction dir, MoveType type) {
    if (type == MoveType::move) {
        switch (dir) {
            case left:
                comp.dir = left;
                break;

            case right:
                comp.dir = right;
                break;

            case jump:
                if (comp.loc == ground) {
                    LOG_DBG("JUMPED!");
                    comp.vel.y = comp.accel.y;
                    comp.loc = air;
                }
                break;
            default:
                comp.dir = none;
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

            default: 
                break;
        }
    }
}

enum Collision_Type {
    NONE = false,
    COLLISION
};

// check for collisions between a bytearray and a rectangle
inline Collision_Type check_collision(const Map& map, const Position &pos)  {
    //LOG_DBG("Real Position: {}, {}", pos.x, pos.y);
    const auto pix = map.at(pos);
    if (pix != Tile::Empty) {
        return COLLISION;
    }
    return NONE;
}

void update_tick(Physics &comp, const Map &map) {
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
        default:
            if (vel.x > -0.001f && vel.x < 0.001f) {
                vel.x = 0.f;
            } else {
                if (comp.loc == air)    vel.x *= 0.987f;
                else                    vel.x *= 0.96f;
            }
            break;
    };

    if (comp.loc == air) {
        vel.y -= accel.g;
    }
    Position new_pos = comp.pos;

    new_pos.x += vel.x;
    new_pos.y += vel.y;

    // calc collisions
    Collision_Type collision = check_collision(map, new_pos);
    // LOG_DBG("Coll type: {}", collision);
    if (collision == Collision_Type::COLLISION) {
        if (comp.loc == Location::air) {
            comp.vel.y = 0;
            comp.loc = Location::ground;
            LOG_DBG("Landed on the ground!");
        } else {
            for (u32 off_y = -max_step; off_y <= max_step; off_y++) {
                new_pos.y += off_y;
                collision = check_collision(map, new_pos);
                if (collision == Collision_Type::NONE) {
                    comp.pos.x = new_pos.x;
                    comp.pos.y = new_pos.y;
                    comp.vel = vel;
                    comp.loc = Location::air;
                    LOG_DBG("Flying!");
                    return;
                }
            }
            comp.vel.x = 0.f;
        }
    } else {
        comp.vel = vel;
        comp.pos.x = new_pos.x;
        comp.pos.y = new_pos.y;
    }
    // LOG_DBG("{} {}, {} {}", pos.x, pos.y, new_pos.x, new_pos.y);
}
