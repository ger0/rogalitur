#include "map.hpp"
#include <algorithm>
#include <cstdint>
#include <unordered_set>

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
	N_Kernel kernel;
	Vec2u position;
	float 	entropy = FLT_MAX;
	Arr<float, TILE_MAX> weights;
	u32 	total_weight;
};

Map generate_map(const u16 width, const u16 height) {
	// Vec<Tile_Entry> tiles;
	Map map(width, height);
	srand(time(NULL));

	auto get_idx = [width, height](u16 x, u16 y) {
		return width * y + x;
	};

	for (u16 w = 0; w < width; w++) {
		for (u16 h = 0; h < height; h++) {
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

	// TODO: Refactor using a different way of storing rotation data
	auto calc_weights = [&map, get_idx](Tile_Entry& neighbours) {
		for (u16 c_id = 0; c_id < CANDIDATE_PRESETS.size(); c_id++) {
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
	auto get_neighbour_info = [&map, get_idx, calc_weights](Vec2u pos) -> std::tuple<Tile_Entry, Vec<Vec2u>> {
		N_Kernel kernel;
		Vec<Vec2u> unk_positions;
		u16 i = 0;
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
		Tile_Entry cell {
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

    auto compare = [&](Tile_Entry const& lhs, Tile_Entry const& rhs) {
        return lhs.entropy > rhs.entropy;
    };

	Vec<Tile_Entry> unknowns;

	auto sort_unknowns = [&unknowns, &compare]() {
		std::sort(unknowns.begin(), unknowns.end(), compare);
	};

	// spawn
	u16 start_x = width / 2;
	u16 start_y = height / 2;
	map.tiles[get_idx(start_x, start_y)] = Tile::Empty;

	// get neighbouring cells' positions
	auto [_, unk_positions] = get_neighbour_info({start_x, start_y});
	auto get_idx_vec2u = [&get_idx](const Vec2u vec2) {
		return get_idx(vec2.x, vec2.y);
	};
	auto vec2u_equal = [](const Vec2u lhs, const Vec2u rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y;
	};
	std::unordered_set<
		Vec2u, 
		decltype(get_idx_vec2u), 
		decltype(vec2u_equal)
	> next_unknown_cells(unk_positions.size(), get_idx_vec2u, vec2u_equal);

	for (const auto& pos : unk_positions) {
		auto [cell, next_neigh_poses] = get_neighbour_info(pos);
		unknowns.push_back(cell);
		// adding neighbouring positions to a list
		// vec_append(next_unknown_cells, next_neigh_poses);
		next_unknown_cells.insert(next_neigh_poses.begin(), next_neigh_poses.end());
	}

    while (!unknowns.empty()) {
		sort_unknowns();

		Tile_Entry current = unknowns.back();
		unknowns.pop_back();
		if (map.tiles[get_idx_vec2u(current.position)] != Tile::Unknown) {
			LOG_ERR("PROBLEM!!!");
		}

		// set the tile
		int choice = rand() % current.total_weight;
		for (i32 i = 0; i < current.weights.size(); i++) {
			choice -= current.weights[i];
			if (choice > 0) {
				continue;
			}

			// set choice 
			const auto& pos = current.position;
			map.tiles[get_idx_vec2u(pos)] = (Tile)i;
			LOG_DBG("UNKNOWN TILE AT POS: {}, {} HAS CHOSEN: {}", current.position.x, current.position.y, i);

			// update existing awaiting unknown tiles neighbouring the new chosen tile
			// TODO: OPTIMIZE BY USING AN ARRAY INSTEAD
			for (auto& it : unknowns) {
				i32 off_x = it.position.x - current.position.x;
				i32 off_y = it.position.y - current.position.y;

				// HARDCODED!
				if (off_x >= -1 && off_x <= 1 && off_y >= -1 && off_y <= 1) {
					auto [new_info, _] = get_neighbour_info(it.position);
					it = new_info;
					LOG_DBG(" 	Updated entropy and weights on pos: {}, {}", 
						new_info.position.x,
						new_info.position.y);
				}
			}

			// TODO: REFACTOR WTF
			Vec<Vec2u> new_positions;
			Vec<Tile_Entry> new_unknowns;
			// add positions to unknowns list for the next iteration
			for (const auto& next_neighbour : next_unknown_cells) {
				if (map.tiles[get_idx_vec2u(next_neighbour)] != Tile::Unknown) {
					continue;
				}
				auto [new_info, new_unk_poses] = get_neighbour_info(next_neighbour);
				// next_unknown_cells.erase(new_info.position);
				if (new_info.total_weight == 0) {
					LOG_ERR("WEIGHT CANNOT BE EQUAL TO 0!!!");
					continue;
				}
				LOG_DBG(" 	Added a new cell to unkowns list on pos: {}, {}", 
						new_info.position.x,
						new_info.position.y);
				new_unknowns.push_back(new_info);
				vec_append(new_positions, new_unk_poses);
			}
			next_unknown_cells.clear();
			next_unknown_cells.insert(new_positions.begin(), new_positions.end());
			vec_append(unknowns, new_unknowns);
			break;
		}
    }
	return map;
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
