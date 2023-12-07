#ifndef RGL_ENTITY_HPP
#define RGL_ENTITY_HPP

#include "map.hpp"
#include "types_utils.hpp"

using ID = u32;
using FLAGS = byte;

constexpr byte PLAYER_FLAG  = 0b00000001;
constexpr byte PHYSICS_FLAG = 0b00000010;
constexpr byte RENDER_FLAG  = 0b00000100;

struct Entity {
    ID      id;
    FLAGS   flags = 0;
};

Entity create_new_entity(FLAGS flags);

#endif // RGL_ENTITY_HPP

