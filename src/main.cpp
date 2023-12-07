#include <cstdlib>
#include <fmt/printf.h>
#include <cstdint>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>

#include "entity.hpp"
#include "physics.hpp"
#include "map.hpp"
#include "types_utils.hpp"

#define DEBUG

static std::vector<Entity> entities;
static std::vector<ID> player_entities;

static std::unordered_map<ID, Physics> physics_comps;

struct Settings {
    u32 width  = 1024;
    u32 height = 768;
} CONF;

enum GameState : byte {
    init,
    playing,
    stopping,
} STATE;

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

void handle_entities(SDL_Window* wndw, SDL_Renderer* rndr) {
    for (const auto& entity : entities) {
        if (entity.flags & PHYSICS_FLAG) {
            auto& comp = physics_comps.at(entity.id);
            step_tick(comp);
        }
        // TODO: add position for rendering
        if (entity.flags & PHYSICS_FLAG 
                && entity.flags & RENDER_FLAG) {
            SDL_SetRenderDrawColor(rndr, 0, 0, 0, 255);
            const auto& pos = physics_comps.at(entity.id).pos;
            SDL_Rect rect = {
                pos.x,
                pos.y, 
                24, 
                12
            };
            SDL_SetRenderDrawColor(rndr, 255, 0, 0, 255);
            SDL_RenderFillRect(rndr, &rect);
        }
    }
}

int main(int argc, char* argv[]) {
    STATE = init;

    u64 prev_tick = SDL_GetTicks64();
    u64 curr_tick = prev_tick;
    u64 delta_time;

    constexpr u64 fps_cap = 240;
    constexpr u64 frame_min_dur = 1'000 / fps_cap;
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

    // DEBUG TESTING
    Map map;
    if (!map.load_data("/home/gero/rogalitur/assets/first_map.bmp")) {
        LOG_ERR("Failed to load map data!");
        return EXIT_FAILURE;
    }

    // ----- Texture initialisation ------
    SDL_Surface *map_surf = SDL_CreateRGBSurfaceWithFormatFrom(
            map.data.data(), 
            map.width, map.height, 8, map.width, SDL_PIXELFORMAT_INDEX8
    );

    /* SDL_Texture* map_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        map.width, map.height 
    ); */

    auto surf = SDL_LoadBMP("/home/gero/rogalitur/assets/first_map.bmp");
    map.load_from_sdl(*surf);
    auto map_texture = SDL_CreateTextureFromSurface(renderer, surf);
    defer {
        SDL_DestroyTexture(map_texture);
    };
    if (map_texture == nullptr) {
        LOG_ERR("Failed to generate texture!");
        return EXIT_FAILURE;
    }
    SDL_FreeSurface(map_surf);

    // -----------------------------------
    // player entity init
    {
        Entity player;
        player.flags |= PLAYER_FLAG;
        player.flags |= PHYSICS_FLAG;
        player.flags |= RENDER_FLAG;

        Physics player_phys;
        player_phys.pos = {.x = int(map.width / 2), .y = int(map.height / 2)};

        entities.push_back(player);
        physics_comps[player.id] = player_phys;
        player_entities.push_back(player.id);
    }
    
    STATE = playing;
    SDL_Event event;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, map_texture, NULL, NULL);

    while (STATE != GameState::stopping) {
        poll_events(event);
        // handle_entities()

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, map_texture, nullptr, nullptr);
        handle_entities(window, renderer);
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
