#include "library.h"

#include <SDL.h>
/* SDL2 has no way to list a folder, so this uses the POSIX functions, which
   macOS, Linux and the Vita SDK all provide. */
#include <dirent.h>

static int has_panels_file(const char *comic_dir)
{
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%s/panels.json", comic_dir);
    SDL_RWops *file = SDL_RWFromFile(path, "rb");
    if (file == NULL) {
        return 0;
    }
    SDL_RWclose(file);
    return 1;
}

static int add_comic(Library *library, const char *comic_dir)
{
    char **longer = SDL_realloc(library->comic_dirs, (library->count + 1) * sizeof(char *));
    char *copy = SDL_strdup(comic_dir);
    if (longer == NULL || copy == NULL) {
        SDL_free(copy);
        return -1;
    }
    library->comic_dirs = longer;
    library->comic_dirs[library->count] = copy;
    library->count += 1;
    return 0;
}

/* For SDL_qsort: orders two entries of comic_dirs by name. */
static int compare_dirs(const void *a, const void *b)
{
    return SDL_strcmp(*(char *const *)a, *(char *const *)b);
}

int library_load(Library *library, const char *library_dir)
{
    library->count = 0;
    library->comic_dirs = NULL;

    DIR *dir = opendir(library_dir);
    if (dir == NULL) {
        SDL_Log("Cannot open the comics folder %s", library_dir);
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue; /* ".", ".." and hidden files */
        }
        char comic_dir[1024];
        SDL_snprintf(comic_dir, sizeof(comic_dir), "%s/%s", library_dir, entry->d_name);
        if (has_panels_file(comic_dir) && add_comic(library, comic_dir) != 0) {
            SDL_Log("Out of memory while listing comics");
            closedir(dir);
            library_free(library);
            return -1;
        }
    }
    closedir(dir);

    SDL_qsort(library->comic_dirs, library->count, sizeof(char *), compare_dirs);
    return 0;
}

void library_free(Library *library)
{
    for (int i = 0; i < library->count; i++) {
        SDL_free(library->comic_dirs[i]);
    }
    SDL_free(library->comic_dirs);
    library->comic_dirs = NULL;
    library->count = 0;
}
