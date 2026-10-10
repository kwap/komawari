/* The shelf: the screen where a comic is chosen by its cover.

   Other comic: Left and Right arrows  | D-pad left and right, L and R
   Open it:     Space or Enter         | Cross or START

   A small picture of the Cross button under the chosen cover shows how to open it. */
#ifndef KOMAWARI_SHELF_H
#define KOMAWARI_SHELF_H

#include <SDL.h>

#include "library.h"

/* Lets the user pick a comic, starting with the one at index chosen.
   Returns the index of the comic to open, or -1 if the user closed the reader. */
int shelf_run(SDL_Window *window, SDL_Renderer *renderer, const Library *library, int chosen);

#endif
