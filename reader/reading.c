#include "reading.h"

#include "book.h"
#include "draw.h"
#include "input.h"
#include "motion.h"
#include "pages.h"
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
    int turn;       /* how the Vita is held, as input_turn tells it */

    /* What is on screen. The picture glides from one shot to another. While
       the picture of the page being read is still loading, the page that was
       read before stays on screen. */
    Pages *pages;
    int shown_page;             /* index into book.pages; -1 before the first picture */
    SDL_Texture *shown_picture; /* NULL if it could not be loaded */
    Uint32 page_asked;          /* when the page being read was asked for */
    Shot glide_from;
    Shot glide_to;
    Uint32 glide_start;
    Uint32 fade_start;
    Uint32 marks_start;

    /* For the log: how the last step went, from the button to the end of its
       movement. */
    Uint32 step_start;       /* when the step was asked for */
    int step_timed;          /* 1 until the step's line is in the log */
    int frames;              /* frames drawn since step_start */
    Uint32 first_frame_done; /* when the first of them was on screen */
    Uint32 slowest_frame_ms;
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

/* Does this page turn when the Vita is held upright? A tall page that is only
   ever shown whole does: upright, it fills the screen instead of a strip of it. */
static int turns_with_vita(const Page *page)
{
    return page->panel_count == 0 && page->height > page->width;
}

static Shot shot_wanted(const Reading *reading)
{
    const Page *page = current_page(reading);
    int turned = turns_with_vita(page) ? reading->turn : 0;
    return shot_of_view(page, reading->view, reading->whole_page, turned);
}

/* Starts the timing of a step. now is when the step was asked for. */
static void time_step(Reading *reading, Uint32 now)
{
    reading->step_start = now;
    reading->step_timed = 1;
    reading->frames = 0;
    reading->slowest_frame_ms = 0;
}

/* Counts a frame that was drawn from start to end. Once the step's movement
   is over, writes one line about the step to the log. */
static void time_frame(Reading *reading, Uint32 start, Uint32 end, int still_moving)
{
    if (!reading->step_timed) {
        return;
    }
    if (reading->frames == 0) {
        reading->first_frame_done = end;
    }
    reading->frames += 1;
    reading->slowest_frame_ms = SDL_max(reading->slowest_frame_ms, end - start);
    if (still_moving) {
        return;
    }
    const Page *page = current_page(reading);
    SDL_Log("Step to page %d, view %d of %d, %s: on screen after %u ms; "
            "then %d frames in %u ms, slowest %u ms",
            reading->page + 1, reading->view + 1, view_count(page),
            reading->whole_page || page->panel_count == 0 ? "whole page" : "one panel",
            reading->first_frame_done - reading->step_start, reading->frames - 1,
            end - reading->first_frame_done, reading->slowest_frame_ms);
    reading->step_timed = 0;
}

/* Is the reader waiting for the picture of the page being read? */
static int waiting_for_page(const Reading *reading)
{
    return reading->shown_page != reading->page;
}

/* Tells the page loader where the reader is, and remembers when. */
static void ask_for_page(Reading *reading, Uint32 now)
{
    reading->page_asked = now;
    /* No line in the log for a step that never got on screen. */
    reading->step_timed = 0;
    pages_want(reading->pages, reading->page, reading->shown_page);
}

/* Puts the page being read on screen and lets it fade in, if its picture is ready. */
static void show_page_if_ready(Reading *reading)
{
    SDL_Texture *picture;
    if (!waiting_for_page(reading) || !pages_ready(reading->pages, reading->page, &picture)) {
        return;
    }
    reading->shown_page = reading->page;
    reading->shown_picture = picture;
    /* The page shown until now is no longer needed for the screen. */
    pages_want(reading->pages, reading->page, reading->shown_page);

    Uint32 now = SDL_GetTicks();
    reading->glide_from = shot_wanted(reading);
    reading->glide_to = reading->glide_from;
    reading->glide_start = 0;
    reading->fade_start = now;
    reading->marks_start = now;
    time_step(reading, reading->page_asked);
}

/* Starts a glide from what is on screen now to the current view of the same page. */
static void glide_to_current_view(Reading *reading)
{
    Uint32 now = SDL_GetTicks();
    reading->glide_from = shot_now(reading, now);
    reading->glide_to = shot_wanted(reading);
    reading->glide_start = now;
    reading->marks_start = now;
    time_step(reading, now);
}

static int something_is_moving(const Reading *reading, Uint32 now)
{
    return motion_running(reading->glide_start, GLIDE_MS, now) ||
           motion_running(reading->fade_start, FADE_MS, now) ||
           motion_running(reading->marks_start, MARKS_MS, now);
}

/* Where a rectangle of the upright screen lies on the real one. turn says
   which end of the Vita is up: 1 the right end, -1 the left. */
static SDL_FRect on_real_screen(SDL_FRect upright, int turn)
{
    if (turn > 0) {
        /* The top of the upright screen is the right edge of the real one. */
        return (SDL_FRect){SCREEN_WIDTH - (upright.y + upright.h), upright.x, upright.h, upright.w};
    }
    return (SDL_FRect){upright.y, SCREEN_HEIGHT - (upright.x + upright.w), upright.h, upright.w};
}

/* A touch on the real screen, as the upright screen sees it. */
static Input on_upright_screen(Input touch, int turn)
{
    Input upright = touch;
    if (turn > 0) {
        upright.x = touch.y;
        upright.y = SCREEN_WIDTH - touch.x;
        upright.dx = touch.dy;
        upright.dy = -touch.dx;
    } else {
        upright.x = SCREEN_HEIGHT - touch.y;
        upright.y = touch.x;
        upright.dx = -touch.dy;
        upright.dy = touch.dx;
    }
    return upright;
}

/* The page on screen, with everything outside the lit part dimmed. */
static void draw_page(const Reading *reading, Shot shot)
{
    if (reading->shown_page < 0) {
        return;
    }
    const Page *page = &reading->book.pages[reading->shown_page];
    SDL_FRect whole = {0, 0, (float)page->width, (float)page->height};
    SDL_FRect on_screen = shot_on_screen(shot, whole);
    SDL_FRect lit = shot_on_screen(shot, shot.lit);

    if (shot.turned == 0) {
        if (reading->shown_picture != NULL) {
            SDL_RenderCopyF(reading->renderer, reading->shown_picture, NULL, &on_screen);
        }
    } else {
        SDL_FRect turned = on_real_screen(on_screen, shot.turned);
        if (reading->shown_picture != NULL) {
            /* SDL turns a picture about the middle of the place it is given.
               So the place has the picture's upright shape, and its middle is
               where the middle of the turned picture belongs. */
            SDL_FRect place = {turned.x + (turned.w - on_screen.w) / 2,
                               turned.y + (turned.h - on_screen.h) / 2, on_screen.w, on_screen.h};
            double quarter = shot.turned > 0 ? 90 : -90; /* clockwise */
            SDL_RenderCopyExF(reading->renderer, reading->shown_picture, NULL, &place, quarter,
                              NULL, SDL_FLIP_NONE);
        }
        lit = on_real_screen(lit, shot.turned);
    }
    draw_dim_outside(reading->renderer, lit, (Uint8)shot.dim);
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

/* Fingers move and zoom the picture, which then stays where they left it. */
static void touch_picture(Reading *reading, Input input)
{
    if (reading->shown_page < 0 || waiting_for_page(reading)) {
        return;
    }
    Shot shot = shot_now(reading, SDL_GetTicks());
    if (shot.turned != 0) {
        input = on_upright_screen(input, shot.turned);
    }
    shot = shot_touched(shot, &reading->book.pages[reading->shown_page], input.x, input.y, input.dx,
                        input.dy, input.spread);
    reading->glide_from = shot;
    reading->glide_to = shot;
    reading->glide_start = 0;
}

/* The Vita was turned upright, or back. A page that turns with it is framed
   anew and fades in. */
static void turn_picture(Reading *reading)
{
    reading->turn = input_turn();
    if (waiting_for_page(reading) || !turns_with_vita(current_page(reading))) {
        return;
    }
    reading->glide_from = shot_wanted(reading);
    reading->glide_to = reading->glide_from;
    reading->glide_start = 0;
    reading->fade_start = SDL_GetTicks();
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

    if (reading->page == old_page && reading->view == old_view &&
        reading->whole_page == old_whole_page) {
        return;
    }
    update_window_title(reading);
    Uint32 now = SDL_GetTicks();
    if (reading->page != old_page) {
        ask_for_page(reading, now);
    }
    if (waiting_for_page(reading)) {
        /* The marks move at once, so the press shows before the picture does. */
        reading->marks_start = now;
        show_page_if_ready(reading);
    } else {
        glide_to_current_view(reading);
    }
}

ReadingEnd reading_run(SDL_Window *window, SDL_Renderer *renderer, const char *comic_dir)
{
    Reading reading = {0};
    reading.window = window;
    reading.renderer = renderer;
    reading.comic_dir = comic_dir;

    Uint32 start = SDL_GetTicks();
    if (book_load(&reading.book, comic_dir) != 0) {
        draw_failure(renderer);
        return READING_ENDED_BACK;
    }
    SDL_Log("Opened %s: %d pages, panels.json read in %u ms", comic_dir, reading.book.page_count,
            SDL_GetTicks() - start);
    reading.pages = pages_open(renderer, comic_dir, &reading.book);
    if (reading.pages == NULL) {
        draw_failure(renderer);
        book_free(&reading.book);
        return READING_ENDED_BACK;
    }
    reading.shown_page = -1;
    reading.turn = input_turn();
    ask_for_page(&reading, SDL_GetTicks());
    update_window_title(&reading);

    ReadingEnd end;
    for (;;) {
        Uint32 now = SDL_GetTicks();
        /* Taking in a loaded picture stalls drawing for a moment. So it waits
           until a glide or a fade is over, unless the reader is waiting for
           that very picture. The marks outlast both, so the loop is still
           running when they end. */
        if (waiting_for_page(&reading) || !(motion_running(reading.glide_start, GLIDE_MS, now) ||
                                            motion_running(reading.fade_start, FADE_MS, now))) {
            pages_take_loaded(reading.pages);
            show_page_if_ready(&reading);
            now = SDL_GetTicks();
        }
        draw_frame(&reading, now);
        int moving = something_is_moving(&reading, now);
        time_frame(&reading, now, SDL_GetTicks(), moving);

        Input input = input_next(moving);
        if (input.action == ACTION_BACK) {
            end = READING_ENDED_BACK;
            break;
        }
        if (input.action == ACTION_QUIT) {
            end = READING_ENDED_QUIT;
            break;
        }
        if (input.action == ACTION_TURN) {
            turn_picture(&reading);
        } else if (input.action == ACTION_DRAG || input.action == ACTION_PINCH) {
            touch_picture(&reading, input);
        } else if (input.action == ACTION_TAP && !waiting_for_page(&reading)) {
            /* Back to the panel as the reader frames it. */
            glide_to_current_view(&reading);
        } else {
            handle_action(&reading, input.action);
        }
    }

    pages_close(reading.pages);
    book_free(&reading.book);
    return end;
}
