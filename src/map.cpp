#include "map.hpp"
#include <cstdint>
#include <queue>

constexpr u32 NEIGHBR_NUM = 8;

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

struct Tile_Entry {
	const char* id;
	Tile tile;
	char debug;
	u32  weight;
	N_Kernel neighbours;
	Arr<Rotation, ROTATION_MAX> rotations;
};

using Candidate_Id = u32;
constexpr Arr<Tile_Entry, 5> CANDIDATE_PRESETS = {{
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

Map generate_map(const u32 width, const u32 height) {
	Map map(width, height);
	srand(time(NULL));

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
		N_Kernel kernel;
		Vec2u 	position;
		float 	entropy = FLT_MAX;
		Arr<float, TILE_MAX> weights;
		u32 	total_weight;
	};

	// TODO: Refactor using a different way of storing rotation data
	auto calc_weights = [&map, get_idx](Neigh_Entry& neighbours) {
		for (u32 c_id = 0; c_id < CANDIDATE_PRESETS.size(); c_id++) {
			const auto& candidate = CANDIDATE_PRESETS.at(c_id);
			for (const auto& c_rotation : candidate.rotations) {
				const auto& c_indices = ROTATION_LOOKUP_INDICES.at(c_rotation);
				// index for default rotation
				bool is_fitting = true;
				u8 lhs_idx = 0;
				for (const u8& rhs_idx : c_indices) {
					const auto& tile = neighbours.kernel.at(lhs_idx);
					if (tile == Tile::Unknown) {
						continue;
					}
					else if (tile != candidate.neighbours.at(lhs_idx)) {
						is_fitting = false;
					}
					lhs_idx++;
				}
				if (is_fitting) {
					neighbours.weights[candidate.tile] += candidate.weight;
					neighbours.total_weight += candidate.weight;
				}
			}
		}
	};

	// TODO: Add boundary checking
	auto get_neighbour_info = [&map, get_idx, calc_weights](Vec2u pos) -> std::tuple<Neigh_Entry, Vec<Vec2u>> {
		N_Kernel kernel;
		Vec<Vec2u> unk_positions;
		u32 i = 0;
		for (i32 h = 1; h >= -1; h--) {
			for (i32 w = -1; w <= 1; w++) {
				const auto& tile = map.tiles.at(get_idx(pos.x + w, pos.y + h));
				kernel[i] = tile;
				if (tile == Tile::Unknown) {
					unk_positions.push_back({pos.x + w, pos.y + h});
				}
				i++;
			}	
		}
		Neigh_Entry cell {
			.kernel = kernel,
			.position = pos,
		};
		calc_weights(cell);
		// calc entropy for the weights
		for (const auto& weight : cell.weights) {
			if (weight == 0) continue;
			const auto probability = weight / cell.total_weight;
			cell.entropy -= probability * log2(probability);
		}
		return std::make_tuple(cell, unk_positions);
	};

    auto compare = [&](Neigh_Entry const& lhs, Neigh_Entry const& rhs) {
        return lhs.entropy > rhs.entropy;
    };

    std::priority_queue<
        Neigh_Entry,
        std::vector<Neigh_Entry>,
        decltype(compare)
    > unknowns(compare);

	// spawn
	u32 start_x = width / 2;
	u32 start_y = height / 2;
	map.tiles[get_idx(start_x, start_y)] = Tile::Empty;

	// get neighbouring cells' positions
	auto [_, unk_positions] = get_neighbour_info({start_x, start_y});
	Vec<Vec2u> next_unknown_cells;

	for (const auto& pos : unk_positions) {
		auto [cell, next_neigh_poses] = get_neighbour_info(pos);
		unknowns.push(cell);
		// adding neighbouring positions to a list
		next_unknown_cells.insert(
			next_unknown_cells.end(),
			next_neigh_poses.begin(),
			next_neigh_poses.end()
		);
	}

    while (!unknowns.empty()) {
		auto current = std::move(unknowns.top());
		unknowns.pop();

		// set the tile
		int choice = rand() % current.total_weight;
		for (i32 i = 0; i < current.weights.size(); i++) {
			choice -= current.weights[i];
			if (choice <= 0) {
				const auto& pos = current.position;
				map.tiles.at(get_idx(pos.x, pos.y)) = (Tile)i;
				LOG_DBG("Pos: {}, {} ; Chosen: {}", current.position.x, current.position.y, i);
				break;
			}
		}
    }
	return map;
}

// DEBUGGING 
void Map::iterate() {
    LOG_DBG("map's data");
	for (u32 i = 0; i < this->height * this->width; i++) {
		if (i % (this->width) == 0) {
			printf("\n");
		}
		printf("%c", this->tiles.at(i));
	}
}

u32 Map::at(Position pos) const {
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
