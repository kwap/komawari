#include "shelf.h"

#include "draw.h"
#include "input.h"
#include "motion.h"
#include "picture.h"
#include "screen.h"

/* How long the covers slide when another comic is chosen, in milliseconds. */
#define SLIDE_MS 260
/* Height on screen of the chosen cover, and how much smaller its neighbours are. */
#define COVER_HEIGHT 340.0f
#define NEIGHBOUR_SCALE 0.72f
/* Distance between the centres of two covers next to each other. Wide enough
   that the covers two places from the middle are off the screen while the row
   stands still: in a ring of three they repeat the covers already in view. */
#define COVER_SPACING 290.0f
/* How far below the top of the screen the middle of the covers is. */
#define COVER_MIDDLE_Y 190.0f
/* Under the chosen cover sits the picture with its title, subtitle and author
   (title.png, 100 pixels high). This is its top edge. */
#define TITLE_TOP 372
/* How fast the title fades as the row moves away from its cover: at 2.5 it is
   gone when the row is 0.4 of a place away. */
#define TITLE_FADE 2.5f
/* How dark the covers beside the chosen one are: 0 is unchanged, 255 is black. */
#define NEIGHBOUR_DIM 160.0f
/* Width divided by height of the grey box drawn for a comic without a cover picture. */
#define MISSING_COVER_SHAPE 0.7f
#define MARKS_ALPHA 230
/* The picture of the Cross button under the title: its size and its middle. */
#define HINT_RADIUS 11.0f
#define HINT_Y 492.0f
/* With this many comics or more the row of covers is a ring: after the last
   comic comes the first one again. */
#define RING_MIN_COMICS 3
/* A swipe shorter than half a cover still moves on by one cover if the finger
   went at least this many pixels. */
#define SWIPE_MIN_PIXELS 40.0f

/* Everything the shelf keeps track of. */
typedef struct {
    SDL_Renderer *renderer;
    const Library *library;
    SDL_Texture **covers; /* one per comic; NULL where cover.jpg is missing */
    SDL_Texture **titles; /* one per comic; NULL where title.png is missing */

    /* The covers sit in a row, one place apart. A position says which place is
       in the middle of the screen; between two covers it is a fraction. In a
       ring it may run past either end, and is then counted round. */
    int chosen;       /* the comic the row is at, or is sliding to */
    float slide_from; /* the row slides from this position ... */
    float slide_to;   /* ... to this one, which is a whole number */
    Uint32 slide_start;

    /* While a finger drags the row, the position follows the finger instead. */
    int dragging;
    float drag_position;
    float drag_from;   /* the position when the drag began */
    float drag_pixels; /* how far the finger has gone; to the right counts as positive */
} Shelf;

/* Loads every comic's cover and the picture of its title. */
static void load_pictures(Shelf *shelf)
{
    int count = shelf->library->count;
    shelf->covers = SDL_calloc(count, sizeof(SDL_Texture *));
    shelf->titles = SDL_calloc(count, sizeof(SDL_Texture *));
    if (shelf->covers == NULL || shelf->titles == NULL) {
        return;
    }
    Uint32 start = SDL_GetTicks();
    for (int i = 0; i < count; i++) {
        char path[1024];
        SDL_snprintf(path, sizeof(path), "%s/cover.jpg", shelf->library->comic_dirs[i]);
        shelf->covers[i] = picture_load(shelf->renderer, path);
        SDL_snprintf(path, sizeof(path), "%s/title.png", shelf->library->comic_dirs[i]);
        shelf->titles[i] = picture_load(shelf->renderer, path);
    }
    SDL_Log("Shelf: the pictures of %d comics loaded in %u ms", count, SDL_GetTicks() - start);
}

static void free_pictures(Shelf *shelf)
{
    for (int i = 0; i < shelf->library->count; i++) {
        if (shelf->covers != NULL) {
            SDL_DestroyTexture(shelf->covers[i]);
        }
        if (shelf->titles != NULL) {
            SDL_DestroyTexture(shelf->titles[i]);
        }
    }
    SDL_free(shelf->covers);
    SDL_free(shelf->titles);
}

static int is_ring(const Shelf *shelf)
{
    return shelf->library->count >= RING_MIN_COMICS;
}

/* The comic at a place in the row. In a ring every place has one. */
static int comic_at(const Shelf *shelf, int place)
{
    int count = shelf->library->count;
    return ((place % count) + count) % count;
}

/* Which position is in the middle of the screen right now. */
static float position_now(const Shelf *shelf, Uint32 now)
{
    if (shelf->dragging) {
        return shelf->drag_position;
    }
    float slid = motion_progress(shelf->slide_start, SLIDE_MS, now);
    return shelf->slide_from + (shelf->slide_to - shelf->slide_from) * slid;
}

/* Starts a slide from where the row is now to the given place. */
static void slide_to_place(Shelf *shelf, int place)
{
    int count = shelf->library->count;
    if (!is_ring(shelf)) {
        place = SDL_clamp(place, 0, count - 1);
    }
    Uint32 now = SDL_GetTicks();
    shelf->slide_from = position_now(shelf, now);
    shelf->slide_to = (float)place;
    shelf->slide_start = now;
    shelf->dragging = 0;
    shelf->chosen = comic_at(shelf, place);

    /* After many turns round a ring the numbers would grow without end, so
       whole turns are taken off again. */
    float turns = (float)(place - shelf->chosen);
    shelf->slide_from -= turns;
    shelf->slide_to -= turns;
}

/* Moves on by a number of covers: 1 is the next comic, -1 the one before. */
static void move_by(Shelf *shelf, int covers)
{
    slide_to_place(shelf, (int)shelf->slide_to + covers);
}

/* Where a cover is drawn. places_away says how far it is from the middle of
   the screen, counted in covers: 0 is the middle, -1 is one place to the left. */
static SDL_FRect cover_area(const Shelf *shelf, int comic, float places_away)
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
    area.y = COVER_MIDDLE_Y - area.h / 2;
    return area;
}

static void draw_cover(const Shelf *shelf, int comic, float places_away)
{
    SDL_FRect area = cover_area(shelf, comic, places_away);
    float nearness = 1 - SDL_min(SDL_fabsf(places_away), 1.0f);
    SDL_Texture *cover = shelf->covers != NULL ? shelf->covers[comic] : NULL;

    if (cover != NULL) {
        SDL_RenderCopyF(shelf->renderer, cover, NULL, &area);
    } else {
        draw_fill(shelf->renderer, area, 90, 255);
    }
    draw_fill(shelf->renderer, area, 0, (Uint8)(NEIGHBOUR_DIM * (1 - nearness)));
}

/* The title of the comic nearest the middle of the screen. It fades out as
   the row moves on, and the next comic's title fades in. */
static void draw_title(const Shelf *shelf, float position)
{
    int place = (int)SDL_floorf(position + 0.5f);
    if (shelf->titles == NULL ||
        (!is_ring(shelf) && (place < 0 || place >= shelf->library->count))) {
        return;
    }
    SDL_Texture *title = shelf->titles[comic_at(shelf, place)];
    int width, height;
    if (title == NULL || SDL_QueryTexture(title, NULL, NULL, &width, &height) != 0) {
        return;
    }
    float away = SDL_fabsf(position - (float)place);
    float shown = SDL_clamp(1 - away * TITLE_FADE, 0.0f, 1.0f);
    /* The picture is drawn at its own size, so its letters stay sharp. */
    SDL_Rect area = {(SCREEN_WIDTH - width) / 2, TITLE_TOP, width, height};
    SDL_SetTextureAlphaMod(title, (Uint8)(255 * shown));
    SDL_RenderCopy(shelf->renderer, title, NULL, &area);
}

static void draw_frame(const Shelf *shelf, Uint32 now)
{
    SDL_SetRenderDrawColor(shelf->renderer, 14, 14, 16, 255);
    SDL_RenderClear(shelf->renderer);

    float position = position_now(shelf, now);
    int middle = (int)SDL_floorf(position + 0.5f);
    /* Two places on either side of the middle are all that fits on the screen.
       The furthest are drawn first and the middle one last, so that it lies on
       top of its neighbours. */
    for (int away = 2; away >= 0; away--) {
        for (int side = -1; side <= 1; side += 2) {
            int place = middle + side * away;
            if (is_ring(shelf) || (place >= 0 && place < shelf->library->count)) {
                draw_cover(shelf, comic_at(shelf, place), (float)place - position);
            }
            if (away == 0) {
                break;
            }
        }
    }

    draw_title(shelf, position);
    draw_cross_button(shelf->renderer, SCREEN_WIDTH / 2.0f, HINT_Y, HINT_RADIUS, MARKS_ALPHA);
    draw_squares(shelf->renderer, shelf->library->count, shelf->chosen, MARKS_ALPHA);
    SDL_RenderPresent(shelf->renderer);
}

/* A finger drags the row along. */
static void drag(Shelf *shelf, float dx)
{
    if (!shelf->dragging) {
        shelf->drag_position = position_now(shelf, SDL_GetTicks());
        shelf->drag_from = shelf->drag_position;
        shelf->drag_pixels = 0;
        shelf->dragging = 1;
    }
    shelf->drag_pixels += dx;
    shelf->drag_position -= dx / COVER_SPACING;
    if (!is_ring(shelf)) {
        /* A row with two ends gives way a little, then stops. */
        float last = (float)(shelf->library->count - 1);
        shelf->drag_position = SDL_clamp(shelf->drag_position, -0.3f, last + 0.3f);
    }
}

/* The finger let go: the row slides on to the nearest cover. */
static void release(Shelf *shelf)
{
    if (!shelf->dragging) {
        return;
    }
    int place = (int)SDL_floorf(shelf->drag_position + 0.5f);
    int place_before = (int)SDL_floorf(shelf->drag_from + 0.5f);
    if (place == place_before && SDL_fabsf(shelf->drag_pixels) >= SWIPE_MIN_PIXELS) {
        /* A short swipe. Pulling the row to the right brings the comic before. */
        place += shelf->drag_pixels > 0 ? -1 : 1;
    }
    slide_to_place(shelf, place);
}

/* A tap on the cover in the middle opens it; a tap on a cover beside it
   brings that one to the middle. Returns 1 if the comic is to be opened. */
static int tap(Shelf *shelf, float x, float y)
{
    SDL_FPoint point = {x, y};
    float position = position_now(shelf, SDL_GetTicks());
    int middle = (int)SDL_floorf(position + 0.5f);
    for (int side = -1; side <= 1; side++) {
        int place = middle + side;
        if (!is_ring(shelf) && (place < 0 || place >= shelf->library->count)) {
            continue;
        }
        SDL_FRect area = cover_area(shelf, comic_at(shelf, place), (float)place - position);
        if (!SDL_PointInFRect(&point, &area)) {
            continue;
        }
        if (side == 0) {
            return 1;
        }
        slide_to_place(shelf, place);
        return 0;
    }
    return 0;
}

int shelf_run(SDL_Window *window, SDL_Renderer *renderer, const Library *library, int chosen)
{
    Shelf shelf = {0};
    shelf.renderer = renderer;
    shelf.library = library;
    shelf.chosen = chosen;
    shelf.slide_from = (float)chosen;
    shelf.slide_to = (float)chosen;
    load_pictures(&shelf);
    SDL_SetWindowTitle(window, "Komawari");

    int opened;
    for (;;) {
        Uint32 now = SDL_GetTicks();
        draw_frame(&shelf, now);

        Input input = input_next(motion_running(shelf.slide_start, SLIDE_MS, now));
        if (input.action == ACTION_RIGHT) {
            move_by(&shelf, 1);
        } else if (input.action == ACTION_LEFT) {
            move_by(&shelf, -1);
        } else if (input.action == ACTION_DRAG) {
            drag(&shelf, input.dx);
        } else if (input.action == ACTION_RELEASE) {
            release(&shelf);
        } else if (input.action == ACTION_CONFIRM ||
                   (input.action == ACTION_TAP && tap(&shelf, input.x, input.y))) {
            opened = shelf.chosen;
            break;
        } else if (input.action == ACTION_QUIT) {
            opened = -1;
            break;
        }
    }

    free_pictures(&shelf);
    return opened;
}
