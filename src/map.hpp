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

struct Position {
    float x = 0.f;
    float y = 0.f;
    static constexpr float MAX = 100.f;
};

struct Map {
    const u16 width, height;
    Vec<Tile> tiles;
    Map(u16 width, u16 height);

    Tile at(Vec2u tile_pos) const;
    Tile at_pos(Position pos) const;
};

#endif // RGL_MAP_HPP
