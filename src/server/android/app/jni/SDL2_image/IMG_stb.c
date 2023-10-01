/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "SDL_image.h"

#ifdef USE_STBIMAGE

#ifndef INT_MAX
#define INT_MAX 0x7FFFFFFF
#endif

#define malloc SDL_malloc
#define realloc SDL_realloc
#define free SDL_free
#undef memcpy
#define memcpy SDL_memcpy
#undef memset
#define memset SDL_memset
#undef strcmp
#define strcmp SDL_strcmp
#undef strncmp
#define strncmp SDL_strncmp
#define strtol SDL_strtol

#define pow SDL_pow
#define ldexp SDL_scalbn

#define STB_IMAGE_STATIC
#define STBI_NO_THREAD_LOCALS
#define STBI_FAILURE_USERMSG
#if defined(__ARM_NEON)
#define STBI_NEON
#endif
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_ASSERT SDL_assert
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static int IMG_LoadSTB_RW_read(void *user, char *data, int size)
{
    return (int) SDL_RWread((SDL_RWops*) user, data, 1, size);
}

static void IMG_LoadSTB_RW_skip(void *user, int n)
{
    SDL_RWseek((SDL_RWops*) user, n, RW_SEEK_CUR);
}

static int IMG_LoadSTB_RW_eof(void *user)
{
    /* FIXME: Do we not have a way to detect EOF? -flibit */
    size_t bytes, filler;
    SDL_RWops *src = (SDL_RWops*) user;
    bytes = SDL_RWread(src, &filler, 1, 1);
    if (bytes != 1) { /* FIXME: Could also be an error... */
        return 1;
    }
    SDL_RWseek(src, -1, RW_SEEK_CUR);
    return 0;
}

SDL_Surface *IMG_LoadSTB_RW(SDL_RWops *src)
{
    Sint64 start;
    int w, h, format;
    stbi_uc *pixels;
    stbi_io_callbacks rw_callbacks;
    SDL_Surface *surface = NULL;

    if ( !src ) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }
    start = SDL_RWtell(src);

    /* Load the image data */
    rw_callbacks.read = IMG_LoadSTB_RW_read;
    rw_callbacks.skip = IMG_LoadSTB_RW_skip;
    rw_callbacks.eof = IMG_LoadSTB_RW_eof;
    pixels = stbi_load_from_callbacks(
        &rw_callbacks,
        src,
        &w,
        &h,
        &format,
        STBI_default
    );
    if ( !pixels ) {
        SDL_RWseek(src, start, RW_SEEK_SET);
        return NULL;
    }

    if (format == STBI_grey || format == STBI_rgb || format == STBI_rgb_alpha) {
        surface = SDL_CreateRGBSurfaceWithFormatFrom(
            pixels,
            w,
            h,
            8 * format,
            w * format,
            (format == STBI_rgb_alpha) ? SDL_PIXELFORMAT_RGBA32 :
            (format == STBI_rgb) ? SDL_PIXELFORMAT_RGB24 :
            SDL_PIXELFORMAT_INDEX8
        );
        if (surface) {
            /* Set a grayscale palette for gray images */
            SDL_Palette *palette = surface->format->palette;
            if (palette) {
                int i;

                for (i = 0; i < palette->ncolors; i++) {
                    palette->colors[i].r = (Uint8)i;
                    palette->colors[i].g = (Uint8)i;
                    palette->colors[i].b = (Uint8)i;
                }
            }

            /* FIXME: This sucks. It'd be better to allocate the surface first, then
             * write directly to the pixel buffer:
             * https://github.com/nothings/stb/issues/58
             * -flibit
             */
            surface->flags &= ~SDL_PREALLOC;
        }

    } else if (format == STBI_grey_alpha) {
        surface = SDL_CreateRGBSurfaceWithFormat(
            0,
            w,
            h,
            32,
            SDL_PIXELFORMAT_RGBA32
        );
        if (surface) {
            Uint8 *src = pixels;
            Uint8 *dst = (Uint8 *)surface->pixels;
            int skip = surface->pitch - (surface->w * 4);
            int row, col;

            for (row = 0; row < h; ++row) {
                for (col = 0; col < w; ++col) {
                    Uint8 c = *src++;
                    Uint8 a = *src++;
                    *dst++ = c;
                    *dst++ = c;
                    *dst++ = c;
                    *dst++ = a;
                }
                dst += skip;
            }
            stbi_image_free(pixels);
        }
    } else {
        IMG_SetError("Unknown image format: %d", format);
    }

    if (!surface) {
        /* The error message should already be set */
        stbi_image_free(pixels); /* calls SDL_free() */
        SDL_RWseek(src, start, RW_SEEK_SET);
    }
    return surface;
}

#endif /* USE_STBIMAGE */
