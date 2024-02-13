#include "map.hpp"
#include <cstdint>

constexpr u16 NEIGHBR_NUM = 8;

enum Rotation: byte {
    ROTATION_0 = 0,
    ROTATION_90 = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3,
    ROTATION_MAX
};

/*
 * +---+---+---+
 * | 0 | 1 | 2 |
 * +---+---+---+
 * | 3 |   | 4 |
 * +---+---+---+
 * | 5 | 6 | 7 |
 * +---+---+---+
 */

// indice ordering for each rotation state
using N_Indices = Arr<u8, NEIGHBR_NUM>;
using N_Kernel 	= Arr<Tile, NEIGHBR_NUM>;

constexpr Arr<N_Indices, ROTATION_MAX> ROTATION_LOOKUP_INDICES {
	// 0   deg:
	N_Indices{0,1,2,3,4,5,6,7},
	// 90  deg:
	N_Indices{2,4,7,1,6,0,3,5},
	// 180 deg:
	N_Indices{7,6,5,4,3,2,1,0},
	// 270 deg:
	N_Indices{5,3,0,6,1,7,4,2}
};

struct WFC_Pattern {
	const char* id;
	Tile tile;
	char debug;
	u16  weight;
	N_Kernel neighbours;
	Arr<Rotation, ROTATION_MAX> rotations;
};

using Candidate_Id = u16;
constexpr Arr<WFC_Pattern, 5> CANDIDATE_PRESETS = {{
	{
		.id 	= "Wall_Corner",
		.tile 	= Wall,
		.debug 	= '#',
		.weight = 10,
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
		.id 	= "Empty_Corner",
		.tile 	= Empty,
		.debug 	= '#',
		.weight = 10,
		.neighbours = {
			Wall,  Wall,  Wall,
			Wall,         Empty,
			Wall,  Empty, Empty
		},
		.rotations = {
			ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270
		}
	},
	{
		.id 	= "Wall",
		.tile 	= Wall,
		.debug  = '#',
		.weight = 20,
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
		.id 	= "Empty_Wall",
		.tile 	= Empty,
		.debug  = '#',
		.weight = 20,
		.neighbours = {
			Wall, Empty, Empty,
			Wall,        Empty,
			Wall, Empty, Empty
		},
		.rotations = {
			ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270
		}
	},
	{
		.id 	= "Room",
		.tile 	= Empty,
		.debug  = ' ',
		.weight = 30,
		.neighbours = {
			Empty, Empty, Empty,
			Empty,        Empty,
			Empty, Empty, Empty
		},
		.rotations = {}
	},
}};

// neighbour 
struct Tile_Entry {
	Tile 		tile;
	bool 		tainted = false;
	float 		entropy = FLT_MAX;
	u32 		total_weight;
	Arr<float, TILE_MAX> weights;
};

void generate_map(const u16 width, const u16 height) {
	Vec<Tile_Entry> tiles(width * height);
	srand(time(NULL));

	auto get_idx = [width, height](u16 x, u16 y) {
		return width * y + x;
	};

	for (u16 w = 0; w < width; w++) {
		for (u16 h = 0; h < height; h++) {
			// Wall boundary around the map
			if (w == 0 or w == (width - 1) 
					or h == 0 or h == (height -1)) {
				tiles[get_idx(w, h)].tile = Tile::Wall;
			}
			// Otherwise unknown
			else {
				tiles[get_idx(w, h)].tile = Tile::Unknown;
			}
		}
	}

	// TODO: Refactor using a different way of storing rotation data
	auto calc_weights = [&](Tile_Entry& cell, N_Kernel& kernel) {
		for (u16 c_id = 0; c_id < CANDIDATE_PRESETS.size(); c_id++) {
			const auto& candidate = CANDIDATE_PRESETS.at(c_id);
			for (const auto& c_rotation : candidate.rotations) {
				const auto& c_indices = ROTATION_LOOKUP_INDICES.at(c_rotation);
				// index for default rotation
				bool is_fitting = true;
				u8 lhs_idx = 0;
				for (const u8& rhs_idx : c_indices) {
					const auto& tile = kernel.at(lhs_idx);
					if (tile == Tile::Unknown) {
						continue;
					}
					else if (tile != candidate.neighbours.at(lhs_idx)) {
						is_fitting = false;
						break;
					}
					lhs_idx++;
				}
				if (is_fitting) {
					cell.weights[candidate.tile] += candidate.weight;
					cell.total_weight += candidate.weight;
				}
			}
		}
	};

	auto get_idx_vec2u = [&get_idx](const Vec2u vec2) {
		return get_idx(vec2.x, vec2.y);
	};

	// TODO: Add boundary checking
	auto calc_cell_info = [&](Vec2u pos) {
		auto& cell = tiles[get_idx_vec2u(pos)];
		N_Kernel kernel;
		u16 i = 0;
		for (i32 h = 1; h >= -1; h--) {
			for (i32 w = -1; w <= 1; w++) {
				auto& n_tile = tiles.at(get_idx(pos.x + w, pos.y + h));
				kernel[i] = n_tile.tile;
				if (n_tile.tile == Tile::Unknown) {
					if (n_tile.tainted == false) {
						LOG_DBG(" 	Tainted at: {}, {}", pos.x + w, pos.y + h);
					}
					n_tile.tainted = true;
				}
				i++;
			}	
		}

		cell.tainted = false;
		calc_weights(cell, kernel);

		// calc entropy for the weights
		for (const auto& weight : cell.weights) {
			if (weight == 0) continue;
			const auto probability = weight / cell.total_weight;
			cell.entropy -= probability * log2(probability);
		}
		LOG_DBG("Updated cell at: {}, {}; with entropy {}", pos.x, pos.y, cell.entropy);
	};

    auto compare = [&](Tile_Entry const& lhs, Tile_Entry const& rhs) {
        return lhs.entropy > rhs.entropy;
    };

	// spawn
	u16 start_x = width / 2;
	u16 start_y = height / 2;
	tiles[get_idx(start_x, start_y)].tile = Tile::Empty;

	// get neighbouring cells' positions
	calc_cell_info({start_x, start_y});
	// TODO: REMOVE
	auto vec2u_equal = [](const Vec2u lhs, const Vec2u rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y;
	};

	auto calc_tainted_cells = [&]() {
		for (u32 y = 0; y < height; y++) {
			for (u32 x = 0; x < width; x++) {
				calc_cell_info({x, y});
			}
		}
	};

	Vec2u current_pos;
	auto set_next_lowest_entropy = [&]() -> bool {
		float lowest_entropy = FLT_MAX;
		for (u32 y = 0; y < height; y++) {
			for (u32 x = 0; x < width; x++) {
				const auto& cell = tiles[get_idx(x, y)];
				LOG_DBG("ENTROPY: {}, TILE: {}", cell.entropy, (u32)cell.tile);
				if (cell.entropy < lowest_entropy && cell.tile == Tile::Unknown) {
					lowest_entropy = cell.entropy;
					current_pos.x = x;
					current_pos.y = y;
				}
			}
		}
		if (lowest_entropy == FLT_MAX) {
			LOG_DBG("FOUND NO NEW TILES TO UPDATE");
			return false;
		} else {
			LOG_DBG("FOUND NEW LOWEST ENTROPY AT: {} {}; ENTR: {}", 
					current_pos.x, current_pos.y ,lowest_entropy);
			return true;
		}
	};

    while (set_next_lowest_entropy()) {
    	auto& cell = tiles[get_idx_vec2u(current_pos)];
		LOG_DBG("UNKNOWN TILE AT POS: {}, {}", current_pos.x, current_pos.y);
		LOG_DBG(" 	entropy: {}, total_weight: {}", cell.entropy, cell.total_weight);

		// set the tile
		int choice = rand() % cell.total_weight;
		for (i32 i = 0; i < cell.weights.size(); i++) {
			choice -= cell.weights[i];
			if (choice > 0) {
				continue;
			}
			// set choice 
			cell.tile = (Tile)i;
			LOG_DBG(" 	HAS CHOSEN: {}", i);
			break;
		}
		calc_tainted_cells();
    }

	for (u16 i = 0; i < height * width; i++) {
		if (i % (width) == 0) {
			printf("\n");
		}
		u32 sym = tiles.at(i).tile;
		printf("%lu", sym);
	}
}

// DEBUGGING 
void Map::iterate() {
    LOG_DBG("map's data");
	for (u16 i = 0; i < this->height * this->width; i++) {
		if (i % (this->width) == 0) {
			printf("\n");
		}
		printf("%c", this->tiles.at(i));
	}
}

u16 Map::at(Position pos) const {
	auto c_pos = pos;	
	c_pos.x = pos.x / cell_width;
	c_pos.y = pos.y / cell_height;

	if (c_pos.x <= 0 || c_pos.y <= 0 || c_pos.x >= width || c_pos.y >= height) {
		return UINT32_MAX;
	}
	return this->tiles[c_pos.x + c_pos.y * width];
}

void Map::set(Vec2u pos, Tile tile) {
	this->data[pos.x + pos.y * width] = tile;
}

// todo: remove 
Map::Map(SDL_Surface &surf, u16 window_w, u16 window_h): 
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

Map::Map(u16 width, u16 height): height(height), width(width) {
	this->tiles.resize(width * height);
}

Map::Map(u16 width, u16 height, u16 window_w, u16 window_h): 
	width(width), 
	height(height), 
	cell_width(window_w  / (float)width), 
	cell_height(window_h / (float)height) {
	tiles.resize(width * height);
}
