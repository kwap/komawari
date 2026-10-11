/* The pictures the shelf shows for each comic: its cover and the picture of
   its title. A second thread loads them, the ones near the middle of the
   shelf first, so the shelf is there at once and fills in. They are kept
   while a comic is read, so coming back to the shelf needs no loading. */
#ifndef KOMAWARI_COVERS_H
#define KOMAWARI_COVERS_H

#include <SDL.h>

#include "library.h"

typedef struct Covers Covers;

/* Starts the loading thread. Returns NULL on failure. The library must stay
   as it is until covers_close. */
Covers *covers_open(SDL_Renderer *renderer, const Library *library);

/* Waits for the loading thread to end and lets go of every picture. */
void covers_close(Covers *covers);

/* Says which comic is in the middle of the shelf. The pictures of that comic
   and of those near it are kept or loaded; those far away are let go. */
void covers_want(Covers *covers, int comic);

/* Takes in a picture that has finished loading, if there is one. When one
   finishes, the loading thread sends an SDL_USEREVENT, which wakes a main loop
   that sleeps in SDL_WaitEvent. */
void covers_take_loaded(Covers *covers);

/* A comic's cover, or the picture of its title. NULL while it is not here
   yet, and for a comic that has no such file. */
SDL_Texture *covers_cover(const Covers *covers, int comic);
SDL_Texture *covers_title(const Covers *covers, int comic);

#endif
