#include "picture.h"

#include <errno.h>
#include <string.h>

#include <SDL_image.h>

#include "file.h"

Uint32 picture_format(SDL_Renderer *renderer)
{
    /* The same choice SDL_CreateTextureFromSurface makes for a decoded JPEG:
       the renderer's first format without transparency. On the Vita that is
       16-bit colour (RGB565). */
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info) != 0 || info.num_texture_formats == 0) {
        return SDL_PIXELFORMAT_ARGB8888;
    }
    for (Uint32 i = 0; i < info.num_texture_formats; i++) {
        Uint32 format = info.texture_formats[i];
        if (!SDL_ISPIXELFORMAT_FOURCC(format) && !SDL_ISPIXELFORMAT_ALPHA(format)) {
            return format;
        }
    }
    return info.texture_formats[0];
}

/* Turns pixels of three bytes (red, green, blue), as a colour JPEG decodes to,
   into 16-bit colour: 5 bits of red, 6 of green and 5 of blue. Returns NULL
   when memory runs out.

   SDL can do this as well, but only with its slowest, general converter. On
   the Vita that took 0.8 seconds for a page, a third of its loading time. */
static SDL_Surface *to_16_bit_colour(const SDL_Surface *decoded)
{
    SDL_Surface *pixels =
        SDL_CreateRGBSurfaceWithFormat(0, decoded->w, decoded->h, 16, SDL_PIXELFORMAT_RGB565);
    if (pixels == NULL) {
        return NULL;
    }
    for (int y = 0; y < decoded->h; y++) {
        const Uint8 *from = (const Uint8 *)decoded->pixels + (size_t)y * (size_t)decoded->pitch;
        Uint16 *to = (Uint16 *)((Uint8 *)pixels->pixels + (size_t)y * (size_t)pixels->pitch);
        for (int x = 0; x < decoded->w; x++) {
            *to = (Uint16)(((from[0] >> 3) << 11) | ((from[1] >> 2) << 5) | (from[2] >> 3));
            from += 3;
            to += 1;
        }
    }
    return pixels;
}

SDL_Surface *picture_read(const char *path, Uint32 format, PictureTiming *timing)
{
    SDL_zerop(timing);
    Uint32 start = SDL_GetTicks();
    size_t file_size = 0;
    void *file = file_read(path, &file_size);
    if (file == NULL) {
        SDL_Log("Cannot read %s: %s", path, strerror(errno));
        return NULL;
    }
    Uint32 read_done = SDL_GetTicks();

    SDL_Surface *decoded = IMG_Load_RW(SDL_RWFromConstMem(file, (int)file_size), 1);
    SDL_free(file);
    if (decoded == NULL) {
        SDL_Log("Cannot decode %s: %s", path, SDL_GetError());
        return NULL;
    }
    Uint32 decode_done = SDL_GetTicks();

    timing->width = decoded->w;
    timing->height = decoded->h;
    timing->file_kb = (int)(file_size / 1024);
    timing->decoded_format = decoded->format->format;

    /* A picture with see-through parts stays as it was decoded: the renderer
       takes that format as it is. */
    SDL_Surface *pixels = decoded;
    if (decoded->format->Amask == 0 && decoded->format->format != format) {
        if (decoded->format->format == SDL_PIXELFORMAT_RGB24 && format == SDL_PIXELFORMAT_RGB565) {
            pixels = to_16_bit_colour(decoded);
        } else {
            pixels = SDL_ConvertSurfaceFormat(decoded, format, 0);
        }
        SDL_FreeSurface(decoded);
        if (pixels == NULL) {
            SDL_Log("Cannot convert %s: %s", path, SDL_GetError());
            return NULL;
        }
    }
    timing->read_ms = read_done - start;
    timing->decode_ms = decode_done - read_done;
    timing->convert_ms = SDL_GetTicks() - decode_done;
    return pixels;
}

SDL_Texture *picture_show(SDL_Renderer *renderer, SDL_Surface *pixels, const char *path,
                          const PictureTiming *timing)
{
    /* The pixels are already in the renderer's format, so this only copies
       them to the graphics chip's memory. */
    Uint32 start = SDL_GetTicks();
    SDL_Texture *picture = SDL_CreateTextureFromSurface(renderer, pixels);
    Uint32 format = pixels->format->format;
    SDL_FreeSurface(pixels);
    if (picture == NULL) {
        SDL_Log("Cannot hand %s to the screen: %s", path, SDL_GetError());
        return NULL;
    }
    Uint32 show_ms = SDL_GetTicks() - start;

    SDL_Log("Picture %s: %d x %d, %d KB. Read %u ms, decode %u ms, convert %u ms, "
            "to screen %u ms, total %u ms. %s to %s",
            path, timing->width, timing->height, timing->file_kb, timing->read_ms,
            timing->decode_ms, timing->convert_ms, show_ms,
            timing->read_ms + timing->decode_ms + timing->convert_ms + show_ms,
            SDL_GetPixelFormatName(timing->decoded_format), SDL_GetPixelFormatName(format));
    return picture;
}

SDL_Texture *picture_load(SDL_Renderer *renderer, const char *path)
{
    PictureTiming timing;
    SDL_Surface *pixels = picture_read(path, picture_format(renderer), &timing);
    if (pixels == NULL) {
        return NULL;
    }
    return picture_show(renderer, pixels, path, &timing);
}
