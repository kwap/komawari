/* Reads a whole file from the memory card into memory, in large pieces. */
#ifndef KOMAWARI_FILE_H
#define KOMAWARI_FILE_H

#include <stddef.h>

/* Reads the file at path and stores its size in size. Returns its bytes, with
   a zero byte after them so that a text file can be used as a string. Returns
   NULL on failure, with the reason in errno. The caller frees the bytes with
   SDL_free. */
void *file_read(const char *path, size_t *size);

#endif
