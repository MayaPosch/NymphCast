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

/* This is a JXL image file loading framework */

#include "SDL_image.h"

#ifdef LOAD_JXL

#include <jxl/decode.h>


static struct {
    int loaded;
    void *handle;
    JxlDecoder* (*JxlDecoderCreate)(const JxlMemoryManager* memory_manager);
    JxlDecoderStatus (*JxlDecoderSubscribeEvents)(JxlDecoder* dec, int events_wanted);
    JxlDecoderStatus (*JxlDecoderSetInput)(JxlDecoder* dec, const uint8_t* data, size_t size);
    JxlDecoderStatus (*JxlDecoderProcessInput)(JxlDecoder* dec);
    JxlDecoderStatus (*JxlDecoderGetBasicInfo)(const JxlDecoder* dec, JxlBasicInfo* info);
    JxlDecoderStatus (*JxlDecoderImageOutBufferSize)(const JxlDecoder* dec, const JxlPixelFormat* format, size_t* size);
    JxlDecoderStatus (*JxlDecoderSetImageOutBuffer)(JxlDecoder* dec, const JxlPixelFormat* format, void* buffer, size_t size);
    void (*JxlDecoderDestroy)(JxlDecoder* dec);
} lib;

#ifdef LOAD_JXL_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    lib.FUNC = (SIG) SDL_LoadFunction(lib.handle, #FUNC); \
    if (lib.FUNC == NULL) { SDL_UnloadObject(lib.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    lib.FUNC = FUNC; \
    if (lib.FUNC == NULL) { IMG_SetError("Missing jxl.framework"); return -1; }
#endif

int IMG_InitJXL()
{
    if ( lib.loaded == 0 ) {
#ifdef LOAD_JXL_DYNAMIC
        lib.handle = SDL_LoadObject(LOAD_JXL_DYNAMIC);
        if ( lib.handle == NULL ) {
            return -1;
        }
#endif
        FUNCTION_LOADER(JxlDecoderCreate, JxlDecoder* (*)(const JxlMemoryManager* memory_manager))
        FUNCTION_LOADER(JxlDecoderSubscribeEvents, JxlDecoderStatus (*)(JxlDecoder* dec, int events_wanted))
        FUNCTION_LOADER(JxlDecoderSetInput, JxlDecoderStatus (*)(JxlDecoder* dec, const uint8_t* data, size_t size))
        FUNCTION_LOADER(JxlDecoderProcessInput, JxlDecoderStatus (*)(JxlDecoder* dec))
        FUNCTION_LOADER(JxlDecoderGetBasicInfo, JxlDecoderStatus (*)(const JxlDecoder* dec, JxlBasicInfo* info))
        FUNCTION_LOADER(JxlDecoderImageOutBufferSize, JxlDecoderStatus (*)(const JxlDecoder* dec, const JxlPixelFormat* format, size_t* size))
        FUNCTION_LOADER(JxlDecoderSetImageOutBuffer, JxlDecoderStatus (*)(JxlDecoder* dec, const JxlPixelFormat* format, void* buffer, size_t size))
        FUNCTION_LOADER(JxlDecoderDestroy, void (*)(JxlDecoder* dec))
    }
    ++lib.loaded;

    return 0;
}
void IMG_QuitJXL()
{
    if ( lib.loaded == 0 ) {
        return;
    }
    if ( lib.loaded == 1 ) {
#ifdef LOAD_JXL_DYNAMIC
        SDL_UnloadObject(lib.handle);
#endif
    }
    --lib.loaded;
}

/* See if an image is contained in a data source */
int IMG_isJXL(SDL_RWops *src)
{
    Sint64 start;
    int is_JXL;
    Uint8 magic[12];

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_JXL = 0;
    if ( SDL_RWread(src, magic, 2, 1) ) {
        if ( magic[0] == 0xFF && magic[1] == 0x0A ) {
            /* This is a JXL codestream */
            is_JXL = 1;
        } else {
            if ( SDL_RWread(src, &magic[2], sizeof(magic) - 2, 1) ) {
                if ( magic[0] == 0x00 && magic[1] == 0x00 &&
                     magic[2] == 0x00 && magic[3] == 0x0C &&
                     magic[4] == 'J' && magic[5] == 'X' &&
                     magic[6] == 'L' && magic[7] == ' ' &&
                     magic[8] == 0x0D && magic[9] == 0x0A &&
                     magic[10] == 0x87 && magic[11] == 0x0A ) {
                    /* This is a JXL container */
                    is_JXL = 1;
                }
            }
        }
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_JXL);
}

/* Load a JXL type image from an SDL datasource */
SDL_Surface *IMG_LoadJXL_RW(SDL_RWops *src)
{
    Sint64 start;
    unsigned char *data;
    size_t datasize;
    JxlDecoder *decoder = NULL;
    JxlBasicInfo info;
    JxlPixelFormat format = { 4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0 };
    size_t outputsize;
    void *pixels = NULL;
    int pitch = 0;
    SDL_Surface *surface = NULL;

    if (!src) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }
    start = SDL_RWtell(src);

    if ((IMG_Init(IMG_INIT_JXL) & IMG_INIT_JXL) == 0) {
        return NULL;
    }

    data = (unsigned char *)SDL_LoadFile_RW(src, &datasize, SDL_FALSE);
    if (!data) {
        return NULL;
    }

    decoder = lib.JxlDecoderCreate(NULL);
    if (!decoder) {
        IMG_SetError("Couldn't create JXL decoder");
        goto done;
    }

    if (lib.JxlDecoderSubscribeEvents(decoder, JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS) {
        IMG_SetError("Couldn't subscribe to JXL events");
        goto done;
    }

    if (lib.JxlDecoderSetInput(decoder, data, datasize) != JXL_DEC_SUCCESS) {
        IMG_SetError("Couldn't set JXL input");
        goto done;
    }

    SDL_zero(info);

    for ( ; ; ) {
        JxlDecoderStatus status = lib.JxlDecoderProcessInput(decoder);

        switch (status ) {
        case JXL_DEC_ERROR:
            IMG_SetError("JXL decoder error");
            goto done;
        case JXL_DEC_NEED_MORE_INPUT:
            IMG_SetError("Incomplete JXL image");
            goto done;
        case JXL_DEC_BASIC_INFO:
            if (lib.JxlDecoderGetBasicInfo(decoder, &info) != JXL_DEC_SUCCESS) {
                IMG_SetError("Couldn't get JXL image info");
                goto done;
            }
            break;
        case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
            if (lib.JxlDecoderImageOutBufferSize(decoder, &format, &outputsize) != JXL_DEC_SUCCESS) {
                IMG_SetError("Couldn't get JXL image size");
                goto done;
            }
            if (info.xsize == 0 || info.ysize == 0) {
                IMG_SetError("Couldn't get pixels for %dx%d JXL image", info.xsize, info.ysize);
                goto done;
            }
            if (pixels) {
                SDL_free(pixels);
            }
            pixels = SDL_malloc(outputsize);
            if (!pixels) {
                SDL_OutOfMemory();
                goto done;
            }
            if ((outputsize / info.ysize) > SDL_MAX_SINT32) {
                SDL_OutOfMemory();
                goto done;
            }
            pitch = (int)(outputsize / info.ysize);
            if (lib.JxlDecoderSetImageOutBuffer(decoder, &format, pixels, outputsize) != JXL_DEC_SUCCESS) {
                IMG_SetError("Couldn't set JXL output buffer");
                goto done;
            }
            break;
        case JXL_DEC_FULL_IMAGE:
            /* We have a full image - in the case of an animation, keep decoding until the last frame */
            break;
        case JXL_DEC_SUCCESS:
            /* All done! */
            surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels, info.xsize, info.ysize, 0, pitch, SDL_PIXELFORMAT_RGBA32);
            if (surface) {
                /* Let SDL manage the memory now */
                pixels = NULL;
                surface->flags &= ~SDL_PREALLOC;
            }
            goto done;
        default:
            IMG_SetError("Unknown JXL decoding status: %d", status);
            goto done;
        }
    }

done:
    if (decoder) {
        lib.JxlDecoderDestroy(decoder);
    }
    if (data) {
        SDL_free(data);
    }
    if (pixels) {
        SDL_free(pixels);
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

int IMG_InitJXL()
{
    IMG_SetError("JXL images are not supported");
    return(-1);
}

void IMG_QuitJXL()
{
}

/* See if an image is contained in a data source */
int IMG_isJXL(SDL_RWops *src)
{
    return(0);
}

/* Load a JXL type image from an SDL datasource */
SDL_Surface *IMG_LoadJXL_RW(SDL_RWops *src)
{
    return(NULL);
}

#endif /* LOAD_JXL */
