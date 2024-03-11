#include <cstdlib>
#include <fmt/printf.h>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>

#include "types_utils.hpp"
#include "entity.hpp"
#include "physics.hpp"
#include "map.hpp"
#include "renderable.hpp"

#define DEBUG

static std::vector<Entity> entities;
static std::vector<ID> player_entities;

static std::unordered_map<ID, Physics> physics_comps;
static std::unordered_map<ID, Renderable> render_comps;

constexpr u64 fps_cap = 240;
constexpr u64 frame_min_dur = 1'000 / fps_cap;

constexpr u64 sprite_ani_fps = 10;
constexpr u64 sprite_frame_dur = 1'000 / sprite_ani_fps;

struct Settings {
    u32 width  = 800;
    u32 height = 600;
} CONF;

enum GameState : byte {
    init,
    playing,
    stopping,
} STATE;

void spawn_player(Position spawn_pos) {
    Entity player = create_new_entity(PLAYER_FLAG | PHYSICS_FLAG | RENDER_FLAG);
    entities.push_back(player);

    Physics player_phys;
    player_phys.loc = Location::air;
    player_phys.pos = spawn_pos;

    physics_comps[player.id] = player_phys;

    player_entities.push_back(player.id);
}

void handle_events(SDL_Event& event, MoveType type) {
    using Dir = Direction;
    if (event.key.keysym.sym == SDLK_q) {
        STATE = GameState::stopping;
    }

    for (const auto& id : player_entities) {
        auto& comp = physics_comps.at(id);
        switch (event.key.keysym.sym) {
            case SDLK_LEFT:     update_move(comp, Dir::left, type); break;
            case SDLK_RIGHT:    update_move(comp, Dir::right,type); break;

            case SDLK_UP:       update_move(comp, Dir::jump, type); break;
            case SDLK_SPACE:    update_move(comp, Dir::jump, type); break;
        }
        auto& rend = render_comps.at(id);
        rend.dir = comp.dir;
    }
}

void poll_events(SDL_Event& event) {
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:      STATE = GameState::stopping; break;
            case SDL_KEYDOWN:   handle_events(event, MoveType::move); break;
            case SDL_KEYUP:     handle_events(event, MoveType::stop); break;
        }
    }
}

void handle_entities(SDL_Window* wndw, SDL_Renderer* rndr, const Map& map) {
    for (const auto& entity : entities) {
        if (entity.flags & PHYSICS_FLAG) {
            auto& comp = physics_comps.at(entity.id);
            update_tick(comp, map);

            if (entity.flags & RENDER_FLAG) {
                //LOG_DBG("{} {}", comp.pos.x, comp.pos.y);
                auto& rend = render_comps.at(entity.id);
                rend.pos.x = comp.pos.x;
                rend.pos.y = comp.pos.y;
            }
        }

        // rendering TODO: Move out of the function
        if (entity.flags & RENDER_FLAG) {
            SDL_SetRenderDrawColor(rndr, 0, 0, 0, 255);
            auto& elem = render_comps.at(entity.id);
            // render 
            SDL_Rect rect = {
                (int)(elem.pos.x / Position::MAX * CONF.width),
                (int)(CONF.height - (elem.pos.y / Position::MAX * CONF.height) - elem.bnd.y), 
                elem.bnd.x, 
                elem.bnd.y
            };
            static byte frame = 0;
            static unsigned subframe = 0;
            const auto count_frames = elem.sprites.size();
            SDL_RendererFlip mirror_flip = 
                elem.dir == Direction::left ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            if (count_frames > 0) {
                SDL_RenderCopyEx(rndr, elem.sprites[frame % count_frames], 
                        nullptr, &rect, 0.0, nullptr, mirror_flip); 
                subframe++;
                if (subframe % sprite_frame_dur == 0) {
                    frame = frame + 1 % count_frames; 
                    subframe = 0;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    STATE = init;

    u64 prev_tick = SDL_GetTicks64();
    u64 curr_tick = prev_tick;
    u64 delta_time;
    u64 prev_frame = SDL_GetTicks64();
    u64 delta_frame;

    // ------- SDL INITIALISATION --------
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow(
        argv[0], 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED,
        CONF.width, CONF.height, SDL_WINDOW_SHOWN
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, 
        -1, 
        SDL_RENDERER_ACCELERATED
    );
    defer {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    };
    if (window == nullptr || renderer == nullptr) {
        LOG_ERR("Failed to initialise SDL window");
        return EXIT_FAILURE;
    }
    // -----------------------------------

    // map generation
    Vec2u dim = {32, 24};
    Vec2u spawn = dim / Vec2u{2, 2};

    Map map(dim, spawn);

    // DEBUG TESTING
    // -----------------------------------
    // player entity init
    spawn_player({
            Position::MAX * spawn.x / (float)dim.x,
            Position::MAX * spawn.y / (float)dim.y,
    });

    // player sprite init 
    Renderable player_rend {
        .bnd = {.x = 24, .y = 36},
    };
    player_rend.add_sprite(renderer, "../assets/char0.bmp");
    player_rend.add_sprite(renderer, "../assets/char1.bmp");
    auto brick_bg   = SDL_LoadBMP("../assets/bricks_background.bmp");
    auto brick_wall = SDL_LoadBMP("../assets/bricks.bmp");

    auto brick_bg_tex  = SDL_CreateTextureFromSurface(renderer, brick_bg);
    auto brick_wall_tex = SDL_CreateTextureFromSurface(renderer, brick_wall);

    SDL_FreeSurface(brick_wall);
    SDL_FreeSurface(brick_bg);
    defer {
        SDL_DestroyTexture(brick_bg_tex);
        SDL_DestroyTexture(brick_wall_tex);
    };
    if (brick_bg_tex == nullptr || brick_wall_tex == nullptr) {
        LOG_ERR("Failed to initialise textures!");
        return EXIT_FAILURE;
    }

    for (const auto& id : player_entities) {
        render_comps[id] = player_rend;
    }
    
    STATE = playing;
    SDL_Event event;
    SDL_Texture *map_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET, CONF.width, CONF.height);
    if (map_texture == nullptr) {
        LOG_ERR("Failed to create map's texture!");
        return EXIT_FAILURE;
    }
    // map texture generation
	{
        int tex_w, tex_h;
        SDL_QueryTexture(brick_wall_tex, nullptr, nullptr, &tex_w, &tex_h);
        float cell_width = CONF.width / (float)map.width;
        float cell_height = CONF.height / (float)map.height;

        SDL_Rect dst_rect = {0, 0, (int)(cell_width), (int)cell_height};

        SDL_SetRenderTarget(renderer, map_texture);
        SDL_RenderClear(renderer);
        for (size_t y = 0; y < map.height; ++y) {
            for (size_t x = 0; x < map.width; ++x) {
                dst_rect.x = x * cell_width;
                dst_rect.y = y * cell_height;

                // Draw wall or empty space based on the map
                SDL_Texture* texture;
                auto tile = map.at(Vec2u{x, y});
                if (tile == Tile::Wall) {
                    texture = brick_wall_tex;
                } else {
                    texture = brick_bg_tex;
                }
                SDL_RenderCopy(renderer, texture, nullptr, &dst_rect);
            }
        }
        SDL_SetRenderTarget(renderer, nullptr);
    }
    defer {
        SDL_DestroyTexture(map_texture);
    };
    SDL_RenderCopy(renderer, map_texture, NULL, NULL);

    while (STATE != GameState::stopping) {
        poll_events(event);

        // render
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, map_texture, nullptr, nullptr);
        handle_entities(window, renderer, map);
        SDL_RenderPresent(renderer);

        curr_tick = SDL_GetTicks64();
        delta_time = curr_tick - prev_tick;

        // rendering synchro
        delta_frame = curr_tick - prev_frame;
        if (delta_frame < frame_min_dur) {
            SDL_Delay(uint(frame_min_dur - delta_frame));
        }
        prev_frame = curr_tick;
    }
    return EXIT_SUCCESS;
}
