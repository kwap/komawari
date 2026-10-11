/* The reading screen: one comic, shown panel by panel.

   Next panel:         Right arrow, Space or Enter | D-pad right, R or Cross
   Previous panel:     Left arrow                  | D-pad left or L
   Whole page / panel: Up arrow                    | D-pad up or Triangle
   Back to the comics: Backspace                   | Circle

   On the touch screen: two fingers spread or pinch the picture, one finger
   drags it, and a tap puts it back. (Mouse: wheel, left button, click.) */
#ifndef KOMAWARI_READING_H
#define KOMAWARI_READING_H

#include <SDL.h>

typedef enum {
    READING_ENDED_BACK, /* the user went back to choose another comic */
    READING_ENDED_QUIT  /* the user closed the reader */
} ReadingEnd;

/* Shows the comic in comic_dir until the user leaves it. */
ReadingEnd reading_run(SDL_Window *window, SDL_Renderer *renderer, const char *comic_dir);

#endif
