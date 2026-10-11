/* Loads a picture file for drawing, and writes to the log how long that took.

   Loading has two steps. The first reads the file and turns it into pixels.
   It uses no renderer, so it may run on another thread while the main one
   keeps drawing. The second hands the pixels to the renderer; SDL allows
   that on the main thread only. */
#ifndef KOMAWARI_PICTURE_H
#define KOMAWARI_PICTURE_H

#include <SDL.h>

/* What the first step found out about a picture, for the log. Times are in milliseconds. */
typedef struct {
    int width;
    int height;
    int file_kb;
    Uint32 read_ms;        /* getting the file's bytes from the memory card */
    Uint32 decode_ms;      /* from JPEG bytes to pixels */
    Uint32 convert_ms;     /* from the decoder's pixel format to the renderer's */
    Uint32 decoded_format; /* the decoder's pixel format */
} PictureTiming;

/* The pixel format in which this renderer wants a picture without transparency. */
Uint32 picture_format(SDL_Renderer *renderer);

/* Step one: reads the file at path and returns its pixels in the given format,
   or NULL on failure, after logging the reason. A picture with see-through
   parts keeps its own format instead. Fills in timing. */
SDL_Surface *picture_read(const char *path, Uint32 format, PictureTiming *timing);

/* Step two: hands the pixels to the renderer and frees them. Returns NULL on
   failure, after logging the reason. Otherwise it writes the picture's line to
   the log: its size in pixels, the size of its file and the time of each step. */
SDL_Texture *picture_show(SDL_Renderer *renderer, SDL_Surface *pixels, const char *path,
                          const PictureTiming *timing);

/* Both steps, one after the other. */
SDL_Texture *picture_load(SDL_Renderer *renderer, const char *path);

#endif
