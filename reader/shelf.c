#include "shelf.h"

#include <SDL_image.h>

#include "draw.h"
#include "input.h"
#include "motion.h"
#include "screen.h"

/* How long the covers slide when another comic is chosen, in milliseconds. */
#define SLIDE_MS 260
/* Height on screen of the chosen cover, and how much smaller its neighbours are. */
#define COVER_HEIGHT 420.0f
#define NEIGHBOUR_SCALE 0.72f
/* Distance between the centres of two covers next to each other. */
#define COVER_SPACING 310.0f
/* How dark the covers beside the chosen one are: 0 is unchanged, 255 is black. */
#define NEIGHBOUR_DIM 160.0f
/* Width divided by height of the grey box drawn for a comic without a cover picture. */
#define MISSING_COVER_SHAPE 0.7f
#define MARKS_ALPHA 230
/* The picture of the Cross button under the chosen cover. */
#define HINT_RADIUS 11.0f
#define HINT_GAP 22.0f

/* Everything the shelf keeps track of. */
typedef struct {
    SDL_Renderer *renderer;
    const Library *library;
    SDL_Texture **covers; /* one per comic; NULL where cover.jpg is missing */

    /* The covers sit in a row. position says which one is in the middle of the
       screen; it slides from slide_from to chosen when the user picks another. */
    int chosen;
    float slide_from;
    Uint32 slide_start;
} Shelf;

static void load_covers(Shelf *shelf)
{
    shelf->covers = SDL_calloc(shelf->library->count, sizeof(SDL_Texture *));
    if (shelf->covers == NULL) {
        return;
    }
    for (int i = 0; i < shelf->library->count; i++) {
        char path[1024];
        SDL_snprintf(path, sizeof(path), "%s/cover.jpg", shelf->library->comic_dirs[i]);
        shelf->covers[i] = IMG_LoadTexture(shelf->renderer, path);
    }
}

static void free_covers(Shelf *shelf)
{
    if (shelf->covers == NULL) {
        return;
    }
    for (int i = 0; i < shelf->library->count; i++) {
        SDL_DestroyTexture(shelf->covers[i]);
    }
    SDL_free(shelf->covers);
}

/* Which place in the row is in the middle of the screen right now. During a
   slide this lies between two covers. */
static float position_now(const Shelf *shelf, Uint32 now)
{
    float slid = motion_progress(shelf->slide_start, SLIDE_MS, now);
    return shelf->slide_from + ((float)shelf->chosen - shelf->slide_from) * slid;
}

static void choose(Shelf *shelf, int comic)
{
    if (comic < 0 || comic >= shelf->library->count || comic == shelf->chosen) {
        return;
    }
    Uint32 now = SDL_GetTicks();
    shelf->slide_from = position_now(shelf, now);
    shelf->slide_start = now;
    shelf->chosen = comic;
}

/* Draws one cover. places_away says how far it is from the middle of the
   screen, counted in covers: 0 is the middle, -1 is one place to the left. */
static void draw_cover(const Shelf *shelf, int comic, float places_away)
{
    /* 1 in the middle of the screen, falling to 0 one place away and beyond. */
    float nearness = 1 - SDL_min(SDL_fabsf(places_away), 1.0f);
    float scale = NEIGHBOUR_SCALE + (1 - NEIGHBOUR_SCALE) * nearness;

    float shape = MISSING_COVER_SHAPE;
    SDL_Texture *cover = shelf->covers != NULL ? shelf->covers[comic] : NULL;
    int picture_width, picture_height;
    if (cover != NULL &&
        SDL_QueryTexture(cover, NULL, NULL, &picture_width, &picture_height) == 0) {
        shape = (float)picture_width / (float)picture_height;
    }

    SDL_FRect area;
    area.h = COVER_HEIGHT * scale;
    area.w = area.h * shape;
    area.x = SCREEN_WIDTH / 2.0f + places_away * COVER_SPACING - area.w / 2;
    area.y = (SCREEN_HEIGHT - area.h) / 2 - 10;

    if (cover != NULL) {
        SDL_RenderCopyF(shelf->renderer, cover, NULL, &area);
    } else {
        draw_fill(shelf->renderer, area, 90, 255);
    }
    draw_fill(shelf->renderer, area, 0, (Uint8)(NEIGHBOUR_DIM * (1 - nearness)));
}

static void draw_frame(const Shelf *shelf, Uint32 now)
{
    SDL_SetRenderDrawColor(shelf->renderer, 14, 14, 16, 255);
    SDL_RenderClear(shelf->renderer);

    float position = position_now(shelf, now);
    int middle = (int)(position + 0.5f);
    /* The cover in the middle is drawn last, so it lies on top of its neighbours. */
    for (int comic = 0; comic < shelf->library->count; comic++) {
        if (comic != middle) {
            draw_cover(shelf, comic, (float)comic - position);
        }
    }
    draw_cover(shelf, middle, (float)middle - position);

    float cover_bottom = (SCREEN_HEIGHT + COVER_HEIGHT) / 2 - 10;
    draw_cross_button(shelf->renderer, SCREEN_WIDTH / 2.0f, cover_bottom + HINT_GAP, HINT_RADIUS,
                      MARKS_ALPHA);
    draw_squares(shelf->renderer, shelf->library->count, shelf->chosen, MARKS_ALPHA);
    SDL_RenderPresent(shelf->renderer);
}

int shelf_run(SDL_Window *window, SDL_Renderer *renderer, const Library *library, int chosen)
{
    Shelf shelf = {0};
    shelf.renderer = renderer;
    shelf.library = library;
    shelf.chosen = chosen;
    shelf.slide_from = (float)chosen;
    load_covers(&shelf);
    SDL_SetWindowTitle(window, "Komawari");

    int opened;
    for (;;) {
        Uint32 now = SDL_GetTicks();
        draw_frame(&shelf, now);

        Action action = input_next(motion_running(shelf.slide_start, SLIDE_MS, now));
        if (action == ACTION_RIGHT) {
            choose(&shelf, shelf.chosen + 1);
        } else if (action == ACTION_LEFT) {
            choose(&shelf, shelf.chosen - 1);
        } else if (action == ACTION_CONFIRM) {
            opened = shelf.chosen;
            break;
        } else if (action == ACTION_QUIT) {
            opened = -1;
            break;
        }
    }

    free_covers(&shelf);
    return opened;
}
