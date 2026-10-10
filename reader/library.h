/* The comics found on the memory card: one folder per comic. */
#ifndef KOMAWARI_LIBRARY_H
#define KOMAWARI_LIBRARY_H

typedef struct {
    int count;
    char **comic_dirs; /* the folder of each comic, sorted by name */
} Library;

/* Finds every folder inside library_dir that holds a panels.json.
   Returns 0 on success, even if no comic was found. On failure it logs the
   reason and returns -1. */
int library_load(Library *library, const char *library_dir);

void library_free(Library *library);

#endif
