#include "entity.hpp"

void Entity::move(Direction m_dir, MoveType type) {
    if (type == MoveType::move) {
        switch (m_dir) {

        case left:
            if (this->dir == right) this->dir = none;
            else                    this->dir = left;
            break;

        case right:
            if (this->dir == left)  this->dir = none;
            else                    this->dir = right;
            break;

        case jump:
            if (this->loc == ground) {
                this->vel.y += this->accel.y;
                this->loc = air;
            }
            break;
        default:
            break;
        }
    } else if (type == MoveType::stop) {
        switch (m_dir) {

        case left:
            if (this->dir == left)  this->dir = none;
            else                    this->dir = right;
            break;

        case right:
            if (this->dir == right) this->dir = none;
            else                    this->dir = left;
            break;
        }
    }
}

void Entity::update_pos() {
    // Handle movements:
    switch (this->dir) {

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
