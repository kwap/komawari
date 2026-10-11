#include "place.h"

#include <SDL.h>

#include "file.h"

/* In the folder of all comics: the folder name of the comic read last. */
#define LAST_COMIC_FILE "last-comic.txt"
/* In a comic's folder: its page and view, counted from 1, as in "12 3". */
#define PLACE_FILE "place.txt"

/* Writes a short text file. A failure only costs the reader its saved place,
   so it is logged and otherwise ignored. */
static void write_text(const char *path, const char *text)
{
    SDL_RWops *file = SDL_RWFromFile(path, "w");
    if (file == NULL) {
        SDL_Log("Cannot write %s: %s", path, SDL_GetError());
        return;
    }
    SDL_RWwrite(file, text, 1, SDL_strlen(text));
    SDL_RWclose(file);
}

/* The last part of a path: a comic's folder name. */
static const char *folder_name(const char *dir)
{
    const char *slash = SDL_strrchr(dir, '/');
    return slash != NULL ? slash + 1 : dir;
}

int place_last_comic(const Library *library, const char *library_dir)
{
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%s/%s", library_dir, LAST_COMIC_FILE);
    size_t size;
    char *name = file_read(path, &size);
    if (name == NULL) {
        return 0;
    }
    int last = 0;
    for (int i = 0; i < library->count; i++) {
        if (SDL_strcmp(folder_name(library->comic_dirs[i]), name) == 0) {
            last = i;
        }
    }
    SDL_free(name);
    return last;
}

void place_remember_comic(const char *library_dir, const char *comic_dir)
{
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%s/%s", library_dir, LAST_COMIC_FILE);
    write_text(path, folder_name(comic_dir));
}

void place_load(const char *comic_dir, int *page, int *view)
{
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%s/%s", comic_dir, PLACE_FILE);
    size_t size;
    char *text = file_read(path, &size);
    if (text == NULL) {
        return;
    }
    int saved_page, saved_view;
    if (SDL_sscanf(text, "%d %d", &saved_page, &saved_view) == 2) {
        *page = saved_page - 1;
        *view = saved_view - 1;
    }
    SDL_free(text);
}

void place_save(const char *comic_dir, int page, int view)
{
    char path[1024];
    char text[32];
    SDL_snprintf(path, sizeof(path), "%s/%s", comic_dir, PLACE_FILE);
    SDL_snprintf(text, sizeof(text), "%d %d\n", page + 1, view + 1);
    write_text(path, text);
}
