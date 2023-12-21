#ifndef RGL_MAP_HPP
#define RGL_MAP_HPP

#include "types_utils.hpp"
#include <SDL2/SDL_render.h>
#include <vector>

struct Map {
    const u32 width;
    const u32 height;

    std::vector<u32> data;
    u32 at(const Vec2i pos) const;

    bool load_data(std::string filename);
    void iterate();
    Map(SDL_Surface &surf);
};

#endif // RGL_MAP_HPP
