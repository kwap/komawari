/* The pictures of a comic's pages, loaded in the background.

   A second thread loads them, so the main one never stops drawing. It loads
   the page being read first and then the page after it, so that turning the
   page needs no waiting. The page before is kept while it is still there. */
#ifndef KOMAWARI_PAGES_H
#define KOMAWARI_PAGES_H

#include <SDL.h>

#include "book.h"

typedef struct Pages Pages;

/* Starts the loading thread for the comic in comic_dir. Returns NULL on failure.
   The folder name and the book must stay as they are until pages_close. */
Pages *pages_open(SDL_Renderer *renderer, const char *comic_dir, const Book *book);

/* Lets go of every picture. The pictures it handed out must not be drawn after this. */
void pages_close(Pages *pages);

/* Says which page is being read and which one is on screen (-1 for none).
   Those two and the pages next to the one being read are kept or loaded;
   every other picture is let go. */
void pages_want(Pages *pages, int reading_page, int shown_page);

/* Takes in a picture that has finished loading, if there is one. This stalls
   drawing for a moment. When a picture finishes, the loading thread sends an
   SDL_USEREVENT, which wakes a main loop that sleeps in SDL_WaitEvent. */
void pages_take_loaded(Pages *pages);

/* Returns 1 if the page's picture is here, and stores it in picture; that is
   NULL for a page that could not be loaded. Returns 0 while it is not here yet. */
int pages_ready(const Pages *pages, int page, SDL_Texture **picture);

#endif
