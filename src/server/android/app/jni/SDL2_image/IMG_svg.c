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

/* This is an SVG image file loading framework, based on Nano SVG:
 * https://github.com/memononen/nanosvg
 */

#include "SDL_image.h"

#ifdef LOAD_SVG

#if !SDL_VERSION_ATLEAST(2, 0, 16)
/* SDL_roundf() is available starting with 2.0.16 */
static float SDLCALL SDL_roundf(float x)
{
    return (x >= 0.0f) ? SDL_floorf(x + 0.5f) : SDL_ceilf(x - 0.5f);
}
#endif /* SDL 2.0.16 */

/* Replace C runtime functions with SDL C runtime functions for building on Windows */
#define free    SDL_free
#define malloc  SDL_malloc
#undef memcpy
#define memcpy  SDL_memcpy
#undef memset
#define memset  SDL_memset
#define qsort   SDL_qsort
#define realloc SDL_realloc
#define sscanf  SDL_sscanf
#undef strchr
#define strchr  SDL_strchr
#undef strcmp
#define strcmp  SDL_strcmp
#undef strncmp
#define strncmp SDL_strncmp
#undef strncpy
#define strncpy SDL_strlcpy
#define strlen  SDL_strlen
#define strstr  SDL_strstr
#define strtol  SDL_strtol
#define strtoll SDL_strtoll

#define acosf   SDL_acosf
#define atan2f  SDL_atan2f
#define cosf    SDL_cosf
#define ceilf   SDL_ceilf
#define fabs    SDL_fabs
#define fabsf   SDL_fabsf
#define floorf  SDL_floorf
#define fmodf   SDL_fmodf
#define pow     SDL_pow
#define sinf    SDL_sinf
#define sqrt    SDL_sqrt
#define sqrtf   SDL_sqrtf
#define tanf    SDL_tanf
#define roundf  SDL_roundf
#ifndef FLT_MAX
#define FLT_MAX     3.402823466e+38F
#endif
#undef HAVE_STDIO_H

#define NSVG_EXPORT static
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

/* See if an image is contained in a data source */
int IMG_isSVG(SDL_RWops *src)
{
    Sint64 start;
    int is_SVG;
    char magic[4096];
    size_t magic_len;

    if (!src)
        return 0;
    start = SDL_RWtell(src);
    is_SVG = 0;
    magic_len = SDL_RWread(src, magic, 1, sizeof(magic) - 1);
    magic[magic_len] = '\0';
    if (SDL_strstr(magic, "<svg")) {
        is_SVG = 1;
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_SVG);
}

/* Load a SVG type image from an SDL datasource */
SDL_Surface *IMG_LoadSizedSVG_RW(SDL_RWops *src, int width, int height)
{
    char *data;
    struct NSVGimage *image;
    struct NSVGrasterizer *rasterizer;
    SDL_Surface *surface = NULL;
    float scale = 1.0f;

    data = (char *)SDL_LoadFile_RW(src, NULL, SDL_FALSE);
    if (!data) {
        return NULL;
    }

    /* For now just use default units of pixels at 96 DPI */
    image = nsvgParse(data, "px", 96.0f);
    SDL_free(data);
    if (!image || image->width <= 0.0f || image->height <= 0.0f) {
        IMG_SetError("Couldn't parse SVG image");
        return NULL;
    }

    rasterizer = nsvgCreateRasterizer();
    if (!rasterizer) {
        IMG_SetError("Couldn't create SVG rasterizer");
        nsvgDelete(image);
        return NULL;
    }

    if (width > 0 && height > 0) {
        float scale_x = (float)width / image->width;
        float scale_y = (float)height / image->height;

        scale = SDL_min(scale_x, scale_y);
    } else if (width > 0) {
        scale = (float)width / image->width;
    } else if (height > 0) {
        scale = (float)height / image->height;
    } else {
        scale = 1.0f;
    }

    surface = SDL_CreateRGBSurfaceWithFormat(0,
                                             (int)SDL_ceilf(image->width * scale),
                                             (int)SDL_ceilf(image->height * scale),
                                             32,
                                             SDL_PIXELFORMAT_RGBA32);

    if (!surface) {
        nsvgDeleteRasterizer(rasterizer);
        nsvgDelete(image);
        return NULL;
    }

    nsvgRasterize(rasterizer, image, 0.0f, 0.0f, scale, (unsigned char *)surface->pixels, surface->w, surface->h, surface->pitch);
    nsvgDeleteRasterizer(rasterizer);
    nsvgDelete(image);

    return surface;
}

#else
#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

/* See if an image is contained in a data source */
int IMG_isSVG(SDL_RWops *src)
{
    return(0);
}

/* Load a SVG type image from an SDL datasource */
SDL_Surface *IMG_LoadSizedSVG_RW(SDL_RWops *src, int width, int height)
{
    return(NULL);
}

#endif /* LOAD_SVG */

/* Load a SVG type image from an SDL datasource */
SDL_Surface *IMG_LoadSVG_RW(SDL_RWops *src)
{
    return IMG_LoadSizedSVG_RW(src, 0, 0);
}

