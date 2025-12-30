#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

#include <vector>
#include <array>
#include <cmath>
#include <numeric>
#include <thread>
#include <iostream>
#include <atomic>
#include <complex>

using Complex = std::complex<long double>;

struct WindowBounds {
    int win_x;
    int win_y;
    double view_min_x;
    double view_min_y;
    double view_max_x;
    double view_max_y;
};

struct State {
    WindowBounds* bounds;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* mandelbrot_texture;
};

Complex map_pixel_to_complex(const int x, const int y, WindowBounds* bounds) {
    double real = bounds->view_min_x + (double)x / bounds->win_x * (bounds->view_max_x - bounds->view_min_x);
    double imaginary = bounds->view_max_y - (double)y / bounds->win_y * (bounds->view_max_y - bounds->view_min_y);
    return Complex(real, imaginary);
}

int diverges(const Complex& c, int max_iterations) {
    Complex z(0.0, 0.0);
    for (int i = 0; i < max_iterations; ++i) {
        z = z * z + c;
        if (std::norm(z) > 4.0) {
            return i;
        }
    }
    return max_iterations;
}

void recalculate_mandelbrot_texture(State* state, int max_iterations) {
    size_t buffer_size = state->bounds->win_x * state->bounds->win_y;
    std::vector<SDL_Color> pixel_buffer(buffer_size);

    constexpr SDL_Color BLACK = {0,0,0,255};

    for (int i = 0; i < buffer_size; i++) {
        int x = i % state->bounds->win_x;
        int y = i / state->bounds->win_x;
        Complex c = map_pixel_to_complex(x, y, state->bounds);
        int d = diverges(c, max_iterations);
        unsigned char r = (d * 255) / max_iterations;
        pixel_buffer[i] = {r, 0, 0, 255};
    }

    SDL_UpdateTexture(state->mandelbrot_texture, NULL, pixel_buffer.data(), state->bounds->win_x * sizeof(SDL_Color));
}


SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    WindowBounds* bounds = new WindowBounds{
        .win_x = 1280,
        .win_y = 720,
        .view_min_x = -2.0,
        .view_min_y = -1.0,
        .view_max_x = 1.0,
        .view_max_y = 1.0
    };

    State* state = new State{
        .bounds = bounds,
        .window = NULL,
        .renderer = NULL,
        .mandelbrot_texture = NULL,
    };

    *appstate = state;

    SDL_SetAppMetadata("mandelbrot", "1.0", "com.mias.mandelbrot");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("mandelbrot", bounds->win_x, bounds->win_y, SDL_WINDOW_RESIZABLE, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    SDL_SetRenderLogicalPresentation(state->renderer, bounds->win_x, bounds->win_y, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    state->mandelbrot_texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, state->bounds->win_x, state->bounds->win_y);
    if (!state->mandelbrot_texture) {
        SDL_Log("Couldn't create mandelbrot_texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    constexpr int max_iters = 250;
    recalculate_mandelbrot_texture(state, max_iters);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{

    State* state = static_cast<State*>(appstate);

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(state->renderer);

    SDL_RenderTexture(state->renderer, state->mandelbrot_texture, NULL, NULL);

    SDL_RenderPresent(state->renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    State* state = static_cast<State*>(appstate);
    if (state) {
        delete state->bounds;
        delete state;
    }
}
