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

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

constexpr int WINDOW_X = 1152;
constexpr int WINDOW_Y = 648;

constexpr double VIEW_MIN_X = -2.0;
constexpr double VIEW_MAX_X = 1.0;
constexpr double VIEW_MIN_Y = -1.0;
constexpr double VIEW_MAX_Y = 1.0;
constexpr int MAX_ITERATIONS = 100;

static std::array<SDL_Color, WINDOW_X * WINDOW_Y> pixel_buffer;
constexpr int NUM_THREADS = 8;
static std::array<std::thread, NUM_THREADS> workers;

Complex map_pixel_to_complex(const int x, const int y) {
    double real = VIEW_MIN_X + (double)x / WINDOW_X * (VIEW_MAX_X - VIEW_MIN_X);
    double imaginary = VIEW_MAX_Y - (double)y / WINDOW_Y * (VIEW_MAX_Y - VIEW_MIN_Y);
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

SDL_Color map_iters_to_colour(int iters, int max_iterations) {
    unsigned char shade = std::floor(std::lerp(0.0,255.0,static_cast<double>(iters)/max_iterations));
    //if (iters == max_iterations) shade = 0;
    SDL_Color col = {
        .r = shade,
        .g = 0,
        .b = 0,
        .a = 255
    };
    return col;
}

void calculate_pixel_buffer(int start_i, int end_i) {
    if (end_i > pixel_buffer.size()) return;
    for (int i = start_i; i < end_i; i++) {
        int x = i % WINDOW_X;
        int y = i / WINDOW_X;
        Complex c = map_pixel_to_complex(x,y);
        int iters = diverges(c, MAX_ITERATIONS);
        pixel_buffer[i] = map_iters_to_colour(iters, MAX_ITERATIONS);
    }
}

void work(int start_i, int end_i) {
    calculate_pixel_buffer(start_i, end_i);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("mandelbrot", "1.0", "com.mias.mandelbrot");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("mandelbrot", WINDOW_X, WINDOW_Y, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WINDOW_X, WINDOW_Y, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    constexpr SDL_Color BLACK = {0,0,0,SDL_ALPHA_OPAQUE};
    pixel_buffer.fill(BLACK);

    for (int i = 0; i < NUM_THREADS; ++i) {
        int start_i = i * pixel_buffer.size() / NUM_THREADS;
        int end_i = (i + 1) * pixel_buffer.size() / NUM_THREADS;
        if (i == NUM_THREADS - 1) {
            end_i = pixel_buffer.size();
        }
        workers[i] = std::thread(work, start_i, end_i);
    }

    for (auto &w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

static unsigned int frame = 0;
SDL_AppResult SDL_AppIterate(void *appstate)
{

    std::cout << frame++ << '\n';

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    for (int i = 0; i < pixel_buffer.size(); i++) {
        int x = i % WINDOW_X;
        int y = i / WINDOW_X;
        SDL_SetRenderDrawColor(
            renderer,
            pixel_buffer[i].r,
            pixel_buffer[i].g,
            pixel_buffer[i].b,
            pixel_buffer[i].a
        );
        SDL_RenderPoint(renderer,x,y);
    }

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
