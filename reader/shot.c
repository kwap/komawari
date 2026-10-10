#include "shot.h"

#include "screen.h"

/* Darker when zoomed in on a panel, lighter when the whole page is shown. */
#define DIM_AROUND_PANEL 200.0f
#define DIM_ON_WHOLE_PAGE 120.0f

/* The smallest rectangle with the screen's shape that contains area, centred on it. */
static SDL_FRect fit_screen(SDL_FRect area)
{
    const float screen_shape = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
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

Shot shot_of_view(const Page *page, int view, int whole_page)
{
    SDL_FRect whole = {0, 0, (float)page->width, (float)page->height};
    Shot shot;
    shot.lit = whole;
    if (page->panel_count > 0) {
        const Panel *panel = &page->panels[view];
        shot.lit = (SDL_FRect){(float)panel->x, (float)panel->y, (float)panel->width,
                               (float)panel->height};
    }
    shot.view = fit_screen(whole_page ? whole : shot.lit);
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
    return shot;
}

SDL_FRect shot_on_screen(Shot shot, SDL_FRect page_area)
{
    /* Screen pixels per page pixel. */
    float scale = (float)SCREEN_WIDTH / shot.view.w;
    return (SDL_FRect){(page_area.x - shot.view.x) * scale, (page_area.y - shot.view.y) * scale,
                       page_area.w * scale, page_area.h * scale};
}
