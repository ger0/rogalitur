#include "types_utils.hpp"
#include "player.hpp"

void Player::move(Direction m_dir, MoveType type) {
    if (type == MoveType::press) {
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
    } else if (type == MoveType::release) {
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
