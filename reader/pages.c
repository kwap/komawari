#include "pages.h"

#include "picture.h"

/* The most pictures kept at once: the page being read, the one before it, the
   one after it, and the one still on screen while another is loading. */
#define KEPT_MAX 4
/* The loading thread's stack. SDL's own choice on the Vita is 64 KB, which is
   little for a JPEG decoder. */
#define LOADER_STACK_BYTES (512 * 1024)

typedef struct {
    int page;             /* index into the book's pages; -1 while this place is free */
    SDL_Texture *picture; /* NULL for a page that could not be loaded */
} Kept;

struct Pages {
    /* Used by the main thread only. */
    SDL_Renderer *renderer;
    Kept kept[KEPT_MAX];
    int reading_page;
    int shown_page;
    int no_room_page; /* a page loaded ahead that the graphics memory had no room for; else -1 */

    /* Set once in pages_open and only read after that. */
    const char *comic_dir;
    const Book *book;
    Uint32 format;

    /* Shared by both threads. Touch these only while holding the lock. */
    SDL_mutex *lock;
    SDL_cond *wake_loader;      /* tells the loading thread to look at the fields below again */
    int closing;                /* 1 once pages_close was called */
    int asked_page;             /* the page to load next; -1 for none */
    int loading_page;           /* the page being loaded right now; -1 for none */
    int loaded_page;            /* a page that is loaded and waits to be taken; -1 for none */
    SDL_Surface *loaded_pixels; /* its pixels; NULL if it could not be loaded */
    PictureTiming loaded_timing;
};

static void page_path(const Pages *pages, int page, char *path, size_t path_size)
{
    SDL_snprintf(path, path_size, "%s/pages/%s", pages->comic_dir, pages->book->pages[page].file);
}

/* What the loading thread does: it loads one page after the other, as asked. */
static int loader_run(void *data)
{
    Pages *pages = data;
    SDL_LockMutex(pages->lock);
    for (;;) {
        /* Sleep until a page is asked for and the page before it was taken. */
        while (!pages->closing && (pages->asked_page < 0 || pages->loaded_page >= 0)) {
            SDL_CondWait(pages->wake_loader, pages->lock);
        }
        if (pages->closing) {
            break;
        }
        int page = pages->asked_page;
        pages->asked_page = -1;
        pages->loading_page = page;
        /* The book may be gone once the lock is let go after pages_close, so
           the file's name is read from it now. */
        char path[1024];
        page_path(pages, page, path, sizeof(path));
        Uint32 format = pages->format;
        SDL_UnlockMutex(pages->lock);

        PictureTiming timing;
        SDL_Surface *pixels = picture_read(path, format, &timing);

        SDL_LockMutex(pages->lock);
        pages->loading_page = -1;
        if (pages->closing) {
            SDL_FreeSurface(pixels);
            break;
        }
        pages->loaded_page = page;
        pages->loaded_pixels = pixels;
        pages->loaded_timing = timing;
        /* The main loop may be asleep, waiting for a button. This wakes it. */
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_USEREVENT;
        SDL_PushEvent(&event);
    }
    SDL_UnlockMutex(pages->lock);

    /* pages_close did not wait for this thread, so the thread tidies up after itself. */
    SDL_DestroyCond(pages->wake_loader);
    SDL_DestroyMutex(pages->lock);
    SDL_free(pages);
    return 0;
}

static Kept *find_kept(Pages *pages, int page)
{
    for (int i = 0; i < KEPT_MAX; i++) {
        if (pages->kept[i].page == page) {
            return &pages->kept[i];
        }
    }
    return NULL;
}

static int is_wanted(const Pages *pages, int page)
{
    return page == pages->shown_page ||
           (page >= pages->reading_page - 1 && page <= pages->reading_page + 1);
}

/* Asks the loading thread for the first picture that is missing: the page
   being read, or else the page after it. */
static void ask_for_missing(Pages *pages)
{
    SDL_LockMutex(pages->lock);
    pages->asked_page = -1;
    for (int page = pages->reading_page; page <= pages->reading_page + 1; page++) {
        int is_ahead = page != pages->reading_page;
        if (page >= pages->book->page_count || (is_ahead && page == pages->no_room_page)) {
            continue;
        }
        if (find_kept(pages, page) != NULL || page == pages->loading_page ||
            page == pages->loaded_page) {
            continue;
        }
        pages->asked_page = page;
        SDL_CondSignal(pages->wake_loader);
        break;
    }
    SDL_UnlockMutex(pages->lock);
}

Pages *pages_open(SDL_Renderer *renderer, const char *comic_dir, const Book *book)
{
    Pages *pages = SDL_calloc(1, sizeof(Pages));
    if (pages == NULL) {
        return NULL;
    }
    pages->renderer = renderer;
    pages->comic_dir = comic_dir;
    pages->book = book;
    pages->format = picture_format(renderer);
    for (int i = 0; i < KEPT_MAX; i++) {
        pages->kept[i].page = -1;
    }
    pages->reading_page = -1;
    pages->shown_page = -1;
    pages->no_room_page = -1;
    pages->asked_page = -1;
    pages->loading_page = -1;
    pages->loaded_page = -1;

    pages->lock = SDL_CreateMutex();
    pages->wake_loader = SDL_CreateCond();
    SDL_Thread *loader = NULL;
    if (pages->lock != NULL && pages->wake_loader != NULL) {
        loader =
            SDL_CreateThreadWithStackSize(loader_run, "page loader", LOADER_STACK_BYTES, pages);
    }
    if (loader == NULL) {
        SDL_Log("Cannot start the page loader: %s", SDL_GetError());
        SDL_DestroyCond(pages->wake_loader);
        SDL_DestroyMutex(pages->lock);
        SDL_free(pages);
        return NULL;
    }
    /* Nobody waits for the thread to end: see pages_close. */
    SDL_DetachThread(loader);
    return pages;
}

void pages_close(Pages *pages)
{
    for (int i = 0; i < KEPT_MAX; i++) {
        SDL_DestroyTexture(pages->kept[i].picture);
    }
    /* The loading thread may be in the middle of a picture, which can take
       seconds and cannot be interrupted. Instead of waiting for it, it is told
       to stop; it frees the rest, this struct included, when it gets there. */
    SDL_LockMutex(pages->lock);
    pages->closing = 1;
    SDL_FreeSurface(pages->loaded_pixels);
    pages->loaded_pixels = NULL;
    SDL_CondSignal(pages->wake_loader);
    SDL_UnlockMutex(pages->lock);
}

void pages_want(Pages *pages, int reading_page, int shown_page)
{
    if (reading_page != pages->reading_page) {
        pages->no_room_page = -1;
    }
    pages->reading_page = reading_page;
    pages->shown_page = shown_page;
    for (int i = 0; i < KEPT_MAX; i++) {
        Kept *kept = &pages->kept[i];
        if (kept->page >= 0 && !is_wanted(pages, kept->page)) {
            SDL_DestroyTexture(kept->picture);
            kept->picture = NULL;
            kept->page = -1;
        }
    }
    ask_for_missing(pages);
}

void pages_take_loaded(Pages *pages)
{
    SDL_LockMutex(pages->lock);
    int page = pages->loaded_page;
    SDL_Surface *pixels = pages->loaded_pixels;
    PictureTiming timing = pages->loaded_timing;
    pages->loaded_page = -1;
    pages->loaded_pixels = NULL;
    SDL_CondSignal(pages->wake_loader);
    SDL_UnlockMutex(pages->lock);
    if (page < 0) {
        return;
    }
    if (!is_wanted(pages, page)) {
        /* The reader moved on while this one was loading. */
        SDL_FreeSurface(pixels);
        return;
    }

    SDL_Texture *picture = NULL;
    if (pixels != NULL) {
        char path[1024];
        page_path(pages, page, path, sizeof(path));
        picture = picture_show(pages->renderer, pixels, path, &timing);
        if (picture == NULL && page != pages->reading_page) {
            /* No room for a page loaded ahead. It is loaded again when the
               reader gets there and the pages behind have been let go. */
            pages->no_room_page = page;
            ask_for_missing(pages);
            return;
        }
    }
    /* Every kept page is a wanted one, and at most KEPT_MAX pages are wanted,
       so there is a free place for this one. */
    Kept *place = find_kept(pages, -1);
    place->page = page;
    place->picture = picture;
    ask_for_missing(pages);
}

int pages_ready(const Pages *pages, int page, SDL_Texture **picture)
{
    for (int i = 0; i < KEPT_MAX; i++) {
        if (pages->kept[i].page == page) {
            *picture = pages->kept[i].picture;
            return 1;
        }
    }
    return 0;
}
