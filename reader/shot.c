#include "shot.h"

#include "screen.h"

/* Darker when zoomed in on a panel, lighter when the whole page is shown. */
#define DIM_AROUND_PANEL 200.0f
#define DIM_ON_WHOLE_PAGE 120.0f
/* The closest the fingers can zoom in: this many screen pixels per page pixel. */
#define CLOSEST_ZOOM 2.0f
/* Each move of the fingers takes this much off the dimming, so that it is gone
   after a few moves and the whole picture can be looked at. */
#define DIM_LOST_PER_TOUCH 25.0f

/* The screen's width and height as a shot sees them: swapped for the upright screen. */
static float screen_width(int turned)
{
    return turned ? SCREEN_HEIGHT : SCREEN_WIDTH;
}

static float screen_height(int turned)
{
    return turned ? SCREEN_WIDTH : SCREEN_HEIGHT;
}

/* The smallest rectangle with the screen's shape that contains area, centred on it. */
static SDL_FRect fit_screen(SDL_FRect area, int turned)
{
    const float screen_shape = screen_width(turned) / screen_height(turned);
    SDL_FRect view = area;
    if (area.w / area.h > screen_shape) {
        /* Wider than the screen: add room above and below. */
        view.h = area.w / screen_shape;
        view.y = area.y - (view.h - area.h) / 2;
    } else {
        /* Taller than the screen: add room left and right. */
        view.w = area.h * screen_shape;
        view.x = area.x - (view.w - area.w) / 2;
    }
    return view;
}

Shot shot_of_view(const Page *page, int view, int whole_page, int turned)
{
    SDL_FRect whole = {0, 0, (float)page->width, (float)page->height};
    Shot shot;
    shot.turned = turned;
    shot.lit = whole;
    if (page->panel_count > 0) {
        const Panel *panel = &page->panels[view];
        shot.lit = (SDL_FRect){(float)panel->x, (float)panel->y, (float)panel->width,
                               (float)panel->height};
    }
    shot.view = fit_screen(whole_page ? whole : shot.lit, turned);
    shot.dim = whole_page ? DIM_ON_WHOLE_PAGE : DIM_AROUND_PANEL;
    return shot;
}

static float between(float a, float b, float t)
{
    return a + (b - a) * t;
}

static SDL_FRect rect_between(SDL_FRect a, SDL_FRect b, float t)
{
    return (SDL_FRect){between(a.x, b.x, t), between(a.y, b.y, t), between(a.w, b.w, t),
                       between(a.h, b.h, t)};
}

Shot shot_between(Shot a, Shot b, float t)
{
    Shot shot;
    shot.view = rect_between(a.view, b.view, t);
    shot.lit = rect_between(a.lit, b.lit, t);
    shot.dim = between(a.dim, b.dim, t);
    shot.turned = b.turned;
    return shot;
}

SDL_FRect shot_on_screen(Shot shot, SDL_FRect page_area)
{
    /* Screen pixels per page pixel. */
    float scale = screen_width(shot.turned) / shot.view.w;
    return (SDL_FRect){(page_area.x - shot.view.x) * scale, (page_area.y - shot.view.y) * scale,
                       page_area.w * scale, page_area.h * scale};
}

/* Keeps the page on the screen along one direction, across or down. start and
   size say which stretch of the page the screen shows now; the two after them
   say the same for before the fingers moved. Returns the start to use. */
static float keep_on_page(float start, float size, float page_size, float start_before,
                          float size_before)
{
    if (size >= page_size) {
        /* All of the page fits on the screen: it sits in the middle. */
        return (page_size - size) / 2;
    }
    /* The screen shows nothing beyond the edges of the page. But a panel at
       the edge is framed with some room beyond it. That much may stay, or the
       picture would jump at the first touch. */
    float beyond_start = SDL_max(-start_before, 0.0f);
    float beyond_end = SDL_max(start_before + size_before - page_size, 0.0f);
    return SDL_clamp(start, -beyond_start, page_size - size + beyond_end);
}

Shot shot_touched(Shot shot, const Page *page, float x, float y, float dx, float dy, float spread)
{
    SDL_FRect whole = {0, 0, (float)page->width, (float)page->height};
    SDL_FRect before = shot.view;
    /* The point of the page that was under the fingers before they moved. */
    float page_per_screen = shot.view.w / screen_width(shot.turned);
    float held_x = shot.view.x + (x - dx) * page_per_screen;
    float held_y = shot.view.y + (y - dy) * page_per_screen;

    float widest = fit_screen(whole, shot.turned).w;
    float narrowest = SDL_min(screen_width(shot.turned) / CLOSEST_ZOOM, widest);
    shot.view.w = SDL_clamp(shot.view.w / spread, narrowest, widest);
    shot.view.h = shot.view.w * screen_height(shot.turned) / screen_width(shot.turned);

    /* Put that point under the fingers again. */
    page_per_screen = shot.view.w / screen_width(shot.turned);
    shot.view.x = held_x - x * page_per_screen;
    shot.view.y = held_y - y * page_per_screen;

    shot.view.x = keep_on_page(shot.view.x, shot.view.w, whole.w, before.x, before.w);
    shot.view.y = keep_on_page(shot.view.y, shot.view.h, whole.h, before.y, before.h);

    shot.dim = SDL_max(shot.dim - DIM_LOST_PER_TOUCH, 0.0f);
    return shot;
}
