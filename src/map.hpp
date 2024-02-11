#ifndef RGL_MAP_HPP
#define RGL_MAP_HPP

#include "types_utils.hpp"
#include <SDL2/SDL_render.h>
#include <vector>

struct Position {
    float   x = 0.f;
    float   y = 0.f;
};

enum Tile: char {
    Wall    = 0,
    Empty   = 1,
    Stairs  = 2,
    Floor   = 3,
    Unknown = 4,
    TILE_MAX = Unknown
};

struct Map {
    const u32 width;
    const u32 height;

    float cell_width; 
    float cell_height; 

    std::vector<u32> data;
    // experimental
    std::vector<Tile> tiles;

    u32 at(Position pos) const;
    void set(Vec2u pos, Tile);

    // bool load_data(std::string filename);
    void iterate();
    // INIT BASED ON SDL_SURFACE
    Map(SDL_Surface &surf, u32 wndw_w, u32 wndw_h);
    Map(u32 width, u32 height, u32 wndw_w, u32 wndw_h);
    Map(u32 width, u32 height);
};

Map generate_map(u32 width, u32 height);

#endif // RGL_MAP_HPP
