#include "covers.h"

#include "picture.h"

/* The pictures of this many comics are kept at most: the one in the middle of
   the shelf and those nearest to it on either side. Together they take about
   18 MB of the graphics memory, which the pages of a comic need as well. */
#define KEPT_COMICS 25
/* The loading thread's stack. SDL's own choice on the Vita is 64 KB, which is
   little for a JPEG decoder. */
#define LOADER_STACK_BYTES (512 * 1024)

/* Each comic has two pictures. A picture's number is its comic's number times
   two, plus one of these. */
enum { COVER, TITLE, KINDS };
static const char *const FILE_NAMES[KINDS] = {"cover.jpg", "title.png"};

typedef struct {
    int tried;            /* 1 once the file was loaded, or found missing */
    SDL_Texture *picture; /* NULL until then, and for a missing file */
} Kept;

struct Covers {
    /* Used by the main thread only. */
    SDL_Renderer *renderer;
    Kept *kept; /* one for every picture of every comic */
    int middle; /* the comic in the middle of the shelf */
    /* For the log: a run of loading, from the first picture asked for until none is missing. */
    int run_pictures; /* pictures taken in during this run; -1 while no run is on */
    Uint32 run_start;

    /* Set once in covers_open and only read after that. */
    const Library *library;
    Uint32 format;

    /* Shared by both threads. Touch these only while holding the lock. */
    SDL_Thread *loader;
    SDL_mutex *lock;
    SDL_cond *wake_loader;      /* tells the loading thread to look at the fields below again */
    int closing;                /* 1 once covers_close was called */
    int asked;                  /* the picture to load next; -1 for none */
    int loading;                /* the picture being loaded right now; -1 for none */
    int loaded;                 /* a picture that is loaded and waits to be taken; -1 for none */
    SDL_Surface *loaded_pixels; /* its pixels; NULL if the file is missing */
    PictureTiming loaded_timing;
};

static void picture_path(const Covers *covers, int number, char *path, size_t path_size)
{
    SDL_snprintf(path, path_size, "%s/%s", covers->library->comic_dirs[number / KINDS],
                 FILE_NAMES[number % KINDS]);
}

/* What the loading thread does: it loads one picture after the other, as asked. */
static int loader_run(void *data)
{
    Covers *covers = data;
    SDL_LockMutex(covers->lock);
    for (;;) {
        /* Sleep until a picture is asked for and the one before it was taken. */
        while (!covers->closing && (covers->asked < 0 || covers->loaded >= 0)) {
            SDL_CondWait(covers->wake_loader, covers->lock);
        }
        if (covers->closing) {
            break;
        }
        int number = covers->asked;
        covers->asked = -1;
        covers->loading = number;
        SDL_UnlockMutex(covers->lock);

        char path[1024];
        picture_path(covers, number, path, sizeof(path));
        PictureTiming timing;
        SDL_Surface *pixels = picture_read(path, covers->format, &timing);

        SDL_LockMutex(covers->lock);
        covers->loading = -1;
        covers->loaded = number;
        covers->loaded_pixels = pixels;
        covers->loaded_timing = timing;
        /* The main loop may be asleep, waiting for a button. This wakes it. */
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_USEREVENT;
        SDL_PushEvent(&event);
    }
    SDL_UnlockMutex(covers->lock);
    return 0;
}

/* How many places apart two comics are on the shelf, going round the shorter way. */
static int places_apart(const Covers *covers, int a, int b)
{
    int apart = SDL_abs(a - b);
    return SDL_min(apart, covers->library->count - apart);
}

static int is_near_middle(const Covers *covers, int comic)
{
    return places_apart(covers, comic, covers->middle) <= KEPT_COMICS / 2;
}

/* Asks the loading thread for the missing picture nearest to the middle of the
   shelf. At the same distance the covers come before the titles. */
static void ask_for_missing(Covers *covers)
{
    int count = covers->library->count;
    SDL_LockMutex(covers->lock);
    covers->asked = -1;
    for (int apart = 0; apart <= KEPT_COMICS / 2 && covers->asked < 0; apart++) {
        for (int kind = 0; kind < KINDS && covers->asked < 0; kind++) {
            for (int side = 1; side >= -1 && covers->asked < 0; side -= 2) {
                int comic = ((covers->middle + side * apart) % count + count) % count;
                int number = comic * KINDS + kind;
                if (places_apart(covers, comic, covers->middle) == apart &&
                    !covers->kept[number].tried && number != covers->loading &&
                    number != covers->loaded) {
                    covers->asked = number;
                }
            }
        }
    }
    int waiting_for_any = covers->asked >= 0 || covers->loading >= 0 || covers->loaded >= 0;
    if (covers->asked >= 0) {
        SDL_CondSignal(covers->wake_loader);
    }
    SDL_UnlockMutex(covers->lock);

    if (waiting_for_any && covers->run_pictures < 0) {
        covers->run_pictures = 0;
        covers->run_start = SDL_GetTicks();
    } else if (!waiting_for_any && covers->run_pictures >= 0) {
        SDL_Log("Shelf: %d pictures loaded in %u ms", covers->run_pictures,
                SDL_GetTicks() - covers->run_start);
        covers->run_pictures = -1;
    }
}

Covers *covers_open(SDL_Renderer *renderer, const Library *library)
{
    Covers *covers = SDL_calloc(1, sizeof(Covers));
    if (covers == NULL) {
        return NULL;
    }
    covers->renderer = renderer;
    covers->library = library;
    covers->format = picture_format(renderer);
    covers->middle = 0;
    covers->run_pictures = -1;
    covers->asked = -1;
    covers->loading = -1;
    covers->loaded = -1;
    covers->kept = SDL_calloc((size_t)library->count * KINDS, sizeof(Kept));
    covers->lock = SDL_CreateMutex();
    covers->wake_loader = SDL_CreateCond();
    if (covers->kept != NULL && covers->lock != NULL && covers->wake_loader != NULL) {
        covers->loader =
            SDL_CreateThreadWithStackSize(loader_run, "cover loader", LOADER_STACK_BYTES, covers);
    }
    if (covers->loader == NULL) {
        SDL_Log("Cannot start the cover loader: %s", SDL_GetError());
        SDL_DestroyCond(covers->wake_loader);
        SDL_DestroyMutex(covers->lock);
        SDL_free(covers->kept);
        SDL_free(covers);
        return NULL;
    }
    return covers;
}

void covers_close(Covers *covers)
{
    SDL_LockMutex(covers->lock);
    covers->closing = 1;
    SDL_CondSignal(covers->wake_loader);
    SDL_UnlockMutex(covers->lock);
    /* A cover takes a tenth of a second to load, so waiting for the thread is no burden. */
    SDL_WaitThread(covers->loader, NULL);

    SDL_FreeSurface(covers->loaded_pixels);
    for (int number = 0; number < covers->library->count * KINDS; number++) {
        SDL_DestroyTexture(covers->kept[number].picture);
    }
    SDL_DestroyCond(covers->wake_loader);
    SDL_DestroyMutex(covers->lock);
    SDL_free(covers->kept);
    SDL_free(covers);
}

void covers_want(Covers *covers, int comic)
{
    covers->middle = comic;
    for (int number = 0; number < covers->library->count * KINDS; number++) {
        Kept *kept = &covers->kept[number];
        if (kept->tried && !is_near_middle(covers, number / KINDS)) {
            SDL_DestroyTexture(kept->picture);
            kept->picture = NULL;
            kept->tried = 0;
        }
    }
    ask_for_missing(covers);
}

void covers_take_loaded(Covers *covers)
{
    SDL_LockMutex(covers->lock);
    int number = covers->loaded;
    SDL_Surface *pixels = covers->loaded_pixels;
    PictureTiming timing = covers->loaded_timing;
    covers->loaded = -1;
    covers->loaded_pixels = NULL;
    SDL_CondSignal(covers->wake_loader);
    SDL_UnlockMutex(covers->lock);
    if (number < 0) {
        return;
    }
    if (!is_near_middle(covers, number / KINDS)) {
        /* The shelf moved on while this one was loading. */
        SDL_FreeSurface(pixels);
    } else {
        Kept *kept = &covers->kept[number];
        kept->tried = 1;
        covers->run_pictures += 1;
        if (pixels != NULL) {
            char path[1024];
            picture_path(covers, number, path, sizeof(path));
            kept->picture = picture_show(covers->renderer, pixels, path, &timing);
        }
    }
    ask_for_missing(covers);
}

SDL_Texture *covers_cover(const Covers *covers, int comic)
{
    return covers->kept[comic * KINDS + COVER].picture;
}

SDL_Texture *covers_title(const Covers *covers, int comic)
{
    return covers->kept[comic * KINDS + TITLE].picture;
}
