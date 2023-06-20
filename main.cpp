#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <cstdlib>
#include <fmt/printf.h>
#include <SDL2/SDL.h>
#include <cstdint>

#include "player.hpp"
#include "types_utils.hpp"

static Player PLAYER;

struct Settings {
    u32 width  = 800;
    u32 height = 600;
} CONF;

enum GameState : byte {
    init,
    play,
    stop,
} STATE;

void handle_events(SDL_Event& event, MoveType type) {
    using dir = Player::Direction;
    switch (event.key.keysym.sym) {
        case SDLK_LEFT:     PLAYER.move(dir::left, type); break;
        case SDLK_RIGHT:    PLAYER.move(dir::right,type); break;
        case SDLK_UP:       PLAYER.move(dir::jump, type); break;
    }
}

void poll_events(SDL_Event& event) {
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:      STATE = stop; break;
            case SDL_KEYDOWN:   handle_events(event, MoveType::press); break;
            case SDL_KEYUP:     handle_events(event, MoveType::release); break;
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

    STATE = play;
    SDL_Event event;

    while (STATE != stop) {
        poll_events(event);
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
