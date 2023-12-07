#include "entity.hpp"

static ID last_id = 0;

Entity create_new_entity(FLAGS flags) {
    last_id++;
    Entity entity;
    entity.id = last_id;
    entity.flags = flags;
    return entity;
}
