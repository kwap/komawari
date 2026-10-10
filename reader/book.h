/* A prepared comic as the reader holds it in memory. See docs/panels-format.md. */
#ifndef KOMAWARI_BOOK_H
#define KOMAWARI_BOOK_H

/* One panel rectangle, in pixels of its page image. */
typedef struct {
    int x;
    int y;
    int width;
    int height;
} Panel;

typedef struct {
    char *file; /* file name inside the comic's pages/ folder */
    int width;  /* size of the page image */
    int height;
    int panel_count; /* 0 means: show the whole page */
    Panel *panels;   /* in reading order */
} Page;

typedef struct {
    int page_count;
    Page *pages; /* in reading order */
} Book;

/* Reads <book_dir>/panels.json into book. Returns 0 on success.
   On failure it logs the reason and returns -1. */
int book_load(Book *book, const char *book_dir);

void book_free(Book *book);

#endif
