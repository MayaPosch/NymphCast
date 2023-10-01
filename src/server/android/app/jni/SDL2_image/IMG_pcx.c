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

/*
 * PCX file reader:
 * Supports:
 *  1..4 bits/pixel in multiplanar format (1 bit/plane/pixel)
 *  8 bits/pixel in single-planar format (8 bits/plane/pixel)
 *  24 bits/pixel in 3-plane format (8 bits/plane/pixel)
 *
 * (The <8bpp formats are expanded to 8bpp surfaces)
 *
 * Doesn't support:
 *  single-planar packed-pixel formats other than 8bpp
 *  4-plane 32bpp format with a fourth "intensity" plane
 */

#include "SDL_endian.h"

#include "SDL_image.h"

#ifdef LOAD_PCX

struct PCXheader {
    Uint8 Manufacturer;
    Uint8 Version;
    Uint8 Encoding;
    Uint8 BitsPerPixel;
    Sint16 Xmin, Ymin, Xmax, Ymax;
    Sint16 HDpi, VDpi;
    Uint8 Colormap[48];
    Uint8 Reserved;
    Uint8 NPlanes;
    Sint16 BytesPerLine;
    Sint16 PaletteInfo;
    Sint16 HscreenSize;
    Sint16 VscreenSize;
    Uint8 Filler[54];
};

/* See if an image is contained in a data source */
int IMG_isPCX(SDL_RWops *src)
{
    Sint64 start;
    int is_PCX;
    const int ZSoft_Manufacturer = 10;
    const int PC_Paintbrush_Version = 5;
    const int PCX_Uncompressed_Encoding = 0;
    const int PCX_RunLength_Encoding = 1;
    struct PCXheader pcxh;

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_PCX = 0;
    if ( SDL_RWread(src, &pcxh, sizeof(pcxh), 1) == 1 ) {
        if ( (pcxh.Manufacturer == ZSoft_Manufacturer) &&
             (pcxh.Version == PC_Paintbrush_Version) &&
             (pcxh.Encoding == PCX_RunLength_Encoding ||
              pcxh.Encoding == PCX_Uncompressed_Encoding) ) {
            is_PCX = 1;
        }
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_PCX);
}

/* Load a PCX type image from an SDL datasource */
SDL_Surface *IMG_LoadPCX_RW(SDL_RWops *src)
{
    Sint64 start;
    struct PCXheader pcxh;
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;
    Uint32 Amask;
    SDL_Surface *surface = NULL;
    int width, height;
    int y, bpl;
    Uint8 *row, *buf = NULL;
    char *error = NULL;
    int bits, src_bits;
    int count = 0;
    Uint8 ch;

    if ( !src ) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }
    start = SDL_RWtell(src);

    if ( !SDL_RWread(src, &pcxh, sizeof(pcxh), 1) ) {
        error = "file truncated";
        goto done;
    }
    pcxh.Xmin = SDL_SwapLE16(pcxh.Xmin);
    pcxh.Ymin = SDL_SwapLE16(pcxh.Ymin);
    pcxh.Xmax = SDL_SwapLE16(pcxh.Xmax);
    pcxh.Ymax = SDL_SwapLE16(pcxh.Ymax);
    pcxh.BytesPerLine = SDL_SwapLE16(pcxh.BytesPerLine);

#if 0
    printf("Manufacturer = %d\n", pcxh.Manufacturer);
    printf("Version = %d\n", pcxh.Version);
    printf("Encoding = %d\n", pcxh.Encoding);
    printf("BitsPerPixel = %d\n", pcxh.BitsPerPixel);
    printf("Xmin = %d, Ymin = %d, Xmax = %d, Ymax = %d\n", pcxh.Xmin, pcxh.Ymin, pcxh.Xmax, pcxh.Ymax);
    printf("HDpi = %d, VDpi = %d\n", pcxh.HDpi, pcxh.VDpi);
    printf("NPlanes = %d\n", pcxh.NPlanes);
    printf("BytesPerLine = %d\n", pcxh.BytesPerLine);
    printf("PaletteInfo = %d\n", pcxh.PaletteInfo);
    printf("HscreenSize = %d\n", pcxh.HscreenSize);
    printf("VscreenSize = %d\n", pcxh.VscreenSize);
#endif

    /* Create the surface of the appropriate type */
    width = (pcxh.Xmax - pcxh.Xmin) + 1;
    height = (pcxh.Ymax - pcxh.Ymin) + 1;
    Rmask = Gmask = Bmask = Amask = 0;
    src_bits = pcxh.BitsPerPixel * pcxh.NPlanes;
    if((pcxh.BitsPerPixel == 1 && pcxh.NPlanes >= 1 && pcxh.NPlanes <= 4)
       || (pcxh.BitsPerPixel == 8 && pcxh.NPlanes == 1)) {
        bits = 8;
    } else if(pcxh.BitsPerPixel == 8 && pcxh.NPlanes == 3) {
        bits = 24;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        Rmask = 0x000000FF;
        Gmask = 0x0000FF00;
        Bmask = 0x00FF0000;
#else
        Rmask = 0xFF0000;
        Gmask = 0x00FF00;
        Bmask = 0x0000FF;
#endif
    } else {
        error = "unsupported PCX format";
        goto done;
    }
    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
                   bits, Rmask, Gmask, Bmask, Amask);
    if ( surface == NULL ) {
        goto done;
    }

    bpl = pcxh.NPlanes * pcxh.BytesPerLine;
    buf = (Uint8 *)SDL_calloc(bpl, 1);
    if ( !buf ) {
        error = "Out of memory";
        goto done;
    }
    row = (Uint8 *)surface->pixels;
    for ( y=0; y<surface->h; ++y ) {
        /* decode a scan line to a temporary buffer first */
        int i;
        if ( pcxh.Encoding == 0 ) {
            if ( !SDL_RWread(src, buf, bpl, 1) ) {
                error = "file truncated";
                goto done;
            }
        } else {
            for ( i = 0; i < bpl; i++ ) {
                if ( !count ) {
                    if ( !SDL_RWread(src, &ch, 1, 1) ) {
                        error = "file truncated";
                        goto done;
                    }
                    if ( ch < 0xc0 ) {
                        count = 1;
                    } else {
                        count = ch - 0xc0;
                        if( !SDL_RWread(src, &ch, 1, 1) ) {
                            error = "file truncated";
                            goto done;
                        }
                    }
                }
                buf[i] = ch;
                count--;
            }
        }

        if ( src_bits <= 4 ) {
            /* expand planes to 1 byte/pixel */
            Uint8 *innerSrc = buf;
            int plane;
            for ( plane = 0; plane < pcxh.NPlanes; plane++ ) {
                int j, k, x = 0;
                for( j = 0; j < pcxh.BytesPerLine; j++ ) {
                    Uint8 byte = *innerSrc++;
                    for( k = 7; k >= 0; k-- ) {
                        unsigned bit = (byte >> k) & 1;
                        /* skip padding bits */
                        if (j * 8 + k >= width)
                            continue;
                        row[x++] |= bit << plane;
                    }
                }
            }
        } else if ( src_bits == 8 ) {
            /* Copy the row directly */
            SDL_memcpy(row, buf, SDL_min(width, bpl));
        } else if ( src_bits == 24 ) {
            /* de-interlace planes */
            Uint8 *innerSrc = buf;
            Uint8 *end1 = buf+bpl;
            int plane;
            for ( plane = 0; plane < pcxh.NPlanes; plane++ ) {
                int x;
                Uint8 *dst = row + plane;
                Uint8 *end2= row + surface->pitch;
                for ( x = 0; x < width; x++ ) {
                    if ( (innerSrc + x) >= end1 || dst >= end2 ) {
                        error = "decoding out of bounds (corrupt?)";
                        goto done;
                    }
                    *dst = innerSrc[x];
                    dst += pcxh.NPlanes;
                }
                innerSrc += pcxh.BytesPerLine;
            }
        }

        row += surface->pitch;
    }

    if ( bits == 8 ) {
        SDL_Color *colors = surface->format->palette->colors;
        int nc = 1 << src_bits;
        int i;

        surface->format->palette->ncolors = nc;
        if ( src_bits == 8 ) {
            Uint8 ch;
            /* look for a 256-colour palette */
            do {
                if ( !SDL_RWread(src, &ch, 1, 1) ) {
                    /* Couldn't find the palette, try the end of the file */
                    SDL_RWseek(src, -768, RW_SEEK_END);
                    break;
                }
            } while ( ch != 12 );

            for ( i = 0; i < 256; i++ ) {
                SDL_RWread(src, &colors[i].r, 1, 1);
                SDL_RWread(src, &colors[i].g, 1, 1);
                SDL_RWread(src, &colors[i].b, 1, 1);
            }
        } else {
            for ( i = 0; i < nc; i++ ) {
                colors[i].r = pcxh.Colormap[i * 3];
                colors[i].g = pcxh.Colormap[i * 3 + 1];
                colors[i].b = pcxh.Colormap[i * 3 + 2];
            }
        }
    }

done:
    SDL_free(buf);
    if ( error ) {
        SDL_RWseek(src, start, RW_SEEK_SET);
        if ( surface ) {
            SDL_FreeSurface(surface);
            surface = NULL;
        }
        IMG_SetError("%s", error);
    }
    return(surface);
}

#else
#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

/* See if an image is contained in a data source */
int IMG_isPCX(SDL_RWops *src)
{
    return(0);
}

/* Load a PCX type image from an SDL datasource */
SDL_Surface *IMG_LoadPCX_RW(SDL_RWops *src)
{
    return(NULL);
}

#endif /* LOAD_PCX */
