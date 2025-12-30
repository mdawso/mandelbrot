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

struct Vector2i {
    int x;
    int y;
};

Complex map_pixel_to_complex(const int x, const int y, WindowBounds* bounds) {
    double real = bounds->view_min_x + (double)x / bounds->win_x * (bounds->view_max_x - bounds->view_min_x);
    double imaginary = bounds->view_max_y - (double)y / bounds->win_y * (bounds->view_max_y - bounds->view_min_y);
    return Complex(real, imaginary);
}

Vector2i map_complex_to_pixel(Complex c, WindowBounds* bounds) {
    int x = static_cast<int>((std::real(c) - bounds->view_min_x) / (bounds->view_max_x - bounds->view_min_x) * bounds->win_x);
    int y = static_cast<int>((bounds->view_max_y - std::imag(c)) / (bounds->view_max_y - bounds->view_min_y) * bounds->win_y);
    return Vector2i{x, y};
}

void zoom(WindowBounds* bounds, Vector2i centre_pixel, double amount) {
    double centre_x = bounds->view_min_x + (double)centre_pixel.x / bounds->win_x * (bounds->view_max_x - bounds->view_min_x);
    double centre_y = bounds->view_max_y - (double)centre_pixel.y / bounds->win_y * (bounds->view_max_y - bounds->view_min_y);

    double width = bounds->view_max_x - bounds->view_min_x;
    double height = bounds->view_max_y - bounds->view_min_y;

    double new_width = width / amount;
    double new_height = height / amount;

    bounds->view_min_x = centre_x - new_width / 2.0;
    bounds->view_max_x = centre_x + new_width / 2.0;
    bounds->view_min_y = centre_y - new_height / 2.0;
    bounds->view_max_y = centre_y + new_height / 2.0;
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

void recalculate_mandelbrot_texture(State* state, int max_iterations, int num_threads) {

    size_t buffer_size = state->bounds->win_x * state->bounds->win_y;
    std::vector<SDL_Color> pixel_buffer(buffer_size);

    constexpr SDL_Color BLACK = {0,0,0,255};

    std::vector<std::thread> workers;
    std::atomic<size_t> next_pixel(0);

    auto worker = [&]() {
        size_t i;
        while ((i = next_pixel.fetch_add(1)) < buffer_size) {
            int x = i % state->bounds->win_x;
            int y = i / state->bounds->win_x;
            Complex c = map_pixel_to_complex(x, y, state->bounds);
            int d = diverges(c, max_iterations);
            unsigned char r = (d * 255) / max_iterations;
            pixel_buffer[i] = {r, 0, 0, 255};
        }
    };

    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back(worker);
    }

    for (auto& thread : workers) {
        thread.join();
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
    
    constexpr Complex zoom_point = Complex(-0.743643887037151, 0.13182590420533);
    zoom(state->bounds, map_complex_to_pixel(zoom_point, state->bounds), 1.1);
    recalculate_mandelbrot_texture(state, 100, 16);

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
