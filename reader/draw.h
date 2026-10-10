/* Simple shapes drawn over the pictures. Positions are in screen pixels.
   For see-through shapes, alpha 255 is solid and 0 is invisible. */
#ifndef KOMAWARI_DRAW_H
#define KOMAWARI_DRAW_H

#include <SDL.h>

/* Fills a rectangle with a grey: 0 is black, 255 is white. */
void draw_fill(SDL_Renderer *renderer, SDL_FRect area, Uint8 grey, Uint8 alpha);

/* Darkens the whole screen except the given rectangle. */
void draw_dim_outside(SDL_Renderer *renderer, SDL_FRect lit, Uint8 alpha);

/* A row of small squares near the bottom of the screen, one per item, with the
   current item bright. Nothing is drawn for fewer than two items. */
void draw_squares(SDL_Renderer *renderer, int count, int current, Uint8 alpha);

/* The Vita's Cross button as a small picture: a ring with an X in it. Shown
   where pressing Cross does something, because the reader draws no text. */
void draw_cross_button(SDL_Renderer *renderer, float centre_x, float centre_y, float radius,
                       Uint8 alpha);

/* A thin bar along the bottom edge. done runs from 0 (empty) to 1 (full). */
void draw_progress_bar(SDL_Renderer *renderer, float done, Uint8 alpha);

/* Shows a red screen for a few seconds, so a failure is visible on a device
   where the log cannot be seen. */
void draw_failure(SDL_Renderer *renderer);

#endif
