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
 * XPM (X PixMap) image loader:
 *
 * Supports the XPMv3 format, EXCEPT:
 * - hotspot coordinates are ignored
 * - only colour ('c') colour symbols are used
 * - rgb.txt is not used (for portability), so only RGB colours
 *   are recognized (#rrggbb etc) - only a few basic colour names are
 *   handled
 *
 * The result is an 8bpp indexed surface if possible, otherwise 32bpp.
 * The colourkey is correctly set if transparency is used.
 *
 * Besides the standard API, also provides
 *
 *     SDL_Surface *IMG_ReadXPMFromArray(char **xpm)
 *     SDL_Surface *IMG_ReadXPMFromArrayToRGB888(char **xpm)
 *
 * that read the image data from an XPM file included in the C source.
 * - 1st function returns an 8bpp indexed surface if possible, otherwise 32bpp.
 * - 2nd function returns always a 32bpp (RGB888) surface
 *
 * TODO: include rgb.txt here. The full table (from solaris 2.6) only
 * requires about 13K in binary form.
 */

#include "SDL_image.h"

#ifdef LOAD_XPM

/* See if an image is contained in a data source */
int IMG_isXPM(SDL_RWops *src)
{
    Sint64 start;
    int is_XPM;
    char magic[9];

    if ( !src )
        return 0;
    start = SDL_RWtell(src);
    is_XPM = 0;
    if ( SDL_RWread(src, magic, sizeof(magic), 1) ) {
        if ( SDL_memcmp(magic, "/* XPM */", sizeof(magic)) == 0 ) {
            is_XPM = 1;
        }
    }
    SDL_RWseek(src, start, RW_SEEK_SET);
    return(is_XPM);
}

/* Hash table to look up colors from pixel strings */
#define STARTING_HASH_SIZE 256

struct hash_entry {
    char *key;
    Uint32 color;
    struct hash_entry *next;
};

struct color_hash {
    struct hash_entry **table;
    struct hash_entry *entries; /* array of all entries */
    struct hash_entry *next_free;
    int size;
    int maxnum;
};

static int hash_key(const char *key, int cpp, int size)
{
    int hash;

    hash = 0;
    while ( cpp-- > 0 ) {
        hash = hash * 33 + *key++;
    }
    return hash & (size - 1);
}

static struct color_hash *create_colorhash(int maxnum)
{
    int bytes, s;
    struct color_hash *hash;

    /* we know how many entries we need, so we can allocate
       everything here */
    hash = (struct color_hash *)SDL_calloc(1, sizeof(*hash));
    if (!hash)
        return NULL;

    /* use power-of-2 sized hash table for decoding speed */
    for (s = STARTING_HASH_SIZE; s < maxnum; s <<= 1)
        ;
    hash->size = s;
    hash->maxnum = maxnum;

    bytes = hash->size * sizeof(struct hash_entry **);
    /* Check for overflow */
    if ((bytes / sizeof(struct hash_entry **)) != hash->size) {
        IMG_SetError("memory allocation overflow");
        SDL_free(hash);
        return NULL;
    }
    hash->table = (struct hash_entry **)SDL_calloc(1, bytes);
    if (!hash->table) {
        SDL_free(hash);
        return NULL;
    }

    bytes = maxnum * sizeof(struct hash_entry);
    /* Check for overflow */
    if ((bytes / sizeof(struct hash_entry)) != maxnum) {
        IMG_SetError("memory allocation overflow");
        SDL_free(hash->table);
        SDL_free(hash);
        return NULL;
    }
    hash->entries = (struct hash_entry *)SDL_calloc(1, bytes);
    if (!hash->entries) {
        SDL_free(hash->table);
        SDL_free(hash);
        return NULL;
    }
    hash->next_free = hash->entries;
    return hash;
}

static int add_colorhash(struct color_hash *hash,
                         char *key, int cpp, Uint32 color)
{
    int index = hash_key(key, cpp, hash->size);
    struct hash_entry *e = hash->next_free++;
    e->color = color;
    e->key = key;
    e->next = hash->table[index];
    hash->table[index] = e;
    return 1;
}

/* fast lookup that works if cpp == 1 */
#define QUICK_COLORHASH(hash, key) ((hash)->table[*(Uint8 *)(key)]->color)

static Uint32 get_colorhash(struct color_hash *hash, const char *key, int cpp)
{
    struct hash_entry *entry = hash->table[hash_key(key, cpp, hash->size)];
    while (entry) {
        if (SDL_memcmp(key, entry->key, cpp) == 0)
            return entry->color;
        entry = entry->next;
    }
    return 0;       /* garbage in - garbage out */
}

static void free_colorhash(struct color_hash *hash)
{
    if (hash) {
        if (hash->table)
            SDL_free(hash->table);
        if (hash->entries)
            SDL_free(hash->entries);
        SDL_free(hash);
    }
}

/*
 * convert colour spec to RGB (in 0xaarrggbb format).
 * return 1 if successful.
 */
static int color_to_argb(char *spec, int speclen, Uint32 *argb)
{
    /* poor man's rgb.txt */
    static struct { char *name; Uint32 argb; } known[] = {
        { "none",                 0x00000000 },
        { "black",                0xff000000 },
        { "white",                0xffFFFFFF },
        { "red",                  0xffFF0000 },
        { "green",                0xff00FF00 },
        { "blue",                 0xff0000FF },
/* This table increases the size of the library by 40K, so it's disabled by default */
#ifdef EXTENDED_XPM_COLORS
        { "aliceblue",            0xfff0f8ff },
        { "antiquewhite",         0xfffaebd7 },
        { "antiquewhite1",        0xffffefdb },
        { "antiquewhite2",        0xffeedfcc },
        { "antiquewhite3",        0xffcdc0b0 },
        { "antiquewhite4",        0xff8b8378 },
        { "aqua",                 0xff00ffff },
        { "aquamarine",           0xff7fffd4 },
        { "aquamarine1",          0xff7fffd4 },
        { "aquamarine2",          0xff76eec6 },
        { "aquamarine3",          0xff66cdaa },
        { "aquamarine4",          0xff458b74 },
        { "azure",                0xfff0ffff },
        { "azure1",               0xfff0ffff },
        { "azure2",               0xffe0eeee },
        { "azure3",               0xffc1cdcd },
        { "azure4",               0xff838b8b },
        { "beige",                0xfff5f5dc },
        { "bisque",               0xffffe4c4 },
        { "bisque1",              0xffffe4c4 },
        { "bisque2",              0xffeed5b7 },
        { "bisque3",              0xffcdb79e },
        { "bisque4",              0xff8b7d6b },
        { "black",                0xff000000 },
        { "blanchedalmond",       0xffffebcd },
        { "blue",                 0xff0000ff },
        { "blue1",                0xff0000ff },
        { "blue2",                0xff0000ee },
        { "blue3",                0xff0000cd },
        { "blue4",                0xff00008B },
        { "blueviolet",           0xff8a2be2 },
        { "brown",                0xffA52A2A },
        { "brown1",               0xffFF4040 },
        { "brown2",               0xffEE3B3B },
        { "brown3",               0xffCD3333 },
        { "brown4",               0xff8B2323 },
        { "burlywood",            0xffDEB887 },
        { "burlywood1",           0xffFFD39B },
        { "burlywood2",           0xffEEC591 },
        { "burlywood3",           0xffCDAA7D },
        { "burlywood4",           0xff8B7355 },
        { "cadetblue",            0xff5F9EA0 },
        { "cadetblue",            0xff5f9ea0 },
        { "cadetblue1",           0xff98f5ff },
        { "cadetblue2",           0xff8ee5ee },
        { "cadetblue3",           0xff7ac5cd },
        { "cadetblue4",           0xff53868b },
        { "chartreuse",           0xff7FFF00 },
        { "chartreuse1",          0xff7FFF00 },
        { "chartreuse2",          0xff76EE00 },
        { "chartreuse3",          0xff66CD00 },
        { "chartreuse4",          0xff458B00 },
        { "chocolate",            0xffD2691E },
        { "chocolate1",           0xffFF7F24 },
        { "chocolate2",           0xffEE7621 },
        { "chocolate3",           0xffCD661D },
        { "chocolate4",           0xff8B4513 },
        { "coral",                0xffFF7F50 },
        { "coral1",               0xffFF7256 },
        { "coral2",               0xffEE6A50 },
        { "coral3",               0xffCD5B45 },
        { "coral4",               0xff8B3E2F },
        { "cornflowerblue",       0xff6495ed },
        { "cornsilk",             0xffFFF8DC },
        { "cornsilk1",            0xffFFF8DC },
        { "cornsilk2",            0xffEEE8CD },
        { "cornsilk3",            0xffCDC8B1 },
        { "cornsilk4",            0xff8B8878 },
        { "crimson",              0xffDC143C },
        { "cyan",                 0xff00FFFF },
        { "cyan1",                0xff00FFFF },
        { "cyan2",                0xff00EEEE },
        { "cyan3",                0xff00CDCD },
        { "cyan4",                0xff008B8B },
        { "darkblue",             0xff00008b },
        { "darkcyan",             0xff008b8b },
        { "darkgoldenrod",        0xffb8860b },
        { "darkgoldenrod1",       0xffffb90f },
        { "darkgoldenrod2",       0xffeead0e },
        { "darkgoldenrod3",       0xffcd950c },
        { "darkgoldenrod4",       0xff8b6508 },
        { "darkgray",             0xffa9a9a9 },
        { "darkgreen",            0xff006400 },
        { "darkgrey",             0xffa9a9a9 },
        { "darkkhaki",            0xffbdb76b },
        { "darkmagenta",          0xff8b008b },
        { "darkolivegreen",       0xff556b2f },
        { "darkolivegreen1",      0xffcaff70 },
        { "darkolivegreen2",      0xffbcee68 },
        { "darkolivegreen3",      0xffa2cd5a },
        { "darkolivegreen4",      0xff6e8b3d },
        { "darkorange",           0xffff8c00 },
        { "darkorange1",          0xffff7f00 },
        { "darkorange2",          0xffee7600 },
        { "darkorange3",          0xffcd6600 },
        { "darkorange4",          0xff8b4500 },
        { "darkorchid",           0xff9932cc },
        { "darkorchid1",          0xffbf3eff },
        { "darkorchid2",          0xffb23aee },
        { "darkorchid3",          0xff9a32cd },
        { "darkorchid4",          0xff68228b },
        { "darkred",              0xff8b0000 },
        { "darksalmon",           0xffe9967a },
        { "darkseagreen",         0xff8fbc8f },
        { "darkseagreen1",        0xffc1ffc1 },
        { "darkseagreen2",        0xffb4eeb4 },
        { "darkseagreen3",        0xff9bcd9b },
        { "darkseagreen4",        0xff698b69 },
        { "darkslateblue",        0xff483d8b },
        { "darkslategray",        0xff2f4f4f },
        { "darkslategray1",       0xff97ffff },
        { "darkslategray2",       0xff8deeee },
        { "darkslategray3",       0xff79cdcd },
        { "darkslategray4",       0xff528b8b },
        { "darkslategrey",        0xff2f4f4f },
        { "darkturquoise",        0xff00ced1 },
        { "darkviolet",           0xff9400D3 },
        { "darkviolet",           0xff9400d3 },
        { "deeppink",             0xffff1493 },
        { "deeppink1",            0xffff1493 },
        { "deeppink2",            0xffee1289 },
        { "deeppink3",            0xffcd1076 },
        { "deeppink4",            0xff8b0a50 },
        { "deepskyblue",          0xff00bfff },
        { "deepskyblue1",         0xff00bfff },
        { "deepskyblue2",         0xff00b2ee },
        { "deepskyblue3",         0xff009acd },
        { "deepskyblue4",         0xff00688b },
        { "dimgray",              0xff696969 },
        { "dimgrey",              0xff696969 },
        { "dodgerblue",           0xff1e90ff },
        { "dodgerblue1",          0xff1e90ff },
        { "dodgerblue2",          0xff1c86ee },
        { "dodgerblue3",          0xff1874cd },
        { "dodgerblue4",          0xff104e8b },
        { "firebrick",            0xffB22222 },
        { "firebrick1",           0xffFF3030 },
        { "firebrick2",           0xffEE2C2C },
        { "firebrick3",           0xffCD2626 },
        { "firebrick4",           0xff8B1A1A },
        { "floralwhite",          0xfffffaf0 },
        { "forestgreen",          0xff228b22 },
        { "fractal",              0xff808080 },
        { "fuchsia",              0xffFF00FF },
        { "gainsboro",            0xffDCDCDC },
        { "ghostwhite",           0xfff8f8ff },
        { "gold",                 0xffFFD700 },
        { "gold1",                0xffFFD700 },
        { "gold2",                0xffEEC900 },
        { "gold3",                0xffCDAD00 },
        { "gold4",                0xff8B7500 },
        { "goldenrod",            0xffDAA520 },
        { "goldenrod1",           0xffFFC125 },
        { "goldenrod2",           0xffEEB422 },
        { "goldenrod3",           0xffCD9B1D },
        { "goldenrod4",           0xff8B6914 },
        { "gray",                 0xff7E7E7E },
        { "gray",                 0xffBEBEBE },
        { "gray0",                0xff000000 },
        { "gray1",                0xff030303 },
        { "gray10",               0xff1A1A1A },
        { "gray100",              0xffFFFFFF },
        { "gray11",               0xff1C1C1C },
        { "gray12",               0xff1F1F1F },
        { "gray13",               0xff212121 },
        { "gray14",               0xff242424 },
        { "gray15",               0xff262626 },
        { "gray16",               0xff292929 },
        { "gray17",               0xff2B2B2B },
        { "gray18",               0xff2E2E2E },
        { "gray19",               0xff303030 },
        { "gray2",                0xff050505 },
        { "gray20",               0xff333333 },
        { "gray21",               0xff363636 },
        { "gray22",               0xff383838 },
        { "gray23",               0xff3B3B3B },
        { "gray24",               0xff3D3D3D },
        { "gray25",               0xff404040 },
        { "gray26",               0xff424242 },
        { "gray27",               0xff454545 },
        { "gray28",               0xff474747 },
        { "gray29",               0xff4A4A4A },
        { "gray3",                0xff080808 },
        { "gray30",               0xff4D4D4D },
        { "gray31",               0xff4F4F4F },
        { "gray32",               0xff525252 },
        { "gray33",               0xff545454 },
        { "gray34",               0xff575757 },
        { "gray35",               0xff595959 },
        { "gray36",               0xff5C5C5C },
        { "gray37",               0xff5E5E5E },
        { "gray38",               0xff616161 },
        { "gray39",               0xff636363 },
        { "gray4",                0xff0A0A0A },
        { "gray40",               0xff666666 },
        { "gray41",               0xff696969 },
        { "gray42",               0xff6B6B6B },
        { "gray43",               0xff6E6E6E },
        { "gray44",               0xff707070 },
        { "gray45",               0xff737373 },
        { "gray46",               0xff757575 },
        { "gray47",               0xff787878 },
        { "gray48",               0xff7A7A7A },
        { "gray49",               0xff7D7D7D },
        { "gray5",                0xff0D0D0D },
        { "gray50",               0xff7F7F7F },
        { "gray51",               0xff828282 },
        { "gray52",               0xff858585 },
        { "gray53",               0xff878787 },
        { "gray54",               0xff8A8A8A },
        { "gray55",               0xff8C8C8C },
        { "gray56",               0xff8F8F8F },
        { "gray57",               0xff919191 },
        { "gray58",               0xff949494 },
        { "gray59",               0xff969696 },
        { "gray6",                0xff0F0F0F },
        { "gray60",               0xff999999 },
        { "gray61",               0xff9C9C9C },
        { "gray62",               0xff9E9E9E },
        { "gray63",               0xffA1A1A1 },
        { "gray64",               0xffA3A3A3 },
        { "gray65",               0xffA6A6A6 },
        { "gray66",               0xffA8A8A8 },
        { "gray67",               0xffABABAB },
        { "gray68",               0xffADADAD },
        { "gray69",               0xffB0B0B0 },
        { "gray7",                0xff121212 },
        { "gray70",               0xffB3B3B3 },
        { "gray71",               0xffB5B5B5 },
        { "gray72",               0xffB8B8B8 },
        { "gray73",               0xffBABABA },
        { "gray74",               0xffBDBDBD },
        { "gray75",               0xffBFBFBF },
        { "gray76",               0xffC2C2C2 },
        { "gray77",               0xffC4C4C4 },
        { "gray78",               0xffC7C7C7 },
        { "gray79",               0xffC9C9C9 },
        { "gray8",                0xff141414 },
        { "gray80",               0xffCCCCCC },
        { "gray81",               0xffCFCFCF },
        { "gray82",               0xffD1D1D1 },
        { "gray83",               0xffD4D4D4 },
        { "gray84",               0xffD6D6D6 },
        { "gray85",               0xffD9D9D9 },
        { "gray86",               0xffDBDBDB },
        { "gray87",               0xffDEDEDE },
        { "gray88",               0xffE0E0E0 },
        { "gray89",               0xffE3E3E3 },
        { "gray9",                0xff171717 },
        { "gray90",               0xffE5E5E5 },
        { "gray91",               0xffE8E8E8 },
        { "gray92",               0xffEBEBEB },
        { "gray93",               0xffEDEDED },
        { "gray94",               0xffF0F0F0 },
        { "gray95",               0xffF2F2F2 },
        { "gray96",               0xffF5F5F5 },
        { "gray97",               0xffF7F7F7 },
        { "gray98",               0xffFAFAFA },
        { "gray99",               0xffFCFCFC },
        { "green",                0xff008000 },
        { "green",                0xff00FF00 },
        { "green1",               0xff00FF00 },
        { "green2",               0xff00EE00 },
        { "green3",               0xff00CD00 },
        { "green4",               0xff008B00 },
        { "greenyellow",          0xffadff2f },
        { "grey",                 0xffBEBEBE },
        { "grey0",                0xff000000 },
        { "grey1",                0xff030303 },
        { "grey10",               0xff1A1A1A },
        { "grey100",              0xffFFFFFF },
        { "grey11",               0xff1C1C1C },
        { "grey12",               0xff1F1F1F },
        { "grey13",               0xff212121 },
        { "grey14",               0xff242424 },
        { "grey15",               0xff262626 },
        { "grey16",               0xff292929 },
        { "grey17",               0xff2B2B2B },
        { "grey18",               0xff2E2E2E },
        { "grey19",               0xff303030 },
        { "grey2",                0xff050505 },
        { "grey20",               0xff333333 },
        { "grey21",               0xff363636 },
        { "grey22",               0xff383838 },
        { "grey23",               0xff3B3B3B },
        { "grey24",               0xff3D3D3D },
        { "grey25",               0xff404040 },
        { "grey26",               0xff424242 },
        { "grey27",               0xff454545 },
        { "grey28",               0xff474747 },
        { "grey29",               0xff4A4A4A },
        { "grey3",                0xff080808 },
        { "grey30",               0xff4D4D4D },
        { "grey31",               0xff4F4F4F },
        { "grey32",               0xff525252 },
        { "grey33",               0xff545454 },
        { "grey34",               0xff575757 },
        { "grey35",               0xff595959 },
        { "grey36",               0xff5C5C5C },
        { "grey37",               0xff5E5E5E },
        { "grey38",               0xff616161 },
        { "grey39",               0xff636363 },
        { "grey4",                0xff0A0A0A },
        { "grey40",               0xff666666 },
        { "grey41",               0xff696969 },
        { "grey42",               0xff6B6B6B },
        { "grey43",               0xff6E6E6E },
        { "grey44",               0xff707070 },
        { "grey45",               0xff737373 },
        { "grey46",               0xff757575 },
        { "grey47",               0xff787878 },
        { "grey48",               0xff7A7A7A },
        { "grey49",               0xff7D7D7D },
        { "grey5",                0xff0D0D0D },
        { "grey50",               0xff7F7F7F },
        { "grey51",               0xff828282 },
        { "grey52",               0xff858585 },
        { "grey53",               0xff878787 },
        { "grey54",               0xff8A8A8A },
        { "grey55",               0xff8C8C8C },
        { "grey56",               0xff8F8F8F },
        { "grey57",               0xff919191 },
        { "grey58",               0xff949494 },
        { "grey59",               0xff969696 },
        { "grey6",                0xff0F0F0F },
        { "grey60",               0xff999999 },
        { "grey61",               0xff9C9C9C },
        { "grey62",               0xff9E9E9E },
        { "grey63",               0xffA1A1A1 },
        { "grey64",               0xffA3A3A3 },
        { "grey65",               0xffA6A6A6 },
        { "grey66",               0xffA8A8A8 },
        { "grey67",               0xffABABAB },
        { "grey68",               0xffADADAD },
        { "grey69",               0xffB0B0B0 },
        { "grey7",                0xff121212 },
        { "grey70",               0xffB3B3B3 },
        { "grey71",               0xffB5B5B5 },
        { "grey72",               0xffB8B8B8 },
        { "grey73",               0xffBABABA },
        { "grey74",               0xffBDBDBD },
        { "grey75",               0xffBFBFBF },
        { "grey76",               0xffC2C2C2 },
        { "grey77",               0xffC4C4C4 },
        { "grey78",               0xffC7C7C7 },
        { "grey79",               0xffC9C9C9 },
        { "grey8",                0xff141414 },
        { "grey80",               0xffCCCCCC },
        { "grey81",               0xffCFCFCF },
        { "grey82",               0xffD1D1D1 },
        { "grey83",               0xffD4D4D4 },
        { "grey84",               0xffD6D6D6 },
        { "grey85",               0xffD9D9D9 },
        { "grey86",               0xffDBDBDB },
        { "grey87",               0xffDEDEDE },
        { "grey88",               0xffE0E0E0 },
        { "grey89",               0xffE3E3E3 },
        { "grey9",                0xff171717 },
        { "grey90",               0xffE5E5E5 },
        { "grey91",               0xffE8E8E8 },
        { "grey92",               0xffEBEBEB },
        { "grey93",               0xffEDEDED },
        { "grey94",               0xffF0F0F0 },
        { "grey95",               0xffF2F2F2 },
        { "grey96",               0xffF5F5F5 },
        { "grey97",               0xffF7F7F7 },
        { "grey98",               0xffFAFAFA },
        { "grey99",               0xffFCFCFC },
        { "honeydew",             0xffF0FFF0 },
        { "honeydew1",            0xffF0FFF0 },
        { "honeydew2",            0xffE0EEE0 },
        { "honeydew3",            0xffC1CDC1 },
        { "honeydew4",            0xff838B83 },
        { "hotpink",              0xffff69b4 },
        { "hotpink1",             0xffff6eb4 },
        { "hotpink2",             0xffee6aa7 },
        { "hotpink3",             0xffcd6090 },
        { "hotpink4",             0xff8b3a62 },
        { "indianred",            0xffcd5c5c },
        { "indianred1",           0xffff6a6a },
        { "indianred2",           0xffee6363 },
        { "indianred3",           0xffcd5555 },
        { "indianred4",           0xff8b3a3a },
        { "indigo",               0xff4B0082 },
        { "ivory",                0xffFFFFF0 },
        { "ivory1",               0xffFFFFF0 },
        { "ivory2",               0xffEEEEE0 },
        { "ivory3",               0xffCDCDC1 },
        { "ivory4",               0xff8B8B83 },
        { "khaki",                0xffF0E68C },
        { "khaki1",               0xffFFF68F },
        { "khaki2",               0xffEEE685 },
        { "khaki3",               0xffCDC673 },
        { "khaki4",               0xff8B864E },
        { "lavender",             0xffE6E6FA },
        { "lavenderblush",        0xfffff0f5 },
        { "lavenderblush1",       0xfffff0f5 },
        { "lavenderblush2",       0xffeee0e5 },
        { "lavenderblush3",       0xffcdc1c5 },
        { "lavenderblush4",       0xff8b8386 },
        { "lawngreen",            0xff7cfc00 },
        { "lemonchiffon",         0xfffffacd },
        { "lemonchiffon1",        0xfffffacd },
        { "lemonchiffon2",        0xffeee9bf },
        { "lemonchiffon3",        0xffcdc9a5 },
        { "lemonchiffon4",        0xff8b8970 },
        { "lightblue",            0xffadd8e6 },
        { "lightblue1",           0xffbfefff },
        { "lightblue2",           0xffb2dfee },
        { "lightblue3",           0xff9ac0cd },
        { "lightblue4",           0xff68838b },
        { "lightcoral",           0xfff08080 },
        { "lightcyan",            0xffe0ffff },
        { "lightcyan1",           0xffe0ffff },
        { "lightcyan2",           0xffd1eeee },
        { "lightcyan3",           0xffb4cdcd },
        { "lightcyan4",           0xff7a8b8b },
        { "lightgoldenrod",       0xffeedd82 },
        { "lightgoldenrod1",      0xffffec8b },
        { "lightgoldenrod2",      0xffeedc82 },
        { "lightgoldenrod3",      0xffcdbe70 },
        { "lightgoldenrod4",      0xff8b814c },
        { "lightgoldenrodyellow", 0xfffafad2 },
        { "lightgray",            0xffd3d3d3 },
        { "lightgreen",           0xff90ee90 },
        { "lightgrey",            0xffd3d3d3 },
        { "lightpink",            0xffffb6c1 },
        { "lightpink1",           0xffffaeb9 },
        { "lightpink2",           0xffeea2ad },
        { "lightpink3",           0xffcd8c95 },
        { "lightpink4",           0xff8b5f65 },
        { "lightsalmon",          0xffffa07a },
        { "lightsalmon1",         0xffffa07a },
        { "lightsalmon2",         0xffee9572 },
        { "lightsalmon3",         0xffcd8162 },
        { "lightsalmon4",         0xff8b5742 },
        { "lightseagreen",        0xff20b2aa },
        { "lightskyblue",         0xff87cefa },
        { "lightskyblue1",        0xffb0e2ff },
        { "lightskyblue2",        0xffa4d3ee },
        { "lightskyblue3",        0xff8db6cd },
        { "lightskyblue4",        0xff607b8b },
        { "lightslateblue",       0xff8470ff },
        { "lightslategray",       0xff778899 },
        { "lightslategrey",       0xff778899 },
        { "lightsteelblue",       0xffb0c4de },
        { "lightsteelblue1",      0xffcae1ff },
        { "lightsteelblue2",      0xffbcd2ee },
        { "lightsteelblue3",      0xffa2b5cd },
        { "lightsteelblue4",      0xff6e7b8b },
        { "lightyellow",          0xffffffe0 },
        { "lightyellow1",         0xffffffe0 },
        { "lightyellow2",         0xffeeeed1 },
        { "lightyellow3",         0xffcdcdb4 },
        { "lightyellow4",         0xff8b8b7a },
        { "lime",                 0xff00FF00 },
        { "limegreen",            0xff32cd32 },
        { "linen",                0xffFAF0E6 },
        { "magenta",              0xffFF00FF },
        { "magenta1",             0xffFF00FF },
        { "magenta2",             0xffEE00EE },
        { "magenta3",             0xffCD00CD },
        { "magenta4",             0xff8B008B },
        { "maroon",               0xff800000 },
        { "maroon",               0xffB03060 },
        { "maroon1",              0xffFF34B3 },
        { "maroon2",              0xffEE30A7 },
        { "maroon3",              0xffCD2990 },
        { "maroon4",              0xff8B1C62 },
        { "mediumaquamarine",     0xff66cdaa },
        { "mediumblue",           0xff0000cd },
        { "mediumforestgreen",    0xff32814b },
        { "mediumgoldenrod",      0xffd1c166 },
        { "mediumorchid",         0xffba55d3 },
        { "mediumorchid1",        0xffe066ff },
        { "mediumorchid2",        0xffd15fee },
        { "mediumorchid3",        0xffb452cd },
        { "mediumorchid4",        0xff7a378b },
        { "mediumpurple",         0xff9370db },
        { "mediumpurple1",        0xffab82ff },
        { "mediumpurple2",        0xff9f79ee },
        { "mediumpurple3",        0xff8968cd },
        { "mediumpurple4",        0xff5d478b },
        { "mediumseagreen",       0xff3cb371 },
        { "mediumslateblue",      0xff7b68ee },
        { "mediumspringgreen",    0xff00fa9a },
        { "mediumturquoise",      0xff48d1cc },
        { "mediumvioletred",      0xffc71585 },
        { "midnightblue",         0xff191970 },
        { "mintcream",            0xfff5fffa },
        { "mistyrose",            0xffffe4e1 },
        { "mistyrose1",           0xffffe4e1 },
        { "mistyrose2",           0xffeed5d2 },
        { "mistyrose3",           0xffcdb7b5 },
        { "mistyrose4",           0xff8b7d7b },
        { "moccasin",             0xffFFE4B5 },
        { "navajowhite",          0xffffdead },
        { "navajowhite1",         0xffffdead },
        { "navajowhite2",         0xffeecfa1 },
        { "navajowhite3",         0xffcdb38b },
        { "navajowhite4",         0xff8b795e },
        { "navy",                 0xff000080 },
        { "navyblue",             0xff000080 },
        { "none",                 0xff0000FF },
        { "oldlace",              0xfffdf5e6 },
        { "olive",                0xff808000 },
        { "olivedrab",            0xff6b8e23 },
        { "olivedrab1",           0xffc0ff3e },
        { "olivedrab2",           0xffb3ee3a },
        { "olivedrab3",           0xff9acd32 },
        { "olivedrab4",           0xff698b22 },
        { "opaque",               0xff000000 },
        { "orange",               0xffFFA500 },
        { "orange1",              0xffFFA500 },
        { "orange2",              0xffEE9A00 },
        { "orange3",              0xffCD8500 },
        { "orange4",              0xff8B5A00 },
        { "orangered",            0xffff4500 },
        { "orangered1",           0xffff4500 },
        { "orangered2",           0xffee4000 },
        { "orangered3",           0xffcd3700 },
        { "orangered4",           0xff8b2500 },
        { "orchid",               0xffDA70D6 },
        { "orchid1",              0xffFF83FA },
        { "orchid2",              0xffEE7AE9 },
        { "orchid3",              0xffCD69C9 },
        { "orchid4",              0xff8B4789 },
        { "palegoldenrod",        0xffeee8aa },
        { "palegreen",            0xff98fb98 },
        { "palegreen1",           0xff9aff9a },
        { "palegreen2",           0xff90ee90 },
        { "palegreen3",           0xff7ccd7c },
        { "palegreen4",           0xff548b54 },
        { "paleturquoise",        0xffafeeee },
        { "paleturquoise1",       0xffbbffff },
        { "paleturquoise2",       0xffaeeeee },
        { "paleturquoise3",       0xff96cdcd },
        { "paleturquoise4",       0xff668b8b },
        { "palevioletred",        0xffdb7093 },
        { "palevioletred1",       0xffff82ab },
        { "palevioletred2",       0xffee799f },
        { "palevioletred3",       0xffcd6889 },
        { "palevioletred4",       0xff8b475d },
        { "papayawhip",           0xffffefd5 },
        { "peachpuff",            0xffffdab9 },
        { "peachpuff1",           0xffffdab9 },
        { "peachpuff2",           0xffeecbad },
        { "peachpuff3",           0xffcdaf95 },
        { "peachpuff4",           0xff8b7765 },
        { "peru",                 0xffCD853F },
        { "pink",                 0xffFFC0CB },
        { "pink1",                0xffFFB5C5 },
        { "pink2",                0xffEEA9B8 },
        { "pink3",                0xffCD919E },
        { "pink4",                0xff8B636C },
        { "plum",                 0xffDDA0DD },
        { "plum1",                0xffFFBBFF },
        { "plum2",                0xffEEAEEE },
        { "plum3",                0xffCD96CD },
        { "plum4",                0xff8B668B },
        { "powderblue",           0xffb0e0e6 },
        { "purple",               0xff800080 },
        { "purple",               0xffA020F0 },
        { "purple1",              0xff9B30FF },
        { "purple2",              0xff912CEE },
        { "purple3",              0xff7D26CD },
        { "purple4",              0xff551A8B },
        { "red",                  0xffFF0000 },
        { "red1",                 0xffFF0000 },
        { "red2",                 0xffEE0000 },
        { "red3",                 0xffCD0000 },
        { "red4",                 0xff8B0000 },
        { "rosybrown",            0xffbc8f8f },
        { "rosybrown1",           0xffffc1c1 },
        { "rosybrown2",           0xffeeb4b4 },
        { "rosybrown3",           0xffcd9b9b },
        { "rosybrown4",           0xff8b6969 },
        { "royalblue",            0xff4169e1 },
        { "royalblue1",           0xff4876ff },
        { "royalblue2",           0xff436eee },
        { "royalblue3",           0xff3a5fcd },
        { "royalblue4",           0xff27408b },
        { "saddlebrown",          0xff8b4513 },
        { "salmon",               0xffFA8072 },
        { "salmon1",              0xffFF8C69 },
        { "salmon2",              0xffEE8262 },
        { "salmon3",              0xffCD7054 },
        { "salmon4",              0xff8B4C39 },
        { "sandybrown",           0xfff4a460 },
        { "seagreen",             0xff2e8b57 },
        { "seagreen1",            0xff54ff9f },
        { "seagreen2",            0xff4eee94 },
        { "seagreen3",            0xff43cd80 },
        { "seagreen4",            0xff2e8b57 },
        { "seashell",             0xffFFF5EE },
        { "seashell1",            0xffFFF5EE },
        { "seashell2",            0xffEEE5DE },
        { "seashell3",            0xffCDC5BF },
        { "seashell4",            0xff8B8682 },
        { "sienna",               0xffA0522D },
        { "sienna1",              0xffFF8247 },
        { "sienna2",              0xffEE7942 },
        { "sienna3",              0xffCD6839 },
        { "sienna4",              0xff8B4726 },
        { "silver",               0xffC0C0C0 },
        { "skyblue",              0xff87ceeb },
        { "skyblue1",             0xff87ceff },
        { "skyblue2",             0xff7ec0ee },
        { "skyblue3",             0xff6ca6cd },
        { "skyblue4",             0xff4a708b },
        { "slateblue",            0xff6a5acd },
        { "slateblue1",           0xff836fff },
        { "slateblue2",           0xff7a67ee },
        { "slateblue3",           0xff6959cd },
        { "slateblue4",           0xff473c8b },
        { "slategray",            0xff708090 },
        { "slategray1",           0xffc6e2ff },
        { "slategray2",           0xffb9d3ee },
        { "slategray3",           0xff9fb6cd },
        { "slategray4",           0xff6c7b8b },
        { "slategrey",            0xff708090 },
        { "snow",                 0xffFFFAFA },
        { "snow1",                0xffFFFAFA },
        { "snow2",                0xffEEE9E9 },
        { "snow3",                0xffCDC9C9 },
        { "snow4",                0xff8B8989 },
        { "springgreen",          0xff00ff7f },
        { "springgreen1",         0xff00ff7f },
        { "springgreen2",         0xff00ee76 },
        { "springgreen3",         0xff00cd66 },
        { "springgreen4",         0xff008b45 },
        { "steelblue",            0xff4682b4 },
        { "steelblue1",           0xff63b8ff },
        { "steelblue2",           0xff5cacee },
        { "steelblue3",           0xff4f94cd },
        { "steelblue4",           0xff36648b },
        { "tan",                  0xffD2B48C },
        { "tan1",                 0xffFFA54F },
        { "tan2",                 0xffEE9A49 },
        { "tan3",                 0xffCD853F },
        { "tan4",                 0xff8B5A2B },
        { "teal",                 0xff008080 },
        { "thistle",              0xffD8BFD8 },
        { "thistle1",             0xffFFE1FF },
        { "thistle2",             0xffEED2EE },
        { "thistle3",             0xffCDB5CD },
        { "thistle4",             0xff8B7B8B },
        { "tomato",               0xffFF6347 },
        { "tomato1",              0xffFF6347 },
        { "tomato2",              0xffEE5C42 },
        { "tomato3",              0xffCD4F39 },
        { "tomato4",              0xff8B3626 },
        { "transparent",          0xff0000FF },
        { "turquoise",            0xff40E0D0 },
        { "turquoise1",           0xff00F5FF },
        { "turquoise2",           0xff00E5EE },
        { "turquoise3",           0xff00C5CD },
        { "turquoise4",           0xff00868B },
        { "violet",               0xffEE82EE },
        { "violetred",            0xffd02090 },
        { "violetred1",           0xffff3e96 },
        { "violetred2",           0xffee3a8c },
        { "violetred3",           0xffcd3278 },
        { "violetred4",           0xff8b2252 },
        { "wheat",                0xffF5DEB3 },
        { "wheat1",               0xffFFE7BA },
        { "wheat2",               0xffEED8AE },
        { "wheat3",               0xffCDBA96 },
        { "wheat4",               0xff8B7E66 },
        { "white",                0xffFFFFFF },
        { "whitesmoke",           0xfff5f5f5 },
        { "yellow",               0xffFFFF00 },
        { "yellow1",              0xffFFFF00 },
        { "yellow2",              0xffEEEE00 },
        { "yellow3",              0xffCDCD00 },
        { "yellow4",              0xff8B8B00 },
        { "yellowgreen",          0xff9acd32 },
#endif /* EXTENDED_XPM_COLORS */
    };

    if (spec[0] == '#') {
        char buf[7];
        switch(speclen) {
        case 4:
            buf[0] = buf[1] = spec[1];
            buf[2] = buf[3] = spec[2];
            buf[4] = buf[5] = spec[3];
            break;
        case 7:
            SDL_memcpy(buf, spec + 1, 6);
            break;
        case 13:
            buf[0] = spec[1];
            buf[1] = spec[2];
            buf[2] = spec[5];
            buf[3] = spec[6];
            buf[4] = spec[9];
            buf[5] = spec[10];
            break;
        }
        buf[6] = '\0';
        *argb = 0xff000000 | (Uint32)SDL_strtol(buf, NULL, 16);
        return 1;
    } else {
        int i;
        for (i = 0; i < SDL_arraysize(known); i++) {
            if (SDL_strncasecmp(known[i].name, spec, speclen) == 0) {
                *argb = known[i].argb;
                return 1;
            }
        }
        return 0;
    }
}

static char *linebuf;
static int buflen;
static char *error;

/*
 * Read next line from the source.
 * If len > 0, it's assumed to be at least len chars (for efficiency).
 * Return NULL and set error upon EOF or parse error.
 */
static char *get_next_line(char ***lines, SDL_RWops *src, int len)
{
    char *linebufnew;

    if (lines) {
        return *(*lines)++;
    } else {
        char c;
        int n;
        do {
            if (!SDL_RWread(src, &c, 1, 1)) {
                error = "Premature end of data";
                return NULL;
            }
        } while (c != '"');
        if (len) {
            len += 4;   /* "\",\n\0" */
            if (len > buflen){
                buflen = len;
                linebufnew = (char *)SDL_realloc(linebuf, buflen);
                if (!linebufnew) {
                    SDL_free(linebuf);
                    error = "Out of memory";
                    return NULL;
                }
                linebuf = linebufnew;
            }
            if (!SDL_RWread(src, linebuf, len - 1, 1)) {
                error = "Premature end of data";
                return NULL;
            }
            n = len - 2;
        } else {
            n = 0;
            do {
                if (n >= buflen - 1) {
                    if (buflen == 0)
                        buflen = 16;
                    buflen *= 2;
                    linebufnew = (char *)SDL_realloc(linebuf, buflen);
                    if (!linebufnew) {
                        SDL_free(linebuf);
                        error = "Out of memory";
                        return NULL;
                    }
                    linebuf = linebufnew;
                }
                if (!SDL_RWread(src, linebuf + n, 1, 1)) {
                    error = "Premature end of data";
                    return NULL;
                }
            } while (linebuf[n++] != '"');
            n--;
        }
        linebuf[n] = '\0';
        return linebuf;
    }
}

#define SKIPSPACE(p)                \
do {                        \
    while (SDL_isspace((unsigned char)*(p))) \
          ++(p);                \
} while (0)

#define SKIPNONSPACE(p)                 \
do {                            \
    while (!SDL_isspace((unsigned char)*(p)) && *p)  \
          ++(p);                    \
} while (0)

/* read XPM from either array or RWops */
static SDL_Surface *load_xpm(char **xpm, SDL_RWops *src, SDL_bool force_32bit)
{
    Sint64 start = 0;
    SDL_Surface *image = NULL;
    int index;
    int x, y;
    int w, h, ncolors, cpp;
    int indexed;
    Uint8 *dst;
    struct color_hash *colors = NULL;
    SDL_Color *im_colors = NULL;
    char *keystrings = NULL, *nextkey;
    char *line;
    char ***xpmlines = NULL;
    int pixels_len;

    error = NULL;
    linebuf = NULL;
    buflen = 0;

    if (src)
        start = SDL_RWtell(src);

    if (xpm)
        xpmlines = &xpm;

    line = get_next_line(xpmlines, src, 0);
    if (!line)
        goto done;
    /*
     * The header string of an XPMv3 image has the format
     *
     * <width> <height> <ncolors> <cpp> [ <hotspot_x> <hotspot_y> ]
     *
     * where the hotspot coords are intended for mouse cursors.
     * Right now we don't use the hotspots but it should be handled
     * one day.
     */
    if (SDL_sscanf(line, "%d %d %d %d", &w, &h, &ncolors, &cpp) != 4
       || w <= 0 || h <= 0 || ncolors <= 0 || cpp <= 0) {
        error = "Invalid format description";
        goto done;
    }

    /* Check for allocation overflow */
    if ((size_t)(ncolors * cpp)/cpp != ncolors) {
        error = "Invalid color specification";
        goto done;
    }
    keystrings = (char *)SDL_malloc(ncolors * cpp);
    if (!keystrings) {
        error = "Out of memory";
        goto done;
    }
    nextkey = keystrings;

    /* Create the new surface */
    if (ncolors <= 256 && !force_32bit) {
        indexed = 1;
        image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8,
                         0, 0, 0, 0);
        im_colors = image->format->palette->colors;
        image->format->palette->ncolors = ncolors;
    } else {
        indexed = 0;
        image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
                         0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    }
    if (!image) {
        /* Hmm, some SDL error (out of memory?) */
        goto done;
    }

    /* Read the colors */
    colors = create_colorhash(ncolors);
    if (!colors) {
        error = "Out of memory";
        goto done;
    }
    for (index = 0; index < ncolors; ++index ) {
        char *p;
        line = get_next_line(xpmlines, src, 0);
        if (!line)
            goto done;

        p = line + cpp + 1;

        /* parse a colour definition */
        for (;;) {
            char nametype;
            char *colname;
            Uint32 argb, pixel;

            SKIPSPACE(p);
            if (!*p) {
                error = "colour parse error";
                goto done;
            }
            nametype = *p;
            SKIPNONSPACE(p);
            SKIPSPACE(p);
            colname = p;
            SKIPNONSPACE(p);
            if (nametype == 's')
                continue;      /* skip symbolic colour names */

            if (!color_to_argb(colname, (int)(p - colname), &argb))
                continue;

            SDL_memcpy(nextkey, line, cpp);
            if (indexed) {
                SDL_Color *c = im_colors + index;
                c->a = (Uint8)(argb >> 24);
                c->r = (Uint8)(argb >> 16);
                c->g = (Uint8)(argb >> 8);
                c->b = (Uint8)(argb);
                pixel = index;
                if (argb == 0x00000000) {
                    SDL_SetColorKey(image, SDL_TRUE, pixel);
                }
            } else {
                pixel = argb;
            }
            add_colorhash(colors, nextkey, cpp, pixel);
            nextkey += cpp;
            break;
        }
    }

    /* Read the pixels */
    pixels_len = w * cpp;
    dst = (Uint8 *)image->pixels;
    for (y = 0; y < h; y++) {
        line = get_next_line(xpmlines, src, pixels_len);
        if (!line)
            goto done;

        if (indexed) {
            /* optimization for some common cases */
            if (cpp == 1)
                for (x = 0; x < w; x++)
                    dst[x] = (Uint8)QUICK_COLORHASH(colors,
                                 line + x);
            else
                for (x = 0; x < w; x++)
                    dst[x] = (Uint8)get_colorhash(colors,
                                   line + x * cpp,
                                   cpp);
        } else {
            for (x = 0; x < w; x++)
                ((Uint32*)dst)[x] = get_colorhash(colors,
                                line + x * cpp,
                                  cpp);
        }
        dst += image->pitch;
    }

done:
    if (error) {
        if ( src )
            SDL_RWseek(src, start, RW_SEEK_SET);
        if ( image ) {
            SDL_FreeSurface(image);
            image = NULL;
        }
        IMG_SetError("%s", error);
    }
    if (keystrings)
        SDL_free(keystrings);
    free_colorhash(colors);
    if (linebuf)
        SDL_free(linebuf);
    return(image);
}

/* Load a XPM type image from an RWops datasource */
SDL_Surface *IMG_LoadXPM_RW(SDL_RWops *src)
{
    if ( !src ) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }
    return load_xpm(NULL, src, 0);
}

SDL_Surface *IMG_ReadXPMFromArray(char **xpm)
{
    if (!xpm) {
        IMG_SetError("array is NULL");
        return NULL;
    }
    return load_xpm(xpm, NULL, SDL_FALSE);
}

SDL_Surface *IMG_ReadXPMFromArrayToRGB888(char **xpm)
{
    if (!xpm) {
        IMG_SetError("array is NULL");
        return NULL;
    }
    return load_xpm(xpm, NULL, SDL_TRUE /* force_32bit */);
}

#else  /* not LOAD_XPM */
#if _MSC_VER >= 1300
#pragma warning(disable : 4100) /* warning C4100: 'op' : unreferenced formal parameter */
#endif

/* See if an image is contained in a data source */
int IMG_isXPM(SDL_RWops *src)
{
    return(0);
}


/* Load a XPM type image from an SDL datasource */
SDL_Surface *IMG_LoadXPM_RW(SDL_RWops *src)
{
    return(NULL);
}

SDL_Surface *IMG_ReadXPMFromArray(char **xpm)
{
    return NULL;
}

SDL_Surface *IMG_ReadXPMFromArrayToRGB888(char **xpm)
{
    return NULL;
}

#endif /* not LOAD_XPM */
