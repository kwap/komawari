/* A shot says how a page is framed on the screen, like a camera position:
   which part of the page fills the screen, and which part is lit. */
#ifndef KOMAWARI_SHOT_H
#define KOMAWARI_SHOT_H

#include <SDL.h>

#include "book.h"

/* The rectangles are in page pixels. */
typedef struct {
    SDL_FRect view; /* the part of the page that fills the screen */
    SDL_FRect lit;  /* the part shown at full brightness */
    float dim;      /* how dark the rest is: 0 is unchanged, 255 is black */
} Shot;

/* The shot for one view of a page: zoomed in on its panel, or showing the
   whole page with that panel lit. */
Shot shot_of_view(const Page *page, int view, int whole_page);

/* The shot part of the way from a to b. t runs from 0 (a) to 1 (b). */
Shot shot_between(Shot a, Shot b, float t);

/* Where a rectangle of the page lands on the screen in this shot. */
SDL_FRect shot_on_screen(Shot shot, SDL_FRect page_area);

/* The shot after fingers moved the picture. (x, y) is the point on the screen
   between the fingers; it came there from (x - dx, y - dy), and the fingers
   spread to spread times their distance. The picture stays under the fingers.
   It gets no smaller than the whole page and no larger than twice its pixels,
   and cannot be pushed off the screen. The dimming around the panel goes. */
Shot shot_touched(Shot shot, const Page *page, float x, float y, float dx, float dy, float spread);

#endif
