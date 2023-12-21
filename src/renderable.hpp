#ifndef RGL_RENDERBL_HPP
#define RGL_RENDERBL_HPP

#include "types_utils.hpp"
#include <SDL2/SDL_render.h>
#include <vector>

struct Renderable {
    Vec2i pos;
    Vec2i bnd;
    std::vector<SDL_Texture*> sprites;

    void add_sprite(SDL_Renderer *renderer, const char* filename);
    ~Renderable();
};

#endif // RGL_RENDERBL_HPP
