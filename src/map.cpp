#include "map.hpp"
#include <cstdint>
#include <memory>
#include <cstdlib>
#include <random>

constexpr u32 NEIGHBR_NUM = 8;

enum Rotation {
    ROTATION_0 = 0,
    ROTATION_90 = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3,
    ROATATION_MAX
};

struct Tile_Entry {
	const char* id;
	Tile tile;
	char debug;
	float weight;
	Arr<Tile, NEIGHBR_NUM> neighbours;
	Arr<Rotation, Rotation::ROATATION_MAX> rotations;
};

/*
 * +---+---+---+
 * |   |   |   |
 * +---+---+---+
 * |   | x |   |
 * +---+---+---+
 * |   |   |   |
 * +---+---+---+
 */

constexpr Arr<Tile_Entry, Tile::Unknown> tile_weights = {{
	{
		.id 	= "Corner",
		.tile 	= Wall,
		.debug 	= '#',
		.weight = 20.f,
		.neighbours = {
			Wall, Wall, Wall,
			Wall,       Wall,
			Wall, Wall, Empty
		},
		.rotations = {
			ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270
		}
	},
	{
		.id 	= "Wall",
		.tile 	= Wall,
		.debug  = '#',
		.weight = 30.f,
		.neighbours = {
			Wall, Wall, Empty,
			Wall,       Empty,
			Wall, Wall, Empty
		},
		.rotations = {
			ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270
		}
	},
	{
		.id 	= "Room",
		.tile 	= Empty,
		.debug  = ' ',
		.weight = 30.f,
		.neighbours = {
			Empty, Empty, Empty,
			Empty,        Empty,
			Empty, Empty, Empty
		},
		.rotations = {}
	},
}};

Map generate_map(const u32 width, const u32 height) {
	Map map(width, height);

	auto get_idx = [width, height](u32 x, u32 y) {
		return width * y + x;
	};

	for (u32 w = 0; w < width; w++) {
		for (u32 h = 0; h < height; h++) {
			// Wall boundary around the map
			if (w == 0 or w == (width - 1) 
					or h == 0 or h == (height -1)) {
				map.tiles[get_idx(w, h)] = Tile::Wall;
			}
			// Otherwise unknown
			else {
				map.tiles[get_idx(w, h)] = Tile::Unknown;
			}
		}
	}

	// neighbour 
	struct Neigh_Entry {
		u32 	idx;
		Tile 	tile;
		float 	entropy = FLT_MAX;
	};

	auto get_neighbours = [&map, get_idx](u32 x, u32 y) {
		Vec<Neigh_Entry> retval(NEIGHBR_NUM);
		for (i32 w = -1; w <= 1; w++) {
			for (i32 h = -1; h <= 1; h++) {
				retval.push_back({
					get_idx(x + w, y + h),
					map.tiles[get_idx(x + w, y + h)]
				});
			}	
		}
		return retval;
	};

	u32 start_x = width / 2;
	u32 start_y = height / 2;
	map.tiles[get_idx(start_x, start_y)] = Tile::Empty;

	// calculate entropy
	auto todo_list = get_neighbours(start_x, start_y);

	// calc entropy

	return map;
}

// DEBUGGING 
void Map::iterate() {
    LOG_DBG("Iterating over map's data");
    u64 i = 0;
    /* for (const u32& item : data) {
        printf("%1lu,", item / 0xFFFFFFFFFFFFFFFF);
    	i++;
        if (i % this->width == 0) printf("\n");
    } */
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
        	// printf("%1lu,", at(Position{(float)x, (float)y}) / 0xFFFFFFFFFFFFFFFF);
        	printf("%1u,", at(Position{(float)x, (float)y}) > 0);
		}
        printf("\n");
	}
}

u32 Map::at(Position pos) const {
	auto c_pos = pos;	
	c_pos.x = pos.x / cell_width;
	c_pos.y = pos.y / cell_height;

	if (c_pos.x <= 0 || c_pos.y <= 0 || c_pos.x >= width || c_pos.y >= height) {
		return UINT32_MAX;
	}
	return this->data[c_pos.x + c_pos.y * width];
}

void Map::set(Vec2u pos, Tile tile) {
	this->data[pos.x + pos.y * width] = tile;
}

// todo: remove 
Map::Map(SDL_Surface &surf, u32 window_w, u32 window_h): 
		width(surf.w), 
		height(surf.h), 
		cell_width(window_w / (float)surf.w), 
		cell_height(window_h / (float)surf.h) {
	SDL_LockSurface(&surf);
    defer {
        SDL_UnlockSurface(&surf);
    };
    byte size = surf.format->BytesPerPixel;
    data.resize(surf.w * surf.h);
	memcpy(data.data(), surf.pixels, width * height * size);
	LOG("{} {} {} {}", width, height, cell_width, cell_height);
}

Map::Map(u32 width, u32 height): height(height), width(width) {
	this->tiles.resize(width * height);
}

Map::Map(u32 width, u32 height, u32 window_w, u32 window_h): 
	width(width), 
	height(height), 
	cell_width(window_w  / (float)width), 
	cell_height(window_h / (float)height) {
	tiles.resize(width * height);
}

void Map::generate() {
	this->set({0, 0}, Tile::Wall);
	for (u32 y = 0; y < height; y++) {
		for (u32 x = 0; x < width; x++) {
		}
	}
}
