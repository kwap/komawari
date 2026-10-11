/* Remembers where the reader stopped: which comic was read last, and the page
   and panel in every comic. Each is a small text file on the memory card, so
   it is still there after the reader was closed. */
#ifndef KOMAWARI_PLACE_H
#define KOMAWARI_PLACE_H

#include "library.h"

/* The index in the library of the comic that was read last, or 0 if there is
   none, or if it is no longer there. */
int place_last_comic(const Library *library, const char *library_dir);

/* Notes that this comic is the one being read. */
void place_remember_comic(const char *library_dir, const char *comic_dir);

/* Reads the place in a comic into page and view, both counted from 0. They
   are left as they are if no place was saved. The caller checks that the
   comic still has such a page and view. */
void place_load(const char *comic_dir, int *page, int *view);

/* Saves the place in a comic. */
void place_save(const char *comic_dir, int page, int view);

#endif
