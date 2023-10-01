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

#if (!defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND)) || !defined(BMP_USES_IMAGEIO)

/* This is a BMP image file loading framework
 *
 * ICO/CUR file support is here as well since it uses similar internal
 * representation
 *
 * A good test suite of BMP images is available at:
 * http://entropymine.com/jason/bmpsuite/bmpsuite/html/bmpsuite.html
 */

#include "SDL_image.h"

#ifdef LOAD_BMP

/* See if an image is contained in a data source */
int IMG_isBMP(SDL_RWops *src)
{
    Sint64 start;
    int is_BMP;
    char magic[2];

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_BMP = 0;
    if ( SDL_RWread(src, magic, sizeof(magic), 1) ) {
        if ( SDL_strncmp(magic, "BM", 2) == 0 ) {
            is_BMP = 1;
        }
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_BMP);
}

static int IMG_isICOCUR(SDL_RWops *src, int type)
{
    Sint64 start;
    int is_ICOCUR;

    /* The Win32 ICO file header (14 bytes) */
    Uint16 bfReserved;
    Uint16 bfType;
    Uint16 bfCount;

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_ICOCUR = 0;
    bfReserved = SDL_ReadLE16(src);
    bfType = SDL_ReadLE16(src);
    bfCount = SDL_ReadLE16(src);
    if ((bfReserved == 0) && (bfType == type) && (bfCount != 0))
        is_ICOCUR = 1;
    SDL_RWseek(src, start, RW_SEEK_SET);

    return (is_ICOCUR);
}

int IMG_isICO(SDL_RWops *src)
{
    return IMG_isICOCUR(src, 1);
}

int IMG_isCUR(SDL_RWops *src)
{
    return IMG_isICOCUR(src, 2);
}

#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_endian.h"

/* Compression encodings for BMP files */
#ifndef BI_RGB
#define BI_RGB      0
#define BI_RLE8     1
#define BI_RLE4     2
#define BI_BITFIELDS    3
#endif

static SDL_Surface *LoadBMP_RW (SDL_RWops *src, int freesrc)
{
    return SDL_LoadBMP_RW(src, freesrc);
}

static Uint8
SDL_Read8(SDL_RWops * src)
{
    Uint8 value;

    SDL_RWread(src, &value, 1, 1);
    return (value);
}

static SDL_Surface *
LoadICOCUR_RW(SDL_RWops * src, int type, int freesrc)
{
    SDL_bool was_error;
    Sint64 fp_offset = 0;
    int bmpPitch;
    int i,j, pad;
    SDL_Surface *surface;
    /*
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;
    */
    Uint8 *bits;
    int ExpandBMP;
    int maxCol = 0;
    int icoOfs = 0;
    Uint32 palette[256];

    /* The Win32 ICO file header (14 bytes) */
    Uint16 bfReserved;
    Uint16 bfType;
    Uint16 bfCount;

    /* The Win32 BITMAPINFOHEADER struct (40 bytes) */
    Uint32 biSize;
    Sint32 biWidth;
    Sint32 biHeight;
    /* Uint16 biPlanes; */
    Uint16 biBitCount;
    Uint32 biCompression;
    /*
    Uint32 biSizeImage;
    Sint32 biXPelsPerMeter;
    Sint32 biYPelsPerMeter;
    Uint32 biClrImportant;
    */
    Uint32 biClrUsed;

    /* Make sure we are passed a valid data source */
    surface = NULL;
    was_error = SDL_FALSE;
    if (src == NULL) {
        was_error = SDL_TRUE;
        goto done;
    }

    /* Read in the ICO file header */
    fp_offset = SDL_RWtell(src);
    SDL_ClearError();

    bfReserved = SDL_ReadLE16(src);
    bfType = SDL_ReadLE16(src);
    bfCount = SDL_ReadLE16(src);
    if ((bfReserved != 0) || (bfType != type) || (bfCount == 0)) {
        IMG_SetError("File is not a Windows %s file", type == 1 ? "ICO" : "CUR");
        was_error = SDL_TRUE;
        goto done;
    }

    /* Read the Win32 Icon Directory */
    for (i = 0; i < bfCount; i++) {
        /* Icon Directory Entries */
        int bWidth = SDL_Read8(src);    /* Uint8, but 0 = 256 ! */
        int bHeight = SDL_Read8(src);   /* Uint8, but 0 = 256 ! */
        int bColorCount = SDL_Read8(src);       /* Uint8, but 0 = 256 ! */
        /*
        Uint8 bReserved;
        Uint16 wPlanes;
        Uint16 wBitCount;
        Uint32 dwBytesInRes;
        */
        Uint32 dwImageOffset;

        /* bReserved = */ SDL_Read8(src);
        /* wPlanes = */ SDL_ReadLE16(src);
        /* wBitCount = */ SDL_ReadLE16(src);
        /* dwBytesInRes = */ SDL_ReadLE32(src);
        dwImageOffset = SDL_ReadLE32(src);

        if (!bWidth)
            bWidth = 256;
        if (!bHeight)
            bHeight = 256;
        if (!bColorCount)
            bColorCount = 256;

        //printf("%dx%d@%d - %08x\n", bWidth, bHeight, bColorCount, dwImageOffset);
        if (bColorCount > maxCol) {
            maxCol = bColorCount;
            icoOfs = dwImageOffset;
            //printf("marked\n");
        }
    }

    /* Advance to the DIB Data */
    if (SDL_RWseek(src, icoOfs, RW_SEEK_SET) < 0) {
        SDL_Error(SDL_EFSEEK);
        was_error = SDL_TRUE;
        goto done;
    }

    /* Read the Win32 BITMAPINFOHEADER */
    biSize = SDL_ReadLE32(src);
    if (biSize == 40) {
        biWidth = SDL_ReadLE32(src);
        biHeight = SDL_ReadLE32(src);
        /* biPlanes = */ SDL_ReadLE16(src);
        biBitCount = SDL_ReadLE16(src);
        biCompression = SDL_ReadLE32(src);
        /* biSizeImage = */ SDL_ReadLE32(src);
        /* biXPelsPerMeter = */ SDL_ReadLE32(src);
        /* biYPelsPerMeter = */ SDL_ReadLE32(src);
        biClrUsed = SDL_ReadLE32(src);
        /* biClrImportant = */ SDL_ReadLE32(src);
    } else {
        IMG_SetError("Unsupported ICO bitmap format");
        was_error = SDL_TRUE;
        goto done;
    }

    /* Check for read error */
    if (SDL_strcmp(SDL_GetError(), "") != 0) {
        was_error = SDL_TRUE;
        goto done;
    }

    /* We don't support any BMP compression right now */
    switch (biCompression) {
    case BI_RGB:
        /* Default values for the BMP format */
        switch (biBitCount) {
        case 1:
        case 4:
            ExpandBMP = biBitCount;
            break;
        case 8:
            ExpandBMP = 8;
            break;
        case 24:
            ExpandBMP = 24;
            break;
        case 32:
            /*
            Rmask = 0x00FF0000;
            Gmask = 0x0000FF00;
            Bmask = 0x000000FF;
            */
            ExpandBMP = 0;
            break;
        default:
            IMG_SetError("ICO file with unsupported bit count");
            was_error = SDL_TRUE;
            goto done;
        }
        break;
    default:
        IMG_SetError("Compressed ICO files not supported");
        was_error = SDL_TRUE;
        goto done;
    }

    /* sanity check image size, so we don't overflow integers, etc. */
    if ((biWidth < 0) || (biWidth > 0xFFFFFF) ||
        (biHeight < 0) || (biHeight > 0xFFFFFF)) {
        IMG_SetError("Unsupported or invalid ICO dimensions");
        was_error = SDL_TRUE;
        goto done;
    }

    /* Create a RGBA surface */
    biHeight = biHeight >> 1;
    //printf("%d x %d\n", biWidth, biHeight);
    surface =
        SDL_CreateRGBSurface(0, biWidth, biHeight, 32, 0x00FF0000,
                             0x0000FF00, 0x000000FF, 0xFF000000);
    if (surface == NULL) {
        was_error = SDL_TRUE;
        goto done;
    }

    /* Load the palette, if any */
    //printf("bc %d bused %d\n", biBitCount, biClrUsed);
    if (biBitCount <= 8) {
        if (biClrUsed == 0) {
            biClrUsed = 1 << biBitCount;
        }
        if (biClrUsed > SDL_arraysize(palette)) {
            IMG_SetError("Unsupported or incorrect biClrUsed field");
            was_error = SDL_TRUE;
            goto done;
        }
        for (i = 0; i < (int) biClrUsed; ++i) {
            SDL_RWread(src, &palette[i], 4, 1);
        }
    }

    /* Read the surface pixels.  Note that the bmp image is upside down */
    bits = (Uint8 *) surface->pixels + (surface->h * surface->pitch);
    switch (ExpandBMP) {
    case 1:
        bmpPitch = (biWidth + 7) >> 3;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        break;
    case 4:
        bmpPitch = (biWidth + 1) >> 1;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        break;
    case 8:
        bmpPitch = biWidth;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        break;
    case 24:
        bmpPitch = biWidth * 3;
        pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
        break;
    default:
        bmpPitch = biWidth * 4;
        pad = 0;
        break;
    }
    while (bits > (Uint8 *) surface->pixels) {
        bits -= surface->pitch;
        switch (ExpandBMP) {
        case 1:
        case 4:
        case 8:
            {
                Uint8 pixel = 0;
                int shift = (8 - ExpandBMP);
                for (i = 0; i < surface->w; ++i) {
                    if (i % (8 / ExpandBMP) == 0) {
                        if (!SDL_RWread(src, &pixel, 1, 1)) {
                            IMG_SetError("Error reading from ICO");
                            was_error = SDL_TRUE;
                            goto done;
                        }
                    }
                    *((Uint32 *) bits + i) = (palette[pixel >> shift]);
                    pixel <<= ExpandBMP;
                }
            }
            break;
        case 24:
            {
                Uint32 pixel;
                Uint8 channel;
                for (i = 0; i < surface->w; ++i) {
                    pixel = 0;
                    for (j = 0; j < 3; ++j) {
                        /* Load each color channel into pixel */
                        if (!SDL_RWread(src, &channel, 1, 1)) {
                            IMG_SetError("Error reading from ICO");
                            was_error = SDL_TRUE;
                            goto done;
                        }
                        pixel |= (channel << (j * 8));
                    }
                    *((Uint32 *) bits + i) = pixel;
                }
            }
            break;

        default:
            if (SDL_RWread(src, bits, 1, surface->pitch)
                != surface->pitch) {
                SDL_Error(SDL_EFREAD);
                was_error = SDL_TRUE;
                goto done;
            }
            break;
        }
        /* Skip padding bytes, ugh */
        if (pad) {
            Uint8 padbyte;
            for (i = 0; i < pad; ++i) {
                SDL_RWread(src, &padbyte, 1, 1);
            }
        }
    }
    /* Read the mask pixels.  Note that the bmp image is upside down */
    bits = (Uint8 *) surface->pixels + (surface->h * surface->pitch);
    ExpandBMP = 1;
    bmpPitch = (biWidth + 7) >> 3;
    pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
    while (bits > (Uint8 *) surface->pixels) {
        Uint8 pixel = 0;
        int shift = (8 - ExpandBMP);

        bits -= surface->pitch;
        for (i = 0; i < surface->w; ++i) {
            if (i % (8 / ExpandBMP) == 0) {
                if (!SDL_RWread(src, &pixel, 1, 1)) {
                    IMG_SetError("Error reading from ICO");
                    was_error = SDL_TRUE;
                    goto done;
                }
            }
            *((Uint32 *) bits + i) |= ((pixel >> shift) ? 0 : 0xFF000000);
            pixel <<= ExpandBMP;
        }
        /* Skip padding bytes, ugh */
        if (pad) {
            Uint8 padbyte;
            for (i = 0; i < pad; ++i) {
                SDL_RWread(src, &padbyte, 1, 1);
            }
        }
    }
  done:
    if (was_error) {
        if (src) {
            SDL_RWseek(src, fp_offset, RW_SEEK_SET);
        }
        if (surface) {
            SDL_FreeSurface(surface);
        }
        surface = NULL;
    }
    if (freesrc && src) {
        SDL_RWclose(src);
    }
    return (surface);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_RW(SDL_RWops *src)
{
    return(LoadBMP_RW(src, 0));
}

/* Load a ICO type image from an SDL datasource */
SDL_Surface *IMG_LoadICO_RW(SDL_RWops *src)
{
    return(LoadICOCUR_RW(src, 1, 0));
}

/* Load a CUR type image from an SDL datasource */
SDL_Surface *IMG_LoadCUR_RW(SDL_RWops *src)
{
    return(LoadICOCUR_RW(src, 2, 0));
}

#else

#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

/* See if an image is contained in a data source */
int IMG_isBMP(SDL_RWops *src)
{
    return(0);
}

int IMG_isICO(SDL_RWops *src)
{
    return(0);
}

int IMG_isCUR(SDL_RWops *src)
{
    return(0);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadBMP_RW(SDL_RWops *src)
{
    return(NULL);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadCUR_RW(SDL_RWops *src)
{
    return(NULL);
}

/* Load a BMP type image from an SDL datasource */
SDL_Surface *IMG_LoadICO_RW(SDL_RWops *src)
{
    return(NULL);
}

#endif /* LOAD_BMP */

#endif /* !defined(__APPLE__) || defined(SDL_IMAGE_USE_COMMON_BACKEND) */
