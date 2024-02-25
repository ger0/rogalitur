#ifndef RGL_MAP_HPP
#define RGL_MAP_HPP

#include "types_utils.hpp"
#include <SDL2/SDL_render.h>

enum Tile: char {
    Empty = 0,
    Stairs,
    Floor,
    Wall,
    Unknown,
    TILE_MAX = Unknown 
};

struct Map {
    const u32 width;
    const u32 height;

    Tile at(Vec2u pos) const;
    
    Vec<Tile> data;
    void set(Vec2u pos, Tile);
    Map(u32 width, u32 height);
};

Map generate_map(u16 width, u16 height);

#endif // RGL_MAP_HPP
