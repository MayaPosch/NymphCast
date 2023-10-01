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

/* This is a AVIF image file loading framework */

#include "SDL_image.h"

#ifdef LOAD_AVIF

#include <avif/avif.h>
#include <limits.h> /* for INT_MAX */


static struct {
    int loaded;
    void *handle;
    avifBool (*avifPeekCompatibleFileType)(const avifROData * input);
    avifDecoder * (*avifDecoderCreate)(void);
    void (*avifDecoderDestroy)(avifDecoder * decoder);
    void (*avifDecoderSetIO)(avifDecoder * decoder, avifIO * io);
    avifResult (*avifDecoderParse)(avifDecoder * decoder);
    avifResult (*avifDecoderNextImage)(avifDecoder * decoder);
    avifResult (*avifImageYUVToRGB)(const avifImage * image, avifRGBImage * rgb);
} lib;

#ifdef LOAD_AVIF_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    lib.FUNC = (SIG) SDL_LoadFunction(lib.handle, #FUNC); \
    if (lib.FUNC == NULL) { SDL_UnloadObject(lib.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    lib.FUNC = FUNC; \
    if (lib.FUNC == NULL) { IMG_SetError("Missing avif.framework"); return -1; }
#endif

int IMG_InitAVIF()
{
    if ( lib.loaded == 0 ) {
#ifdef LOAD_AVIF_DYNAMIC
        lib.handle = SDL_LoadObject(LOAD_AVIF_DYNAMIC);
        if ( lib.handle == NULL ) {
            return -1;
        }
#endif
        FUNCTION_LOADER(avifPeekCompatibleFileType, avifBool (*)(const avifROData * input))
        FUNCTION_LOADER(avifDecoderCreate, avifDecoder *(*)(void))
        FUNCTION_LOADER(avifDecoderDestroy, void (*)(avifDecoder * decoder))
        FUNCTION_LOADER(avifDecoderSetIO, void (*)(avifDecoder * decoder, avifIO * io))
        FUNCTION_LOADER(avifDecoderParse, avifResult (*)(avifDecoder * decoder))
        FUNCTION_LOADER(avifDecoderNextImage, avifResult (*)(avifDecoder * decoder))
        FUNCTION_LOADER(avifImageYUVToRGB, avifResult (*)(const avifImage * image, avifRGBImage * rgb))
    }
    ++lib.loaded;

    return 0;
}
void IMG_QuitAVIF()
{
    if ( lib.loaded == 0 ) {
        return;
    }
    if ( lib.loaded == 1 ) {
#ifdef LOAD_AVIF_DYNAMIC
        SDL_UnloadObject(lib.handle);
#endif
    }
    --lib.loaded;
}

static SDL_bool ReadAVIFHeader(SDL_RWops *src, Uint8 **header_data, size_t *header_size)
{
    Uint8 magic[16];
    Uint64 size;
    size_t read = 0;
    Uint8 *data;

    *header_data = NULL;
    *header_size = 0;

    if (!SDL_RWread(src, magic, 8, 1)) {
        return SDL_FALSE;
    }
    read += 8;

    if (SDL_memcmp(&magic[4], "ftyp", 4) != 0) {
        return SDL_FALSE;
    }

    size = (((size_t)magic[0] << 24) |
            ((size_t)magic[1] << 16) |
            ((size_t)magic[2] << 8) |
            ((size_t)magic[3] << 0));
    if (size == 1) {
        /* 64-bit header size */
        if (!SDL_RWread(src, &magic[8], 8, 1)) {
            return SDL_FALSE;
        }
        read += 8;

        size = (((Uint64)magic[8] << 56) |
                ((Uint64)magic[9] << 48) |
                ((Uint64)magic[10] << 40) |
                ((Uint64)magic[11] << 32) |
                ((Uint64)magic[12] << 24) |
                ((Uint64)magic[13] << 16) |
                ((Uint64)magic[14] << 8) |
                ((Uint64)magic[15] << 0));
    }

    if (size > INT_MAX) {
        return SDL_FALSE;
    }
    if (size <= read) {
        return SDL_FALSE;
    }

    /* Read in the header */
    data = (Uint8 *)SDL_malloc(size);
    if (!data) {
        return SDL_FALSE;
    }
    SDL_memcpy(data, magic, read);

    if (!SDL_RWread(src, &data[read], (size - read), 1)) {
        SDL_free(data);
        return SDL_FALSE;
    }
    *header_data = data;
    *header_size = (size_t)size;
    return SDL_TRUE;
}

/* See if an image is contained in a data source */
int IMG_isAVIF(SDL_RWops *src)
{
    Sint64 start;
    int is_AVIF;
    Uint8 *data;
    size_t size;

    if ( !src )
        return 0;

    start = SDL_RWtell(src);
    is_AVIF = 0;
    if (ReadAVIFHeader(src, &data, &size)) {
        /* This might be AVIF, do more thorough checks */
        if ((IMG_Init(IMG_INIT_AVIF) & IMG_INIT_AVIF) != 0) {
            avifROData header;

            header.data = data;
            header.size = size;
            is_AVIF = lib.avifPeekCompatibleFileType(&header);
        }
        SDL_free(data);
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_AVIF);
}

/* Context for AFIF I/O operations */
typedef struct
{
    SDL_RWops *src;
    Uint64 start;
    uint8_t *data;
    size_t size;
} avifIOContext;

static avifResult ReadAVIFIO(struct avifIO * io, uint32_t readFlags, uint64_t offset, size_t size, avifROData * out)
{
    avifIOContext *context = (avifIOContext *)io->data;

    /* The AVIF reader bounces all over, so always seek to the correct offset */
    if (SDL_RWseek(context->src, context->start + offset, RW_SEEK_SET) < 0) {
        return AVIF_RESULT_IO_ERROR;
    }

    if (size > context->size) {
        uint8_t *data = (uint8_t *)SDL_realloc(context->data, size);
        if (!data) {
            return AVIF_RESULT_IO_ERROR;
        }
        context->data = data;
        context->size = size; 
    }

    out->data = context->data;
    out->size = SDL_RWread(context->src, context->data, 1, size);
    if (out->size == 0) {
        return AVIF_RESULT_IO_ERROR;
    }

    return AVIF_RESULT_OK;
}

static void DestroyAVIFIO(struct avifIO * io)
{
    avifIOContext *context = (avifIOContext *)io->data;

    if (context->data) {
        SDL_free(context->data);
        context->data = NULL;
    }
}

/* Load a AVIF type image from an SDL datasource */
SDL_Surface *IMG_LoadAVIF_RW(SDL_RWops *src)
{
    Sint64 start;
    avifDecoder *decoder = NULL;
    avifIO io;
    avifIOContext context;
    avifRGBImage rgb;
    avifResult result;
    SDL_Surface *surface = NULL;

    if (!src) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }
    start = SDL_RWtell(src);

    if ((IMG_Init(IMG_INIT_AVIF) & IMG_INIT_AVIF) == 0) {
        return NULL;
    }

    SDL_zero(context);
    SDL_zero(io);
    SDL_zero(rgb);

    decoder = lib.avifDecoderCreate();
    if (!decoder) {
        IMG_SetError("Couldn't create AVIF decoder");
        goto done;
    }

    /* Be permissive so we can load as many images as possible */
    decoder->strictFlags = AVIF_STRICT_DISABLED;

    context.src = src;
    context.start = start;
    io.destroy = DestroyAVIFIO;
    io.read = ReadAVIFIO;
    io.data = &context;
    lib.avifDecoderSetIO(decoder, &io);

    result = lib.avifDecoderParse(decoder);
    if (result != AVIF_RESULT_OK) {
        IMG_SetError("Couldn't parse AVIF image: %d", result);
        goto done;
    }

    result = lib.avifDecoderNextImage(decoder);
    if (result != AVIF_RESULT_OK) {
        IMG_SetError("Couldn't get AVIF image: %d", result);
        goto done;
    }

    surface = SDL_CreateRGBSurfaceWithFormat(0, decoder->image->width, decoder->image->height, 0, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) {
        goto done;
    }

    /* Convert the YUV image to RGB */
    rgb.width = surface->w;
    rgb.height = surface->h;
    rgb.depth = 8;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    rgb.format = AVIF_RGB_FORMAT_BGRA;
#else
    rgb.format = AVIF_RGB_FORMAT_ARGB;
#endif
    rgb.pixels = (uint8_t *)surface->pixels;
    rgb.rowBytes = (uint32_t)surface->pitch;
    result = lib.avifImageYUVToRGB(decoder->image, &rgb);
    if (result != AVIF_RESULT_OK) {
        IMG_SetError("Couldn't convert AVIF image to RGB: %d", result);
        SDL_FreeSurface(surface);
        surface = NULL;
        goto done;
    }

done:
    if (decoder) {
        lib.avifDecoderDestroy(decoder);
    }
    if (!surface) {
        SDL_RWseek(src, start, RW_SEEK_SET);
    }
    return surface;
}

#else
#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

int IMG_InitAVIF()
{
    IMG_SetError("AVIF images are not supported");
    return(-1);
}

void IMG_QuitAVIF()
{
}

/* See if an image is contained in a data source */
int IMG_isAVIF(SDL_RWops *src)
{
    return(0);
}

/* Load a AVIF type image from an SDL datasource */
SDL_Surface *IMG_LoadAVIF_RW(SDL_RWops *src)
{
    return(NULL);
}

#endif /* LOAD_AVIF */
