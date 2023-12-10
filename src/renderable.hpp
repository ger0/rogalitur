#ifndef RGL_RENDERBL_HPP
#define RGL_RENDERBL_HPP

#include "types_utils.hpp"
#include <SDL2/SDL_rect.h>

struct Renderable {
    Vec2i pos;
    Vec2i bnd;
    // Sprite?
    // std::vector<Animation_Sprites> ani_sprites;
};

#endif // RGL_RENDERBL_HPP
