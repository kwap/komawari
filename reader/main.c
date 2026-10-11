/* Komawari reader: shows prepared comics one panel at a time.

   The same code runs in a window on a desktop computer and on the PS Vita.
   It starts on the shelf (shelf.h), where a comic is chosen, and moves between
   the shelf and the reading screen (reading.h) until the user quits. */
#include <SDL.h>

#include "covers.h"
#include "draw.h"
#include "input.h"
#include "library.h"
#include "place.h"
#include "reading.h"
#include "screen.h"
#include "shelf.h"

/* Opens the window (the whole screen on the Vita) and the renderer that draws into it. */
static int open_screen(SDL_Window **window, SDL_Renderer **renderer)
{
    /* Smooth scaling instead of blocky pixels. Must be set before any picture is loaded. */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    *window = SDL_CreateWindow("Komawari", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (*window == NULL) {
        return -1;
    }
    /* Drawing in step with the screen's refresh keeps movement smooth. Where
       that is not available, any renderer will do. */
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (*renderer == NULL) {
        *renderer = SDL_CreateRenderer(*window, -1, 0);
    }
    if (*renderer == NULL) {
        return -1;
    }
    /* Lets the draw functions paint see-through shapes. */
    SDL_SetRenderDrawBlendMode(*renderer, SDL_BLENDMODE_BLEND);
    return 0;
}

int main(int argc, char *argv[])
{
    /* DEFAULT_LIBRARY_DIR is set by CMakeLists.txt. */
    const char *library_dir = argc > 1 ? argv[1] : DEFAULT_LIBRARY_DIR;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if (open_screen(&window, &renderer) != 0) {
        SDL_Log("Cannot open the screen: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    input_open();

    Library library;
    if (library_load(&library, library_dir) != 0 || library.count == 0) {
        SDL_Log("No comics found in %s", library_dir);
        draw_failure(renderer);
        SDL_Quit();
        return 1;
    }
    SDL_Log("Found %d comics in %s", library.count, library_dir);
    Covers *covers = covers_open(renderer, &library);
    if (covers == NULL) {
        draw_failure(renderer);
        SDL_Quit();
        return 1;
    }

    /* The shelf opens on the comic that was read last. */
    int chosen = place_last_comic(&library, library_dir);
    /* With a single comic there is nothing to choose, so it opens straight away.
       The shelf is still where the user lands when leaving that comic. */
    int skip_shelf = library.count == 1;
    for (;;) {
        if (!skip_shelf) {
            chosen = shelf_run(window, renderer, &library, covers, chosen);
            if (chosen < 0) {
                break;
            }
        }
        skip_shelf = 0;
        place_remember_comic(library_dir, library.comic_dirs[chosen]);
        if (reading_run(window, renderer, library.comic_dirs[chosen]) == READING_ENDED_QUIT) {
            break;
        }
    }

    covers_close(covers);
    library_free(&library);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
