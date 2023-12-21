#include "renderable.hpp"
#include <SDL2/SDL_render.h>

void Renderable::add_sprite(SDL_Renderer *renderer, const char* filename) {
    auto tex = SDL_LoadBMP(filename);
    sprites.push_back(SDL_CreateTextureFromSurface(renderer, tex));
}

Renderable::~Renderable() {
    for (auto& texture: sprites) {
        SDL_DestroyTexture(texture);
    }
}
