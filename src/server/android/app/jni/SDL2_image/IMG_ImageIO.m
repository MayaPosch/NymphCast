/*
 *  IMG_ImageIO.c
 *  SDL_image
 *
 *  Created by Eric Wing on 1/1/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#if defined(__APPLE__) && !defined(SDL_IMAGE_USE_COMMON_BACKEND)

#include "SDL_image.h"

// Used because CGDataProviderCreate became deprecated in 10.5
#include <AvailabilityMacros.h>
#include <TargetConditionals.h>
#include <Foundation/Foundation.h>

#if (TARGET_OS_IPHONE == 1) || (TARGET_IPHONE_SIMULATOR == 1)
#import <MobileCoreServices/MobileCoreServices.h> // for UTCoreTypes.h
#import <ImageIO/ImageIO.h>
#import <UIKit/UIImage.h>
#else
// For ImageIO framework and also LaunchServices framework (for UTIs)
#include <ApplicationServices/ApplicationServices.h>
#endif

/**************************************************************
 ***** Begin Callback functions for block reading *************
 **************************************************************/

// This callback reads some bytes from an SDL_rwops and copies it
// to a Quartz buffer (supplied by Apple framework).
static size_t MyProviderGetBytesCallback(void* rwops_userdata, void* quartz_buffer, size_t the_count)
{
    return (size_t)SDL_RWread((struct SDL_RWops *)rwops_userdata, quartz_buffer, 1, the_count);
}

// This callback is triggered when the data provider is released
// so you can clean up any resources.
static void MyProviderReleaseInfoCallback(void* rwops_userdata)
{
    // What should I put here?
    // I think the user and SDL_RWops controls closing, so I don't do anything.
}

static void MyProviderRewindCallback(void* rwops_userdata)
{
    SDL_RWseek((struct SDL_RWops *)rwops_userdata, 0, RW_SEEK_SET);
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1050 // CGDataProviderCreateSequential was introduced in 10.5; CGDataProviderCreate is deprecated
off_t MyProviderSkipForwardBytesCallback(void* rwops_userdata, off_t the_count)
{
    off_t start_position = SDL_RWtell((struct SDL_RWops *)rwops_userdata);
    SDL_RWseek((struct SDL_RWops *)rwops_userdata, the_count, RW_SEEK_CUR);
    off_t end_position = SDL_RWtell((struct SDL_RWops *)rwops_userdata);
    return (end_position - start_position);
}
#else // CGDataProviderCreate was deprecated in 10.5
static void MyProviderSkipBytesCallback(void* rwops_userdata, size_t the_count)
{
    SDL_RWseek((struct SDL_RWops *)rwops_userdata, the_count, RW_SEEK_CUR);
}
#endif

/**************************************************************
 ***** End Callback functions for block reading ***************
 **************************************************************/

// This creates a CGImageSourceRef which is a handle to an image that can be used to examine information
// about the image or load the actual image data.
static CGImageSourceRef CreateCGImageSourceFromRWops(SDL_RWops* rw_ops, CFDictionaryRef hints_and_options)
{
    CGImageSourceRef source_ref;

    // Similar to SDL_RWops, Apple has their own callbacks for dealing with data streams.

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1050 // CGDataProviderCreateSequential was introduced in 10.5; CGDataProviderCreate is deprecated
    CGDataProviderSequentialCallbacks provider_callbacks =
    {
        0,
        MyProviderGetBytesCallback,
        MyProviderSkipForwardBytesCallback,
        MyProviderRewindCallback,
        MyProviderReleaseInfoCallback
    };

    CGDataProviderRef data_provider = CGDataProviderCreateSequential(rw_ops, &provider_callbacks);


#else // CGDataProviderCreate was deprecated in 10.5

    CGDataProviderCallbacks provider_callbacks =
    {
        MyProviderGetBytesCallback,
        MyProviderSkipBytesCallback,
        MyProviderRewindCallback,
        MyProviderReleaseInfoCallback
    };

    CGDataProviderRef data_provider = CGDataProviderCreate(rw_ops, &provider_callbacks);
#endif
    // Get the CGImageSourceRef.
    // The dictionary can be NULL or contain hints to help ImageIO figure out the image type.
    source_ref = CGImageSourceCreateWithDataProvider(data_provider, hints_and_options);
    CGDataProviderRelease(data_provider);
    return source_ref;
}

/* Create a CGImageSourceRef from a file. */
/* Remember to CFRelease the created source when done. */
static CGImageSourceRef CreateCGImageSourceFromFile(const char* the_path)
{
    CFURLRef the_url = NULL;
    CGImageSourceRef source_ref = NULL;
    CFStringRef cf_string = NULL;

    /* Create a CFString from a C string */
    cf_string = CFStringCreateWithCString(NULL, the_path, kCFStringEncodingUTF8);
    if (!cf_string) {
        return NULL;
    }

    /* Create a CFURL from a CFString */
    the_url = CFURLCreateWithFileSystemPath(NULL, cf_string, kCFURLPOSIXPathStyle, false);

    /* Don't need the CFString any more (error or not) */
    CFRelease(cf_string);

    if(!the_url)
    {
        return NULL;
    }


    source_ref = CGImageSourceCreateWithURL(the_url, NULL);
    /* Don't need the URL any more (error or not) */
    CFRelease(the_url);

    return source_ref;
}

static CGImageRef CreateCGImageFromCGImageSource(CGImageSourceRef image_source)
{
    CGImageRef image_ref = NULL;

    if(NULL == image_source)
    {
        return NULL;
    }

    // Get the first item in the image source (some image formats may
    // contain multiple items).
    image_ref = CGImageSourceCreateImageAtIndex(image_source, 0, NULL);
    if(NULL == image_ref)
    {
        IMG_SetError("CGImageSourceCreateImageAtIndex() failed");
    }
    return image_ref;
}

static CFDictionaryRef CreateHintDictionary(CFStringRef uti_string_hint)
{
    CFDictionaryRef hint_dictionary = NULL;

    if(uti_string_hint != NULL)
    {
        // Do a bunch of work to setup a CFDictionary containing the jpeg compression properties.
        CFStringRef the_keys[1];
        CFStringRef the_values[1];

        the_keys[0] = kCGImageSourceTypeIdentifierHint;
        the_values[0] = uti_string_hint;

        // kCFTypeDictionaryKeyCallBacks or kCFCopyStringDictionaryKeyCallBacks?
        hint_dictionary = CFDictionaryCreate(NULL, (const void**)&the_keys, (const void**)&the_values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    }
    return hint_dictionary;
}

// Once we have our image, we need to get it into an SDL_Surface
static SDL_Surface* Create_SDL_Surface_From_CGImage_RGB(CGImageRef image_ref)
{
    /* This code is adapted from Apple's Documentation found here:
     * http://developer.apple.com/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/index.html
     * Listing 9-4††Using a Quartz image as a texture source.
     * Unfortunately, this guide doesn't show what to do about
     * non-RGBA image formats so I'm making the rest up.
     * All this code should be scrutinized.
     */

    size_t w = CGImageGetWidth(image_ref);
    size_t h = CGImageGetHeight(image_ref);
    CGRect rect = {{0, 0}, {w, h}};

    CGImageAlphaInfo alpha = CGImageGetAlphaInfo(image_ref);
    //size_t bits_per_pixel = CGImageGetBitsPerPixel(image_ref);
    size_t bits_per_component = 8;

    SDL_Surface* surface;
    Uint32 Amask;
    Uint32 Rmask;
    Uint32 Gmask;
    Uint32 Bmask;

    CGContextRef bitmap_context;
    CGBitmapInfo bitmap_info;
    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();

    if (alpha == kCGImageAlphaNone ||
        alpha == kCGImageAlphaNoneSkipFirst ||
        alpha == kCGImageAlphaNoneSkipLast) {
        bitmap_info = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host; /* XRGB */
        Amask = 0x00000000;
    } else {
        /* kCGImageAlphaFirst isn't supported */
        //bitmap_info = kCGImageAlphaFirst | kCGBitmapByteOrder32Host; /* ARGB */
        bitmap_info = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host; /* ARGB */
        Amask = 0xFF000000;
    }

    Rmask = 0x00FF0000;
    Gmask = 0x0000FF00;
    Bmask = 0x000000FF;

    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, (int)w, (int)h, 32, Rmask, Gmask, Bmask, Amask);
    if (surface)
    {
        // Sets up a context to be drawn to with surface->pixels as the area to be drawn to
        bitmap_context = CGBitmapContextCreate(
                                               surface->pixels,
                                               surface->w,
                                               surface->h,
                                               bits_per_component,
                                               surface->pitch,
                                               color_space,
                                               bitmap_info
                                               );

        // Draws the image into the context's image_data
        CGContextDrawImage(bitmap_context, rect, image_ref);

        CGContextRelease(bitmap_context);

        // FIXME: Reverse the premultiplied alpha
        if ((bitmap_info & kCGBitmapAlphaInfoMask) == kCGImageAlphaPremultipliedFirst) {
            int i, j;
            Uint8 *p = (Uint8 *)surface->pixels;
            for (i = surface->h * surface->pitch/4; i--; ) {
#if __LITTLE_ENDIAN__
                Uint8 A = p[3];
                if (A) {
                    for (j = 0; j < 3; ++j) {
                        p[j] = (p[j] * 255) / A;
                    }
                }
#else
                Uint8 A = p[0];
                if (A) {
                    for (j = 1; j < 4; ++j) {
                        p[j] = (p[j] * 255) / A;
                    }
                }
#endif /* ENDIAN */
                p += 4;
            }
        }
    }

    if (color_space)
    {
        CGColorSpaceRelease(color_space);
    }

    return surface;
}
static SDL_Surface* Create_SDL_Surface_From_CGImage_Index(CGImageRef image_ref)
{
    size_t w = CGImageGetWidth(image_ref);
    size_t h = CGImageGetHeight(image_ref);
    size_t bits_per_pixel = CGImageGetBitsPerPixel(image_ref);
    size_t bytes_per_row = CGImageGetBytesPerRow(image_ref);

    SDL_Surface* surface;
    SDL_Palette* palette;
    CGColorSpaceRef color_space = CGImageGetColorSpace(image_ref);
    CGColorSpaceRef base_color_space = CGColorSpaceGetBaseColorSpace(color_space);
    size_t num_components = CGColorSpaceGetNumberOfComponents(base_color_space);
    size_t num_entries = CGColorSpaceGetColorTableCount(color_space);
    uint8_t *entry, entries[num_components * num_entries];

    /* What do we do if it's not RGB? */
    if (num_components != 3) {
        SDL_SetError("Unknown colorspace components %lu", num_components);
        return NULL;
    }
    if (bits_per_pixel != 8) {
        SDL_SetError("Unknown bits_per_pixel %lu", bits_per_pixel);
        return NULL;
    }

    CGColorSpaceGetColorTable(color_space, entries);
    surface = SDL_CreateRGBSurface(SDL_SWSURFACE, (int)w, (int)h, (int)bits_per_pixel, 0, 0, 0, 0);
    if (surface) {
        uint8_t* pixels = (uint8_t*)surface->pixels;
        CGDataProviderRef provider = CGImageGetDataProvider(image_ref);
        NSData* data = (id)CGDataProviderCopyData(provider);
        [data autorelease];
        const uint8_t* bytes = [data bytes];
        size_t i;

        palette = surface->format->palette;
        for (i = 0, entry = entries; i < num_entries; ++i) {
            palette->colors[i].r = entry[0];
            palette->colors[i].g = entry[1];
            palette->colors[i].b = entry[2];
            entry += num_components;
        }

        for (i = 0; i < h; ++i) {
            SDL_memcpy(pixels, bytes, w);
            pixels += surface->pitch;
            bytes += bytes_per_row;
        }
    }
    return surface;
}
static SDL_Surface* Create_SDL_Surface_From_CGImage(CGImageRef image_ref)
{
    CGColorSpaceRef color_space = CGImageGetColorSpace(image_ref);
    if (CGColorSpaceGetModel(color_space) == kCGColorSpaceModelIndexed) {
        return Create_SDL_Surface_From_CGImage_Index(image_ref);
    } else {
        return Create_SDL_Surface_From_CGImage_RGB(image_ref);
    }
}


#ifdef JPG_USES_IMAGEIO

int IMG_InitJPG()
{
    return 0;
}

void IMG_QuitJPG()
{
}

#endif /* JPG_USES_IMAGEIO */

#ifdef PNG_USES_IMAGEIO

int IMG_InitPNG()
{
    return 0;
}

void IMG_QuitPNG()
{
}

#endif /* PNG_USES_IMAGEIO */

int IMG_InitTIF()
{
    return 0;
}

void IMG_QuitTIF()
{
}

static int Internal_isType (SDL_RWops *rw_ops, CFStringRef uti_string_to_test)
{
    int is_type = 0;

    if (rw_ops == NULL)
        return 0;

    Sint64 start = SDL_RWtell(rw_ops);
    CFDictionaryRef hint_dictionary = CreateHintDictionary(uti_string_to_test);
    CGImageSourceRef image_source = CreateCGImageSourceFromRWops(rw_ops, hint_dictionary);

    if (hint_dictionary != NULL) {
        CFRelease(hint_dictionary);
    }

    if (NULL == image_source) {
        // reset the file pointer
        SDL_RWseek(rw_ops, start, SEEK_SET);
        return 0;
    }

    // This will get the UTI of the container, not the image itself.
    // Under most cases, this won't be a problem.
    // But if a person passes an icon file which contains a bmp,
    // the format will be of the icon file.
    // But I think the main SDL_image codebase has this same problem so I'm not going to worry about it.
    CFStringRef uti_type = CGImageSourceGetType(image_source);
    //  CFShow(uti_type);

    // Unsure if we really want conformance or equality
    is_type = (int)UTTypeConformsTo(uti_string_to_test, uti_type);

    CFRelease(image_source);

    // reset the file pointer
    SDL_RWseek(rw_ops, start, SEEK_SET);
    return is_type;
}

#ifdef BMP_USES_IMAGEIO

int IMG_isCUR(SDL_RWops *src)
{
    /* FIXME: Is this a supported type? */
    return Internal_isType(src, CFSTR("com.microsoft.cur"));
}

int IMG_isICO(SDL_RWops *src)
{
    return Internal_isType(src, kUTTypeICO);
}

int IMG_isBMP(SDL_RWops *src)
{
    return Internal_isType(src, kUTTypeBMP);
}

#endif /* BMP_USES_IMAGEIO */

int IMG_isGIF(SDL_RWops *src)
{
    return Internal_isType(src, kUTTypeGIF);
}

#ifdef JPG_USES_IMAGEIO

// Note: JPEG 2000 is kUTTypeJPEG2000
int IMG_isJPG(SDL_RWops *src)
{
    return Internal_isType(src, kUTTypeJPEG);
}

#endif /* JPG_USES_IMAGEIO */

#ifdef PNG_USES_IMAGEIO

int IMG_isPNG(SDL_RWops *src)
{
    return Internal_isType(src, kUTTypePNG);
}

#endif /* PNG_USES_IMAGEIO */

// This isn't a public API function. Apple seems to be able to identify tga's.
int IMG_isTGA(SDL_RWops *src)
{
    return Internal_isType(src, CFSTR("com.truevision.tga-image"));
}

int IMG_isTIF(SDL_RWops *src)
{
    return Internal_isType(src, kUTTypeTIFF);
}

static SDL_Surface *LoadImageFromRWops (SDL_RWops *rw_ops, CFStringRef uti_string_hint)
{
    CFDictionaryRef hint_dictionary = CreateHintDictionary(uti_string_hint);
    CGImageSourceRef image_source = CreateCGImageSourceFromRWops(rw_ops, hint_dictionary);

    if (hint_dictionary != NULL)
        CFRelease(hint_dictionary);

    if (NULL == image_source)
        return NULL;

    CGImageRef image_ref = CreateCGImageFromCGImageSource(image_source);
    CFRelease(image_source);

    if (NULL == image_ref)
        return NULL;
    SDL_Surface *sdl_surface = Create_SDL_Surface_From_CGImage(image_ref);
    CFRelease(image_ref);

    return sdl_surface;
}

static SDL_Surface* LoadImageFromFile (const char *file)
{
    CGImageSourceRef image_source = NULL;

    image_source = CreateCGImageSourceFromFile(file);

    if(NULL == image_source)
        return NULL;

    CGImageRef image_ref = CreateCGImageFromCGImageSource(image_source);
    CFRelease(image_source);

    if (NULL == image_ref)
        return NULL;
    SDL_Surface *sdl_surface = Create_SDL_Surface_From_CGImage(image_ref);
    CFRelease(image_ref);
    return sdl_surface;
}

#ifdef BMP_USES_IMAGEIO

SDL_Surface* IMG_LoadCUR_RW (SDL_RWops *src)
{
    /* FIXME: Is this a supported type? */
    return LoadImageFromRWops(src, CFSTR("com.microsoft.cur"));
}

SDL_Surface* IMG_LoadICO_RW (SDL_RWops *src)
{
    return LoadImageFromRWops(src, kUTTypeICO);
}

SDL_Surface* IMG_LoadBMP_RW (SDL_RWops *src)
{
    return LoadImageFromRWops(src, kUTTypeBMP);
}

#endif /* BMP_USES_IMAGEIO */

SDL_Surface* IMG_LoadGIF_RW (SDL_RWops *src)
{
    return LoadImageFromRWops (src, kUTTypeGIF);
}

#ifdef JPG_USES_IMAGEIO

SDL_Surface* IMG_LoadJPG_RW (SDL_RWops *src)
{
    return LoadImageFromRWops (src, kUTTypeJPEG);
}

#endif /* JPG_USES_IMAGEIO */

#ifdef PNG_USES_IMAGEIO

SDL_Surface* IMG_LoadPNG_RW (SDL_RWops *src)
{
    return LoadImageFromRWops (src, kUTTypePNG);
}

#endif /* PNG_USES_IMAGEIO */

SDL_Surface* IMG_LoadTGA_RW (SDL_RWops *src)
{
    return LoadImageFromRWops(src, CFSTR("com.truevision.tga-image"));
}

SDL_Surface* IMG_LoadTIF_RW (SDL_RWops *src)
{
    return LoadImageFromRWops(src, kUTTypeTIFF);
}

// Since UIImage doesn't really support streams well, we should optimize for the file case.
// Apple provides both stream and file loading functions in ImageIO.
// Potentially, Apple can optimize for either case.
SDL_Surface* IMG_Load (const char *file)
{
    SDL_Surface *surface = NULL;
    char *ext = strrchr(file, '.');
    if (ext) {
        ext++;
    }

    if (ext && (SDL_strcasecmp(ext, "ico") == 0 || SDL_strcasecmp(ext, "cur") == 0)) {
        /* CreateCGImageSourceFromFile() doesn't detect the correct format, skip it */
    } else {
        surface = LoadImageFromFile(file);
    }
    if (!surface) {
        // Either the file doesn't exist or ImageIO doesn't understand the format.
        // For the latter case, fallback to the native SDL_image handlers.
        SDL_RWops *src = SDL_RWFromFile(file, "rb");
        if (!src) {
            /* The error message has been set in SDL_RWFromFile */
            return NULL;
        }
        surface = IMG_LoadTyped_RW(src, 1, ext);
    }
    return surface;
}

#endif /* defined(__APPLE__) && !defined(SDL_IMAGE_USE_COMMON_BACKEND) */
