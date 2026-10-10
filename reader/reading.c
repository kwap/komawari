#include "reading.h"

#include <SDL_image.h>

#include "book.h"
#include "draw.h"
#include "input.h"
#include "motion.h"
#include "screen.h"
#include "shot.h"

/* How long the picture glides from one panel to the next, in milliseconds. */
#define GLIDE_MS 260
/* How long a new page takes to fade in. */
#define FADE_MS 180
/* How long the position marks stay on screen after a step, and how much of
   that time they spend fading out. */
#define MARKS_MS 1500
#define MARKS_FADE_MS 300
#define MARKS_ALPHA 230

/* Everything the reading screen keeps track of. */
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    const char *comic_dir;
    Book book;

    /* Where the reader is. */
    int page;       /* index into book.pages */
    int view;       /* index of the panel shown; 0 on a page without panels */
    int whole_page; /* 1 while the whole page is shown instead of one panel */

    /* What is on screen. The picture glides from one shot to another. */
    SDL_Texture *page_picture;
    Shot glide_from;
    Shot glide_to;
    Uint32 glide_start;
    Uint32 fade_start;
    Uint32 marks_start;
} Reading;

static const Page *current_page(const Reading *reading)
{
    return &reading->book.pages[reading->page];
}

/* A page without panels is shown whole, as one view. */
static int view_count(const Page *page)
{
    return page->panel_count > 0 ? page->panel_count : 1;
}

static void step_forward(Reading *reading)
{
    if (reading->view + 1 < view_count(current_page(reading))) {
        reading->view += 1;
    } else if (reading->page + 1 < reading->book.page_count) {
        reading->page += 1;
        reading->view = 0;
    }
}

static void step_back(Reading *reading)
{
    if (reading->view > 0) {
        reading->view -= 1;
    } else if (reading->page > 0) {
        reading->page -= 1;
        reading->view = view_count(current_page(reading)) - 1;
    }
}

/* The shot on screen right now, which may be part of the way through a glide. */
static Shot shot_now(const Reading *reading, Uint32 now)
{
    float glided = motion_progress(reading->glide_start, GLIDE_MS, now);
    return shot_between(reading->glide_from, reading->glide_to, glided);
}

static Shot shot_wanted(const Reading *reading)
{
    return shot_of_view(current_page(reading), reading->view, reading->whole_page);
}

/* Loads the current page's picture and lets it fade in. */
static void show_new_page(Reading *reading)
{
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%s/pages/%s", reading->comic_dir,
                 current_page(reading)->file);

    SDL_DestroyTexture(reading->page_picture);
    reading->page_picture = IMG_LoadTexture(reading->renderer, path);
    if (reading->page_picture == NULL) {
        SDL_Log("Cannot load %s: %s", path, SDL_GetError());
    }

    /* Loading takes a while, so the clock is read after it. */
    Uint32 now = SDL_GetTicks();
    reading->glide_from = shot_wanted(reading);
    reading->glide_to = reading->glide_from;
    reading->glide_start = 0;
    reading->fade_start = now;
    reading->marks_start = now;
}

/* Starts a glide from what is on screen now to the current view of the same page. */
static void glide_to_current_view(Reading *reading)
{
    Uint32 now = SDL_GetTicks();
    reading->glide_from = shot_now(reading, now);
    reading->glide_to = shot_wanted(reading);
    reading->glide_start = now;
    reading->marks_start = now;
}

static int something_is_moving(const Reading *reading, Uint32 now)
{
    return motion_running(reading->glide_start, GLIDE_MS, now) ||
           motion_running(reading->fade_start, FADE_MS, now) ||
           motion_running(reading->marks_start, MARKS_MS, now);
}

/* The page, with everything outside the lit part dimmed. */
static void draw_page(const Reading *reading, Shot shot)
{
    const Page *page = current_page(reading);
    SDL_FRect whole = {0, 0, (float)page->width, (float)page->height};

    if (reading->page_picture != NULL) {
        SDL_FRect on_screen = shot_on_screen(shot, whole);
        SDL_RenderCopyF(reading->renderer, reading->page_picture, NULL, &on_screen);
    }
    draw_dim_outside(reading->renderer, shot_on_screen(shot, shot.lit), (Uint8)shot.dim);
}

/* The position marks: a bar for the place in the comic and one square per
   view of this page. They show for a moment after each step, then fade. */
static void draw_marks(const Reading *reading, Uint32 now)
{
    if (!motion_running(reading->marks_start, MARKS_MS, now)) {
        return;
    }
    Uint32 time_left = reading->marks_start + MARKS_MS - now;
    Uint8 alpha = MARKS_ALPHA;
    if (time_left < MARKS_FADE_MS) {
        alpha = (Uint8)(MARKS_ALPHA * time_left / MARKS_FADE_MS);
    }
    float done = (float)(reading->page + 1) / (float)reading->book.page_count;
    draw_progress_bar(reading->renderer, done, alpha);
    draw_squares(reading->renderer, view_count(current_page(reading)), reading->view, alpha);
}

static void draw_fade_in(const Reading *reading, Uint32 now)
{
    float faded_in = motion_progress(reading->fade_start, FADE_MS, now);
    SDL_FRect screen = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    draw_fill(reading->renderer, screen, 0, (Uint8)(255 * (1 - faded_in)));
}

static void draw_frame(const Reading *reading, Uint32 now)
{
    SDL_SetRenderDrawColor(reading->renderer, 0, 0, 0, 255);
    SDL_RenderClear(reading->renderer);
    draw_page(reading, shot_now(reading, now));
    draw_fade_in(reading, now);
    draw_marks(reading, now);
    SDL_RenderPresent(reading->renderer);
}

/* On a desktop the window title tells where the reader is. */
static void update_window_title(const Reading *reading)
{
    char title[160];
    SDL_snprintf(title, sizeof(title), "Komawari - page %d of %d, view %d of %d", reading->page + 1,
                 reading->book.page_count, reading->view + 1, view_count(current_page(reading)));
    SDL_SetWindowTitle(reading->window, title);
}

/* Changes the reader's place as the user asked, and starts the matching movement. */
static void handle_action(Reading *reading, Action action)
{
    int old_page = reading->page;
    int old_view = reading->view;
    int old_whole_page = reading->whole_page;

    if (action == ACTION_RIGHT || action == ACTION_CONFIRM) {
        step_forward(reading);
    } else if (action == ACTION_LEFT) {
        step_back(reading);
    } else if (action == ACTION_UP) {
        reading->whole_page = !reading->whole_page;
    }

    if (reading->page != old_page) {
        show_new_page(reading);
        update_window_title(reading);
    } else if (reading->view != old_view || reading->whole_page != old_whole_page) {
        glide_to_current_view(reading);
        update_window_title(reading);
    }
}

ReadingEnd reading_run(SDL_Window *window, SDL_Renderer *renderer, const char *comic_dir)
{
    Reading reading = {0};
    reading.window = window;
    reading.renderer = renderer;
    reading.comic_dir = comic_dir;

    if (book_load(&reading.book, comic_dir) != 0) {
        draw_failure(renderer);
        return READING_ENDED_BACK;
    }
    SDL_Log("Opened %s: %d pages", comic_dir, reading.book.page_count);
    show_new_page(&reading);
    update_window_title(&reading);

    ReadingEnd end;
    for (;;) {
        Uint32 now = SDL_GetTicks();
        draw_frame(&reading, now);

        Action action = input_next(something_is_moving(&reading, now));
        if (action == ACTION_BACK) {
            end = READING_ENDED_BACK;
            break;
        }
        if (action == ACTION_QUIT) {
            end = READING_ENDED_QUIT;
            break;
        }
        handle_action(&reading, action);
    }

    SDL_DestroyTexture(reading.page_picture);
    book_free(&reading.book);
    return end;
}
