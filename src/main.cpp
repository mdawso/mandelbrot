#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

#include "mandelbrot.hpp"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

constexpr int WINDOW_X = 1920;
constexpr int WINDOW_Y = 1080;

constexpr double VIEW_MIN_X = -2.0;
constexpr double VIEW_MAX_X = 1.0;
constexpr double VIEW_MIN_Y = -1.0;
constexpr double VIEW_MAX_Y = 1.0;
constexpr int MAX_ITERATIONS = 25;

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

    return SDL_APP_CONTINUE;
}

Complex map_pixel_to_complex(const int x, const int y) {
    double real = VIEW_MIN_X + (double)x / WINDOW_X * (VIEW_MAX_X - VIEW_MIN_X);
    double imaginary = VIEW_MAX_Y - (double)y / WINDOW_Y * (VIEW_MAX_Y - VIEW_MIN_Y);
    return Complex(real, imaginary);
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    for (int y = 0; y < WINDOW_Y; y++) {
        for (int x = 0; x < WINDOW_X; x++) {

            Complex complex = map_pixel_to_complex(x, y);
            bool escaped = diverges(complex, MAX_ITERATIONS);

            if (escaped) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            }

            SDL_RenderPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
