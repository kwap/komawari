/* Timing of things that move on screen. Times are in milliseconds, as SDL_GetTicks gives them. */
#ifndef KOMAWARI_MOTION_H
#define KOMAWARI_MOTION_H

#include <SDL.h>

/* Is a movement that began at start and lasts duration still going on? */
int motion_running(Uint32 start, Uint32 duration, Uint32 now);

/* How far such a movement has come: 0 at its start, 1 once it is over.
   It starts and ends gently instead of at a constant speed. */
float motion_progress(Uint32 start, Uint32 duration, Uint32 now);

#endif
