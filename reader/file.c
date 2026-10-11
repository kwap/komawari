#include "file.h"

#include <errno.h>
#include <stdio.h>

#include <SDL.h>

/* How much of a file is fetched from the memory card in one go. */
#define READ_BUFFER_BYTES (256 * 1024)

/* This uses C's own file functions, not SDL's, because only they can set the
   size of the read buffer. The Vita SDK's C library reads 1 KB at a time
   unless told otherwise, and SDL reads files through it. That gave 2.8 MB per
   second; large reads are about four times as fast. */
void *file_read(const char *path, size_t *size)
{
    *size = 0;
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    setvbuf(file, NULL, _IOFBF, READ_BUFFER_BYTES);

    char *bytes = NULL;
    long length = -1;
    if (fseek(file, 0, SEEK_END) == 0) {
        length = ftell(file);
    }
    if (length >= 0 && fseek(file, 0, SEEK_SET) == 0) {
        bytes = SDL_malloc((size_t)length + 1);
        if (bytes != NULL && fread(bytes, 1, (size_t)length, file) != (size_t)length) {
            SDL_free(bytes);
            bytes = NULL;
        }
    }
    /* Closing the file must not replace the reason for a failure. */
    int reason = errno;
    fclose(file);
    errno = reason;
    if (bytes == NULL) {
        return NULL;
    }
    bytes[length] = '\0';
    *size = (size_t)length;
    return bytes;
}
