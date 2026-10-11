/* The shelf: the screen where a comic is chosen by its cover. Under the cover
   in the middle stand the comic's title, subtitle and author, which
   `komawari prepare` drew into a picture (title.png).

   Other comic: Left and Right arrows  | D-pad left and right, L and R
   Open it:     Space or Enter         | Cross or START

   On the touch screen: a finger slides the covers along, a tap on the cover in
   the middle opens it, and a tap on a cover beside it brings that one to the
   middle. (Mouse: drag and click.)

   With three comics or more the covers form a ring: after the last one comes
   the first again. A small picture of the Cross button under the chosen cover
   shows how to open it. */
#ifndef KOMAWARI_SHELF_H
#define KOMAWARI_SHELF_H

#include <SDL.h>

#include "covers.h"
#include "library.h"

/* Lets the user pick a comic, starting with the one at index chosen. The
   covers appear as they are loaded; until then a comic has a grey box.
   Returns the index of the comic to open, or -1 if the user closed the reader. */
int shelf_run(SDL_Window *window, SDL_Renderer *renderer, const Library *library, Covers *covers,
              int chosen);

#endif
