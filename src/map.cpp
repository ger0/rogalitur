#include "map.hpp"
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
constexpr Arr<WFC_Pattern, 4> CANDIDATE_PRESETS = {{
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
struct Tile_Entry {
	Tile 		tile;
	float 		entropy = NAN;
	u32 		total_weight;
	Arr<float, TILE_MAX> weights;
};

static u16 width, height;

u32 get_idx(u16 x, u16 y) {
	return width * y + x;
};

u32 get_idx_vec2u(const Vec2u vec2) {
	return get_idx(vec2.x, vec2.y);
};
bool vec2u_equal(const Vec2u lhs, const Vec2u rhs) {
	return lhs.x == rhs.x && lhs.y == rhs.y;
};

std::unordered_set<
	Vec2u, 
	decltype(&get_idx_vec2u), //get_idx_vec2u
	decltype(&vec2u_equal)
>next_tainted_cells(1, get_idx_vec2u, vec2u_equal);


// TODO: Refactor using a different way of storing rotation data
void calc_weights(Tile_Entry& cell, N_Kernel& kernel) {
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

N_Kernel get_neighbour_kernel(Vec2u pos, Vec<Tile_Entry>& tiles) {
	N_Kernel kernel;
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

void calc_cell_info(Vec2u pos, Vec<Tile_Entry>& tiles) {
	auto& cell = tiles.at(get_idx_vec2u(pos));
	N_Kernel kernel = get_neighbour_kernel(pos, tiles);
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

Map generate_map(const u16 m_width, const u16 m_height) {
	Map map(m_width, m_height);
	// temporary fix
	width = m_width;
	height = m_height;

	Vec<Tile_Entry> tiles(width * height);
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
	u16 start_x = width / 2;
	u16 start_y = height / 2;
	tiles.at(get_idx(start_x, start_y)).tile = Tile::Empty;

	// get neighbouring cells' positions
	calc_cell_info({start_x, start_y}, tiles);

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
		LOG_DBG("TILE AT POS: {}, {}", current_pos.x, current_pos.y);
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
			break;
		}
		LOG_DBG(" 	 	HAS CHOSEN: {}", (u32)cell.tile);
		iter++;
    }

	// output the result
	for (u16 i = 0; i < height * width; i++) {
		map.data[i] = tiles[i].tile;
		if (i % (width) == 0) {
			printf("\n");
		}
		u32 sym = tiles.at(i).tile;
		printf("%lu ", sym);
	}
	return map;
}

Tile Map::at(Vec2u pos) const {
	if (pos.x <= 0 || pos.x >= width 
		|| pos.y <= 0 || pos.y >= height) {
		return Tile::Wall;
	} else {
		return data[pos.x + (width * pos.y)];
	}
}

Map::Map(u16 width, u16 height): height(height), width(width) {
	this->data.resize(width * height);
}
