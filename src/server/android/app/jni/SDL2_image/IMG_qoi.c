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

/* This file use QOI library:
 * https://github.com/phoboslab/qoi
 */

#include "SDL_image.h"
#include <limits.h> /* for INT_MAX */

#ifdef LOAD_QOI

/* SDL < 2.0.12 compatibility */
#ifndef SDL_zeroa
#define SDL_zeroa(x) SDL_memset((x), 0, sizeof((x)))
#endif

#define QOI_MALLOC(sz) SDL_malloc(sz)
#define QOI_FREE(p)    SDL_free(p)
#define QOI_ZEROARR(a) SDL_zeroa(a)

#define QOI_NO_STDIO
#define QOI_IMPLEMENTATION
#include "qoi.h"

/* See if an image is contained in a data source */
int IMG_isQOI(SDL_RWops *src)
{
    Sint64 start;
    int is_QOI;
    char magic[4];

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_QOI = 0;
    if ( SDL_RWread(src, magic, sizeof(magic), 1) ) {
        if ( SDL_strncmp(magic, "qoif", 4) == 0 ) {
            is_QOI = 1;
        }
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_QOI);
}

/* Load a QOI type image from an SDL datasource */
SDL_Surface *IMG_LoadQOI_RW(SDL_RWops *src)
{
    void *data;
    size_t size;
    void *pixel_data;
    qoi_desc image_info;
    SDL_Surface *surface = NULL;

    data = (void *)SDL_LoadFile_RW(src, &size, SDL_FALSE);
    if ( !data ) {
        return NULL;
    }
    if ( size > INT_MAX ) {
        SDL_free(data);
        IMG_SetError("QOI image is too big.");
        return NULL;
    }

    pixel_data = qoi_decode(data, (int)size, &image_info, 4);
    /* pixel_data is in R,G,B,A order regardless of endianness */
    SDL_free(data);
    if ( !pixel_data ) {
        IMG_SetError("Couldn't parse QOI image");
        return NULL;
    }

    surface = SDL_CreateRGBSurfaceWithFormatFrom(pixel_data,
                                                 image_info.width,
                                                 image_info.height,
                                                 32,
					         (image_info.width * 4),
                                                 SDL_PIXELFORMAT_RGBA32);
    if ( !surface ) {
        SDL_free(pixel_data);
        IMG_SetError("Couldn't create SDL_Surface");
        return NULL;
    }

    /* Let SDL manage the memory now */
    surface->flags &= ~SDL_PREALLOC;

    return surface;
}

#else
#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

/* See if an image is contained in a data source */
int IMG_isQOI(SDL_RWops *src)
{
    return(0);
}

/* Load a QOI type image from an SDL datasource */
SDL_Surface *IMG_LoadQOI_RW(SDL_RWops *src)
{
    return(NULL);
}

#endif /* LOAD_QOI */
