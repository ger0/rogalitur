#include "map.hpp"
#include <cassert>
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
using N_indices = Arr<u8, NEIGHBR_NUM>;
using N_kernel 	= Arr<Tile, NEIGHBR_NUM>;

constexpr Arr<N_indices, ROTATION_MAX> ROTATION_LOOKUP_INDICES {
	// 0   deg:
	N_indices{0,1,2,3,4,5,6,7},
	// 90  deg:
	N_indices{2,4,7,1,6,0,3,5},
	// 180 deg:
	N_indices{7,6,5,4,3,2,1,0},
	// 270 deg:
	N_indices{5,3,0,6,1,7,4,2}
};

struct WFC_pattern {
	const char* id;
	Tile tile;
	char debug;
	u16  weight;
	N_kernel neighbours;
	Arr<Rotation, ROTATION_MAX> rotations;
};

using Candidate_id = u16;
constexpr Arr<WFC_pattern, 4> CANDIDATE_PRESETS = {{
	{
		.id 	= "Corner",
		.tile 	= Empty,
		.debug 	= '#',
		.weight = 5,
		.neighbours = {
			Wall, Wall,  Wall,
			Wall,        Empty,
			Wall, Empty, Empty
		},
		.rotations = {
			ROTATION_0, ROTATION_90, ROTATION_180, ROTATION_270
		}
	},
	{
		.id 	= "Wall",
		.tile 	= Wall,
		.debug  = '#',
		.weight = 10,
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
		.id 	= "Full_Wall",
		.tile 	= Wall,
		.debug  = '#',
		.weight = 50,
		.neighbours = {
			Wall, Wall, Wall,
			Wall,       Wall,
			Wall, Wall, Wall
		},
		.rotations = {}
	},
	{
		.id 	= "Room",
		.tile 	= Empty,
		.debug  = ' ',
		.weight = 70,
		.neighbours = {
			Empty, Empty, Empty,
			Empty,        Empty,
			Empty, Empty, Empty
		},
		.rotations = {}
	},
}};

// neighbour 
struct Tile_entry {
	Tile 		tile;
	float 		entropy = NAN;
	u32 		total_weight;
	Arr<float, TILE_MAX> weights;
};

struct Map_impl {
    const u32 width;
    const u32 height;
    Vec<Tile> data;

	u32 get_idx(u16 x, u16 y) {
		return width * y + x;
	};

	u32 get_idx_vec2u(const Vec2u vec2) {
		return get_idx(vec2.x, vec2.y);
	};

	std::unordered_set<Vec2u, Vec2u> next_tainted_cells;

    Tile at(Vec2u pos) const;

    Map_impl(Vec2u dimensions, Vec2u starting_pos);
	N_kernel get_neighbour_kernel(Vec2u pos, Vec<Tile_entry>& tiles);
	void calc_weights(Tile_entry& cell, N_kernel& kernel);
	void calc_cell_info(Vec2u pos, Vec<Tile_entry>& tiles);
};

// TODO: Refactor using a different way of storing rotation data
void Map_impl::calc_weights(Tile_entry& cell, N_kernel& kernel) {
	if (cell.tile != Tile::Unknown) {
		return;
	}
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
				/* LOG_DBG("Added weight {}, {} for {}", cell.weights[candidate.tile],
						cell.total_weight, (u32)candidate.tile); */
			}
		}
	}
};

N_kernel Map_impl::get_neighbour_kernel(Vec2u pos, Vec<Tile_entry>& tiles) {
	N_kernel kernel;
	u16 i = 0;
	// add neighbouring cells to the set of tainted cells (positions of the cells)
	for (i32 h = 1; h >= -1; h--) {
		for (i32 w = -1; w <= 1; w++, i++) {
			auto n_pos = Vec2u{pos.x + w, pos.y + h};
			if (h == 0 && w == 0) {
				i--;
				continue;
			}
			if (n_pos.x <= 0 || n_pos.x >= width 
					|| n_pos.y <= 0 || n_pos.y >= height) {
				kernel[i] = Tile::Wall;
				continue;
			}
			auto& n_tile = tiles.at(get_idx(n_pos.x, n_pos.y)).tile;
			kernel[i] = n_tile;
			if (n_tile == Tile::Unknown) {
				// LOG_DBG(" 	Tainted at: {}, {}", pos.x + w, pos.y + h);
				next_tainted_cells.insert(n_pos);
			}
		}	
	}
	return kernel;
}

void Map_impl::calc_cell_info(Vec2u pos, Vec<Tile_entry>& tiles) {
	auto& cell = tiles.at(get_idx_vec2u(pos));
	N_kernel kernel = get_neighbour_kernel(pos, tiles);
	calc_weights(cell, kernel);
	// calc entropy for the weights
	float entropy = 0.f;
	for (const auto& weight : cell.weights) {
		if (weight == 0) continue;
		const auto probability = weight / cell.total_weight;
		entropy -= probability * log2(probability);
		/* LOG_DBG("prob: {}, log: {}, total: {}, entropy: {}", 
				probability, log2(probability), cell.total_weight, cell.entropy); */
	}
	cell.entropy = entropy;
	//LOG_DBG("Updated cell at: {}, {}; with entropy {}", pos.x, pos.y, cell.entropy);
};

Map_impl::Map_impl(Vec2u dim, Vec2u start_pos): height(dim.x), width(dim.y) {
	this->data.resize(width * height);
	Vec<Tile_entry> tiles(width * height);
	srand(time(NULL));

	for (u16 w = 0; w < width; w++) {
		for (u16 h = 0; h < height; h++) {
			// Wall boundary around the map
			if (w == 0 or w == (width - 1) 
					or h == 0 or h == (height -1)) {
				tiles.at(get_idx(w, h)).tile = Tile::Wall;
			}
			// Otherwise unknown
			else {
				tiles.at(get_idx(w, h)).tile = Tile::Unknown;
			}
		}
	}

	// spawn
	tiles.at(get_idx(start_pos.x, start_pos.y)).tile = Tile::Empty;

	// get neighbouring cells' positions
	get_neighbour_kernel(start_pos, tiles);

	auto calc_tainted_cells = [&]() {
		auto tainted_cells(std::move(next_tainted_cells));
		next_tainted_cells.clear();
		for (const auto& cell_pos : tainted_cells) {
			calc_cell_info(cell_pos, tiles);
		}
	};

	Vec2u current_pos;
	auto setup_lowest_entropy = [&]() -> bool {
		calc_tainted_cells();
		float lowest_entropy = FLT_MAX;
		for (u32 y = 0; y < height; y++) {
			for (u32 x = 0; x < width; x++) {
				const auto& cell = tiles.at(get_idx(x, y));
				if (isnan(cell.entropy)) { 
					continue;
				}
				/* LOG_DBG("ENTROPY: {}, TILE: {}, WEIGHTS: {}", 
						cell.entropy, 
						(u32)cell.tile, 
						fmt::join(cell.weights, ", ")); */
				if (cell.entropy < lowest_entropy && cell.tile == Tile::Unknown) {
					lowest_entropy = cell.entropy;
					current_pos.x = x;
					current_pos.y = y;
				}
			}
		}
		return lowest_entropy != FLT_MAX;
	};

	u32 iter = 0;
    while (setup_lowest_entropy()) {
    	auto& cell = tiles.at(get_idx_vec2u(current_pos));
		/* LOG_DBG("TILE AT POS: {}, {}", current_pos.x, current_pos.y);
		LOG_DBG(" 	entropy: {}, total_weight: {}", cell.entropy, cell.total_weight); */
		if (cell.tile != Tile::Unknown) {
			LOG_ERR("ILLEGAL STATE DETECTED!");
			assert(false);
		}

		// set the tile
		int choice = rand() % cell.total_weight;
		for (i32 i = 0; i < cell.weights.size(); i++) {
			choice -= cell.weights[i];
			if (choice > 0) {
				continue;
			}
			// set choice 
			cell.tile = (Tile)i;
			break;
		}
		// LOG_DBG(" 	 	HAS CHOSEN: {}", (u32)cell.tile);
		iter++;
    }

	// output the result
	for (u16 i = 0; i < height * width; i++) {
		this->data[i] = tiles[i].tile;
		if (i % (width) == 0) {
			printf("\n");
		}
		u32 sym = tiles.at(i).tile;
		printf("%lu ", sym);
	}
	LOG_DBG("SPAWN : {}", (u32)data[get_idx_vec2u(start_pos)]);
	LOG_DBG("SPAWN POS: {} {}", 100.f * start_pos.x / (float)width, 100.f * start_pos.y / (float)height);
}

Tile Map_impl::at(Vec2u pos) const {
	if (pos.x <= 0 || pos.x >= width 
		|| pos.y <= 0 || pos.y >= height) {
		return Tile::Wall;
	} else {
		return data[pos.x + (width * pos.y)];
	}
}

// TODO: Refactor
Map::Map(Vec2u dim, Vec2u spawn_pos): width(dim.x), height(dim.y) {
	auto impl = Map_impl(dim, spawn_pos);
	this->tiles = impl.data;
}

Tile Map::at_pos(Position pos) const {
	Vec2u n_pos {
		.x = (u16)(Position::MAX * pos.x / this->width),
		.y = (u16)(Position::MAX * pos.y / this->height)
	};
	return this->at(n_pos);
}

Tile Map::at(Vec2u pos) const {
	if (pos.x <= 0 || pos.x >= width 
		|| pos.y <= 0 || pos.y >= height) {
		return Tile::Wall;
	} else {
		return this->tiles.at(pos.x + (width * pos.y));
	}
}
