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

/* A simple library to load images of various formats as SDL surfaces */

#include "SDL_image.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#if defined(SDL_BUILD_MAJOR_VERSION) && defined(SDL_COMPILE_TIME_ASSERT)
SDL_COMPILE_TIME_ASSERT(SDL_BUILD_MAJOR_VERSION,
                        SDL_IMAGE_MAJOR_VERSION == SDL_BUILD_MAJOR_VERSION);
SDL_COMPILE_TIME_ASSERT(SDL_BUILD_MINOR_VERSION,
                        SDL_IMAGE_MINOR_VERSION == SDL_BUILD_MINOR_VERSION);
SDL_COMPILE_TIME_ASSERT(SDL_BUILD_MICRO_VERSION,
                        SDL_IMAGE_PATCHLEVEL == SDL_BUILD_MICRO_VERSION);
#endif

#if defined(SDL_COMPILE_TIME_ASSERT)
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MAJOR_VERSION_min, SDL_IMAGE_MAJOR_VERSION >= 0);
/* Limited only by the need to fit in SDL_version */
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MAJOR_VERSION_max, SDL_IMAGE_MAJOR_VERSION <= 255);

SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MINOR_VERSION_min, SDL_IMAGE_MINOR_VERSION >= 0);
/* Limited only by the need to fit in SDL_version */
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_MINOR_VERSION_max, SDL_IMAGE_MINOR_VERSION <= 255);

SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_PATCHLEVEL_min, SDL_IMAGE_PATCHLEVEL >= 0);
/* Limited by its encoding in SDL_VERSIONNUM and in the ABI versions */
SDL_COMPILE_TIME_ASSERT(SDL_IMAGE_PATCHLEVEL_max, SDL_IMAGE_PATCHLEVEL <= 99);
#endif

/* Table of image detection and loading functions */
static struct {
    const char *type;
    int (SDLCALL *is)(SDL_RWops *src);
    SDL_Surface *(SDLCALL *load)(SDL_RWops *src);
} supported[] = {
    /* keep magicless formats first */
    { "TGA", NULL,      IMG_LoadTGA_RW },
    { "AVIF",IMG_isAVIF,IMG_LoadAVIF_RW },
    { "CUR", IMG_isCUR, IMG_LoadCUR_RW },
    { "ICO", IMG_isICO, IMG_LoadICO_RW },
    { "BMP", IMG_isBMP, IMG_LoadBMP_RW },
    { "GIF", IMG_isGIF, IMG_LoadGIF_RW },
    { "JPG", IMG_isJPG, IMG_LoadJPG_RW },
    { "JXL", IMG_isJXL, IMG_LoadJXL_RW },
    { "LBM", IMG_isLBM, IMG_LoadLBM_RW },
    { "PCX", IMG_isPCX, IMG_LoadPCX_RW },
    { "PNG", IMG_isPNG, IMG_LoadPNG_RW },
    { "PNM", IMG_isPNM, IMG_LoadPNM_RW }, /* P[BGP]M share code */
    { "SVG", IMG_isSVG, IMG_LoadSVG_RW },
    { "TIF", IMG_isTIF, IMG_LoadTIF_RW },
    { "XCF", IMG_isXCF, IMG_LoadXCF_RW },
    { "XPM", IMG_isXPM, IMG_LoadXPM_RW },
    { "XV",  IMG_isXV,  IMG_LoadXV_RW  },
    { "WEBP", IMG_isWEBP, IMG_LoadWEBP_RW },
    { "QOI", IMG_isQOI, IMG_LoadQOI_RW },
};

/* Table of animation detection and loading functions */
static struct {
    const char *type;
    int (SDLCALL *is)(SDL_RWops *src);
    IMG_Animation *(SDLCALL *load)(SDL_RWops *src);
} supported_anims[] = {
    /* keep magicless formats first */
    { "GIF", IMG_isGIF, IMG_LoadGIFAnimation_RW },
};

const SDL_version *IMG_Linked_Version(void)
{
    static SDL_version linked_version;
    SDL_IMAGE_VERSION(&linked_version);
    return(&linked_version);
}

extern int IMG_InitAVIF(void);
extern void IMG_QuitAVIF(void);
extern int IMG_InitJPG(void);
extern void IMG_QuitJPG(void);
extern int IMG_InitJXL(void);
extern void IMG_QuitJXL(void);
extern int IMG_InitPNG(void);
extern void IMG_QuitPNG(void);
extern int IMG_InitTIF(void);
extern void IMG_QuitTIF(void);

extern int IMG_InitWEBP(void);
extern void IMG_QuitWEBP(void);

static int initialized = 0;

int IMG_Init(int flags)
{
    int result = 0;

    if (flags & IMG_INIT_AVIF) {
        if ((initialized & IMG_INIT_AVIF) || IMG_InitAVIF() == 0) {
            result |= IMG_INIT_AVIF;
        }
    }
    if (flags & IMG_INIT_JPG) {
        if ((initialized & IMG_INIT_JPG) || IMG_InitJPG() == 0) {
            result |= IMG_INIT_JPG;
        }
    }
    if (flags & IMG_INIT_JXL) {
        if ((initialized & IMG_INIT_JXL) || IMG_InitJXL() == 0) {
            result |= IMG_INIT_JXL;
        }
    }
    if (flags & IMG_INIT_PNG) {
        if ((initialized & IMG_INIT_PNG) || IMG_InitPNG() == 0) {
            result |= IMG_INIT_PNG;
        }
    }
    if (flags & IMG_INIT_TIF) {
        if ((initialized & IMG_INIT_TIF) || IMG_InitTIF() == 0) {
            result |= IMG_INIT_TIF;
        }
    }
    if (flags & IMG_INIT_WEBP) {
        if ((initialized & IMG_INIT_WEBP) || IMG_InitWEBP() == 0) {
            result |= IMG_INIT_WEBP;
        }
    }
    initialized |= result;

    return initialized;
}

void IMG_Quit()
{
    if (initialized & IMG_INIT_AVIF) {
        IMG_QuitAVIF();
    }
    if (initialized & IMG_INIT_JPG) {
        IMG_QuitJPG();
    }
    if (initialized & IMG_INIT_JXL) {
        IMG_QuitJXL();
    }
    if (initialized & IMG_INIT_PNG) {
        IMG_QuitPNG();
    }
    if (initialized & IMG_INIT_TIF) {
        IMG_QuitTIF();
    }
    if (initialized & IMG_INIT_WEBP) {
        IMG_QuitWEBP();
    }
    initialized = 0;
}

#if !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND)
/* Load an image from a file */
SDL_Surface *IMG_Load(const char *file)
{
#if __EMSCRIPTEN__
    int w, h;
    char *data;
    SDL_Surface *surf;

    data = emscripten_get_preloaded_image_data(file, &w, &h);
    if (data != NULL) {
        surf = SDL_CreateRGBSurface(0, w, h, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
        if (surf != NULL) {
            memcpy(surf->pixels, data, w * h * 4);
        }
        free(data);
        return surf;
    }
#endif

    SDL_RWops *src = SDL_RWFromFile(file, "rb");
    const char *ext = SDL_strrchr(file, '.');
    if (ext) {
        ext++;
    }
    if (!src) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }
    return IMG_LoadTyped_RW(src, 1, ext);
}
#endif

/* Load an image from an SDL datasource (for compatibility) */
SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc)
{
    return IMG_LoadTyped_RW(src, freesrc, NULL);
}

/* Portable case-insensitive string compare function */
static int IMG_string_equals(const char *str1, const char *str2)
{
    while ( *str1 && *str2 ) {
        if ( SDL_toupper((unsigned char)*str1) !=
             SDL_toupper((unsigned char)*str2) )
            break;
        ++str1;
        ++str2;
    }
    return (!*str1 && !*str2);
}

/* Load an image from an SDL datasource, optionally specifying the type */
SDL_Surface *IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, const char *type)
{
    int i;
    SDL_Surface *image;

    /* Make sure there is something to do.. */
    if ( src == NULL ) {
        IMG_SetError("Passed a NULL data source");
        return(NULL);
    }

    /* See whether or not this data source can handle seeking */
    if ( SDL_RWseek(src, 0, RW_SEEK_CUR) < 0 ) {
        IMG_SetError("Can't seek in this data source");
        if (freesrc)
            SDL_RWclose(src);
        return(NULL);
    }

#ifdef __EMSCRIPTEN__
    /*load through preloadedImages*/

    if ( src->type == SDL_RWOPS_STDFILE ) {
        int w, h, success;
        char *data;
        SDL_Surface *surf;

        data = emscripten_get_preloaded_image_data_from_FILE(src->hidden.stdio.fp, &w, &h);

        if (data)
        {
            surf = SDL_CreateRGBSurface(0, w, h, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
            if (surf != NULL) {
                memcpy(surf->pixels, data, w * h * 4);
            }
            free(data);

            if (freesrc)
                SDL_RWclose(src);

            /* If SDL_CreateRGBSurface returns NULL, it has set the error message for us */
            return surf;
        }
    }
#endif

    /* Detect the type of image being loaded */
    for ( i=0; i < SDL_arraysize(supported); ++i ) {
        if (supported[i].is) {
            if (!supported[i].is(src))
                continue;
        } else {
            /* magicless format */
            if (!type || !IMG_string_equals(type, supported[i].type))
                continue;
        }
#ifdef DEBUG_IMGLIB
        fprintf(stderr, "IMGLIB: Loading image as %s\n",
            supported[i].type);
#endif
        image = supported[i].load(src);
        if (freesrc)
            SDL_RWclose(src);
        return image;
    }

    if ( freesrc ) {
        SDL_RWclose(src);
    }
    IMG_SetError("Unsupported image format");
    return NULL;
}

#if SDL_VERSION_ATLEAST(2,0,0)
SDL_Texture *IMG_LoadTexture(SDL_Renderer *renderer, const char *file)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = IMG_Load(file);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
    return texture;
}

SDL_Texture *IMG_LoadTexture_RW(SDL_Renderer *renderer, SDL_RWops *src, int freesrc)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = IMG_Load_RW(src, freesrc);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
    return texture;
}

SDL_Texture *IMG_LoadTextureTyped_RW(SDL_Renderer *renderer, SDL_RWops *src, int freesrc, const char *type)
{
    SDL_Texture *texture = NULL;
    SDL_Surface *surface = IMG_LoadTyped_RW(src, freesrc, type);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }
    return texture;
}
#endif /* SDL 2.0 */

/* Load an animation from a file */
IMG_Animation *IMG_LoadAnimation(const char *file)
{
    SDL_RWops *src = SDL_RWFromFile(file, "rb");
    const char *ext = SDL_strrchr(file, '.');
    if (ext) {
        ext++;
    }
    if (!src) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }
    return IMG_LoadAnimationTyped_RW(src, 1, ext);
}

/* Load an animation from an SDL datasource (for compatibility) */
IMG_Animation *IMG_LoadAnimation_RW(SDL_RWops *src, int freesrc)
{
    return IMG_LoadAnimationTyped_RW(src, freesrc, NULL);
}

/* Load an animation from an SDL datasource, optionally specifying the type */
IMG_Animation *IMG_LoadAnimationTyped_RW(SDL_RWops *src, int freesrc, const char *type)
{
    int i;
    IMG_Animation *anim;
    SDL_Surface *image;

    /* Make sure there is something to do.. */
    if ( src == NULL ) {
        IMG_SetError("Passed a NULL data source");
        return(NULL);
    }

    /* See whether or not this data source can handle seeking */
    if ( SDL_RWseek(src, 0, RW_SEEK_CUR) < 0 ) {
        IMG_SetError("Can't seek in this data source");
        if (freesrc)
            SDL_RWclose(src);
        return(NULL);
    }

    /* Detect the type of image being loaded */
    for ( i=0; i < SDL_arraysize(supported_anims); ++i ) {
        if (supported_anims[i].is) {
            if (!supported_anims[i].is(src))
                continue;
        } else {
            /* magicless format */
            if (!type || !IMG_string_equals(type, supported_anims[i].type))
                continue;
        }
#ifdef DEBUG_IMGLIB
        fprintf(stderr, "IMGLIB: Loading image as %s\n",
            supported_anims[i].type);
#endif
        anim = supported_anims[i].load(src);
        if (freesrc)
            SDL_RWclose(src);
        return anim;
    }

    /* Create a single frame animation from an image */
    image = IMG_LoadTyped_RW(src, freesrc, type);
    if (image) {
        anim = (IMG_Animation *)SDL_malloc(sizeof(*anim));
        if (anim) {
            anim->w = image->w;
            anim->h = image->h;
            anim->count = 1;

            anim->frames = (SDL_Surface **)SDL_calloc(anim->count, sizeof(*anim->frames));
            anim->delays = (int *)SDL_calloc(anim->count, sizeof(*anim->delays));

            if (anim->frames && anim->delays) {
                anim->frames[0] = image;
                return anim;
            }
            IMG_FreeAnimation(anim);
        }
        SDL_FreeSurface(image);
        SDL_OutOfMemory();
    }
    return NULL;
}

void IMG_FreeAnimation(IMG_Animation *anim)
{
    if (anim) {
        if (anim->frames) {
            int i;
            for (i = 0; i < anim->count; ++i) {
                if (anim->frames[i]) {
                    SDL_FreeSurface(anim->frames[i]);
                }
            }
            SDL_free(anim->frames);
        }
        if (anim->delays) {
            SDL_free(anim->delays);
        }
        SDL_free(anim);
    }
}

