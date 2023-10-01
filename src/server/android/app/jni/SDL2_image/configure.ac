dnl Process this file with autoconf to produce a configure script.

dnl Set various version strings - taken gratefully from the GTk sources
# See docs/release_checklist.md
m4_define([MAJOR_VERSION_MACRO], [2])
m4_define([MINOR_VERSION_MACRO], [6])
m4_define([MICRO_VERSION_MACRO], [3])

AC_INIT([SDL2_image],
        [MAJOR_VERSION_MACRO.MINOR_VERSION_MACRO.MICRO_VERSION_MACRO],
        [https://github.com/libsdl-org/SDL_image/issues],
        [SDL2_image])
AC_CONFIG_MACRO_DIR([acinclude])
AC_CONFIG_SRCDIR([IMG.c])

AC_SUBST([MAJOR_VERSION], MAJOR_VERSION_MACRO)
AC_SUBST([MINOR_VERSION], MINOR_VERSION_MACRO)
AC_SUBST([MICRO_VERSION], MICRO_VERSION_MACRO)

BINARY_AGE=`expr $MINOR_VERSION \* 100 + $MICRO_VERSION`
AS_CASE(["$MINOR_VERSION"],
  [*@<:@02468@:>@],
    dnl Stable branch, 2.6.1 -> libSDL2-2.0.so.0.600.1
    [INTERFACE_AGE="$MICRO_VERSION"],
  [*],
    dnl Development branch, 2.5.1 -> libSDL2-2.0.so.0.501.0
    [INTERFACE_AGE=0])

dnl libtool versioning
LT_INIT([win32-dll])

# For historical reasons, the library name redundantly includes the major
# version twice: libSDL2_image-2.0.so.0.
# TODO: in SDL 3, stop using -release, which will simplify it to libSDL3.so.0
LT_RELEASE=2.0
# Increment this if there is an incompatible change - but if that happens,
# we should rename the library from SDL2 to SDL3, at which point this would
# reset to 0 anyway.
LT_MAJOR=0
LT_AGE=`expr $BINARY_AGE - $INTERFACE_AGE`
LT_CURRENT=`expr $LT_MAJOR + $LT_AGE`
LT_REVISION=$INTERFACE_AGE
LT_EXTRA=""
m4_pattern_allow([LT_MAJOR])

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_EXTRA)

dnl For use in static assertions
AC_DEFINE_UNQUOTED([SDL_BUILD_MAJOR_VERSION], $MAJOR_VERSION, [ ])
AC_DEFINE_UNQUOTED([SDL_BUILD_MINOR_VERSION], $MINOR_VERSION, [ ])
AC_DEFINE_UNQUOTED([SDL_BUILD_MICRO_VERSION], $MICRO_VERSION, [ ])

dnl Detect the canonical build and host environments
AC_CANONICAL_HOST

dnl Setup for automake
AM_INIT_AUTOMAKE([foreign tar-ustar])

dnl Check for tools
AC_PROG_CC
AC_PROG_OBJC
AC_CHECK_TOOL(RC,[windres],[:])
AC_PROG_INSTALL
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

case "$host" in
    *-*-beos*)
        ac_default_prefix=/boot/develop/tools/gnupro
        ;;
    *-*-cygwin* | *-*-mingw*)
        if test "$build" != "$host"; then # cross-compiling
            # Default cross-compile location
            ac_default_prefix=/usr/local/cross-tools/$host
        else
            # Look for the location of the tools and install there
            if test "$BUILD_PREFIX" != ""; then
                ac_default_prefix=$BUILD_PREFIX
            elif test "$MINGW_PREFIX" != ""; then
                ac_default_prefix=$MINGW_PREFIX
            fi
        fi
        use_version_rc=true
        LT_EXTRA="-Wl,version.o"
        ;;
    *-*-darwin*)
        AC_ARG_ENABLE([imageio], [AS_HELP_STRING([--enable-imageio], [use native Mac OS X frameworks for loading images [default=yes]])],
                      [], [enable_imageio=yes])
        dnl Show a message when we use ImageIO support so it's not a surprise
        AC_MSG_CHECKING([for ImageIO support])
        AC_MSG_RESULT($enable_imageio)
        if test x$enable_imageio = xyes; then
            IMG_LIBS="-Wl,-framework,ApplicationServices -lobjc $IMG_LIBS"
        else
            CFLAGS="$CFLAGS -DSDL_IMAGE_USE_COMMON_BACKEND"
        fi
        ;;
    *-*-os2*)
        # disable static builds on os/2
        enable_static=no
        # -DBUILD_SDL is needed for DECLSPEC
        CFLAGS="$CFLAGS -DBUILD_SDL"
        # OS/2 does not support a DLL name longer than 8 characters.
        LT_EXTRA="-os2dllname SDL2img"
        ;;
esac
AM_CONDITIONAL(USE_IMAGEIO, test x$enable_imageio = xyes)
if test x$enable_imageio = xyes; then
    AC_SUBST([USE_IMAGEIO], 1)
else
    AC_SUBST([USE_IMAGEIO], 0)
fi
AM_CONDITIONAL(USE_VERSION_RC, test x$use_version_rc = xtrue)

dnl set this to use on systems that use lib64 instead of lib
base_bindir=`echo \${bindir} | sed 's/.*\/\(.*\)/\1/; q'`
base_libdir=`echo \${libdir} | sed 's/.*\/\(.*\)/\1/; q'`

dnl Function to find a library in the compiler search path
find_lib()
{
    gcc_bin_path=[`$CC -print-search-dirs 2>/dev/null | fgrep programs: | sed 's/[^=]*=\(.*\)/\1/' | sed 's/:/ /g'`]
    gcc_lib_path=[`$CC -print-search-dirs 2>/dev/null | fgrep libraries: | sed 's/[^=]*=\(.*\)/\1/' | sed 's/:/ /g'`]
    env_lib_path=[`echo $LIBS $LDFLAGS $* | sed 's/-L[ ]*//g'`]
    if test "$cross_compiling" = yes; then
        host_lib_path=""
    else
        host_lib_path="$ac_default_prefix/$base_libdir $ac_default_prefix/$base_bindir /usr/$base_libdir /usr/local/$base_libdir"
    fi
    for path in $env_lib_path $gcc_bin_path $gcc_lib_path $host_lib_path; do
        lib=[`ls -- $path/$1 2>/dev/null | sed -e '/\.so\..*\./d' -e 's,.*/,,' | sort | tail -1`]
        if test x$lib != x; then
            echo $lib
            return
        fi
    done
}

CheckNoUndef()
{
AC_MSG_CHECKING(for linker option --no-undefined)
    have_no_undefined=no
    case "${host_os}" in
dnl Skip this on platforms where it is just simply busted.
    openbsd*) ;;
    darwin*) have_no_undefined="-Wl,-undefined,error"
       LDFLAGS="$LDFLAGS -Wl,-undefined,error" ;;
    *) save_LDFLAGS="$LDFLAGS"
       LDFLAGS="$LDFLAGS -Wl,--no-undefined"
       AC_LINK_IFELSE([AC_LANG_PROGRAM],
          [have_no_undefined=yes],[LDFLAGS="$save_LDFLAGS"])
       ;;
    esac
    AC_MSG_RESULT($have_no_undefined)
}

dnl See if GCC's -Wall is supported.
CheckWarnAll()
{
    AC_MSG_CHECKING(for GCC -Wall option)
    have_gcc_Wall=no

    save_CFLAGS="$CFLAGS"
    CFLAGS="$save_CFLAGS -Wall"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([int x = 0;])], [have_gcc_Wall=yes])
    AC_MSG_RESULT($have_gcc_Wall)
    CFLAGS="$save_CFLAGS"

    if test x$have_gcc_Wall = xyes; then
        CFLAGS="$CFLAGS -Wall"

dnl Haiku headers use multicharacter constants all over the place. Ignore these warnings when using -Wall.
        AC_MSG_CHECKING(for necessary GCC -Wno-multichar option)
        need_gcc_Wno_multichar=no
        case "$host" in
            *-*-haiku*)
                need_gcc_Wno_multichar=yes
                ;;
        esac
        AC_MSG_RESULT($need_gcc_Wno_multichar)
        if test x$need_gcc_Wno_multichar = xyes; then
            CFLAGS="$CFLAGS -Wno-multichar"
        fi
    fi
}

dnl See if GCC's -fvisibility=hidden is supported (gcc4 and later, usually).
CheckVisibilityHidden()
{
    AC_MSG_CHECKING(for GCC -fvisibility=hidden option)
    have_gcc_fvisibility=no
    case "$host" in
    *-*-cygwin* | *-*-mingw* | *-*-os2*)
        AC_MSG_RESULT([ignored for $host_os])
        return
        ;;
    esac

    visibility_CFLAGS="-fvisibility=hidden"
    save_CFLAGS="$CFLAGS"
    CFLAGS="$save_CFLAGS $visibility_CFLAGS -Werror"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #if !defined(__GNUC__) || __GNUC__ < 4
    #error SDL only uses visibility attributes in GCC 4 or newer
    #endif
    ]],[])], [have_gcc_fvisibility=yes],[])
    AC_MSG_RESULT($have_gcc_fvisibility)
    CFLAGS="$save_CFLAGS"

    if test x$have_gcc_fvisibility = xyes; then
        CFLAGS="$CFLAGS $visibility_CFLAGS"
    fi
}

dnl Check command-line options
AC_ARG_ENABLE([stb_image], [AS_HELP_STRING([--enable-stb-image], [use stb_image for loading JPG and PNG images [default=yes]])],
 [], [enable_stb_image=yes])
AC_ARG_ENABLE([avif], [AS_HELP_STRING([--enable-avif], [support loading AVIF images [default=yes]])],
 [], [enable_avif=yes])
AC_ARG_ENABLE([avif-shared], [AS_HELP_STRING([--enable-avif-shared], [dynamically load AVIF support [default=yes]])],
 [], [enable_avif_shared=yes])
AC_ARG_ENABLE([bmp], [AS_HELP_STRING([--enable-bmp], [support loading BMP/ICO/CUR images [default=yes]])],
 [], [enable_bmp=yes])
AC_ARG_ENABLE([gif], [AS_HELP_STRING([--enable-gif], [support loading GIF images [default=yes]])],
 [], [enable_gif=yes])
AC_ARG_ENABLE([jpg], [AS_HELP_STRING([--enable-jpg], [support loading JPG images [default=yes]])],
 [], [enable_jpg=yes])
AC_ARG_ENABLE([jpg-shared], [AS_HELP_STRING([--enable-jpg-shared], [dynamically load JPG support [default=yes]])],
 [], [enable_jpg_shared=yes])
AC_ARG_ENABLE([save-jpg], [AS_HELP_STRING([--enable-save-jpg], [support saving JPG images [default=yes]])],
 [], [enable_save_jpg=yes])
AC_ARG_ENABLE([jxl], [AS_HELP_STRING([--enable-jxl], [support loading JXL images [default=yes]])],
 [], [enable_jxl=yes])
AC_ARG_ENABLE([jxl-shared], [AS_HELP_STRING([--enable-jxl-shared], [dynamically load JXL support [default=yes]])],
 [], [enable_jxl_shared=yes])
AC_ARG_ENABLE([lbm], [AS_HELP_STRING([--enable-lbm], [support loading LBM images [default=yes]])],
 [], [enable_lbm=yes])
AC_ARG_ENABLE([pcx], [AS_HELP_STRING([--enable-pcx], [support loading PCX images [default=yes]])],
 [], [enable_pcx=yes])
AC_ARG_ENABLE([png], [AS_HELP_STRING([--enable-png], [support loading PNG images [default=yes]])],
 [], [enable_png=yes])
AC_ARG_ENABLE([png-shared], [AS_HELP_STRING([--enable-png-shared], [dynamically load PNG support [default=yes]])],
 [], [enable_png_shared=yes])
AC_ARG_ENABLE([save-png], [AS_HELP_STRING([--enable-save-png], [support saving PNG images [default=yes]])],
 [], [enable_save_png=yes])
AC_ARG_ENABLE([pnm], [AS_HELP_STRING([--enable-pnm], [support loading PNM images [default=yes]])],
 [], [enable_pnm=yes])
AC_ARG_ENABLE([svg], [AS_HELP_STRING([--enable-svg], [support loading SVG images [default=yes]])],
 [], [enable_svg=yes])
AC_ARG_ENABLE([tga], [AS_HELP_STRING([--enable-tga], [support loading TGA images [default=yes]])],
 [], [enable_tga=yes])
AC_ARG_ENABLE([tif], [AS_HELP_STRING([--enable-tif], [support loading TIFF images [default=yes]])],
 [], [enable_tif=yes])
AC_ARG_ENABLE([tif-shared], [AS_HELP_STRING([--enable-tif-shared], [dynamically load TIFF support [default=yes]])],
 [], [enable_tif_shared=yes])
AC_ARG_ENABLE([xcf], [AS_HELP_STRING([--enable-xcf], [support loading XCF images [default=yes]])],
 [], [enable_xcf=yes])
AC_ARG_ENABLE([xpm], [AS_HELP_STRING([--enable-xpm], [support loading XPM images [default=yes]])],
 [], [enable_xpm=yes])
AC_ARG_ENABLE([xv], [AS_HELP_STRING([--enable-xv], [support loading XV images [default=yes]])],
 [], [enable_xv=yes])
AC_ARG_ENABLE([webp], [AS_HELP_STRING([--enable-webp], [support loading WEBP images [default=yes]])],
 [], [enable_webp=yes])
AC_ARG_ENABLE([webpdec], [AS_HELP_STRING([--enable-webpdec], [support loading WEBP images via libwebpdecoder instead of libwebp [default=no]])],
 [], [enable_webpdec=no])
AC_ARG_ENABLE([webp-shared], [AS_HELP_STRING([--enable-webp-shared], [dynamically load WEBP support [default=yes]])],
 [], [enable_webp_shared=yes])
AC_ARG_ENABLE([qoi], [AS_HELP_STRING([--enable-qoi], [support loading QOI images [default=yes]])],
 [], [enable_qoi=yes])
AC_ARG_ENABLE([tests],
              [AS_HELP_STRING([--enable-tests], [build tests [default=no]])],
              [], [enable_tests=no])
AC_ARG_ENABLE([installed-tests],
              [AS_HELP_STRING([--enable-installed-tests], [install tests [default=no]])],
              [], [enable_installed_tests=no])

dnl Check for SDL
SDL_VERSION=2.0.9
AC_SUBST(SDL_VERSION)
AM_PATH_SDL2($SDL_VERSION,
            :,
            AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CFLAGS="$CFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"

if test x$enable_stb_image = xyes; then
    AC_DEFINE([USE_STBIMAGE])
    AC_SUBST([USE_STBIMAGE], 1)
else
    AC_SUBST([USE_STBIMAGE], 0)
fi

AC_SUBST([USE_WIC], 0)

load_avif=0
if test x$enable_avif = xyes; then
    PKG_CHECK_MODULES([LIBAVIF], [libavif], [dnl
        have_avif_hdr=yes
        have_avif_lib=yes
        have_avif_pc=yes
      ], [dnl
        AC_CHECK_HEADER([avif/avif.h], [
            have_avif_hdr=yes
            LIBAVIF_CFLAGS=""
        ])
        AC_CHECK_LIB([avif], [avifVersion], [
            have_avif_lib=yes
            LIBAVIF_LIBS="-lavif"
        ])
      ])
    if test x$have_avif_hdr = xyes -a x$have_avif_lib = xyes; then
        if test x$enable_avif = xyes; then
            AC_DEFINE([LOAD_AVIF])
        fi

        case "$host" in
            *-*-darwin*)
                avif_lib=[`find_lib libavif.dylib`]
                ;;
            *-*-cygwin* | *-*-mingw*)
                avif_lib=[`find_lib "libavif*.dll"`]
                ;;
            *)
                avif_lib=[`find_lib "libavif[0-9]*.so.*"`]
                if test x$avif_lib = x; then
                    avif_lib=[`find_lib "libavif.so.*"`]
                fi
                ;;
        esac
        load_avif=1
    elif test x$enable_avif = xyes; then
        AC_MSG_WARN([*** Unable to find AVIF library (https://github.com/AOMediaCodec/libavif)])
        AC_MSG_WARN([AVIF image loading disabled])
    fi
fi
AC_SUBST([LOAD_AVIF], $load_avif)

load_jpg=0
if test x$enable_jpg = xyes; then
    if test x$enable_stb_image = xyes; then
        AC_DEFINE([LOAD_JPG])
        load_jpg=1
    elif test x$enable_imageio = xyes; then
        AC_DEFINE([LOAD_JPG])
        load_jpg=1
        AC_DEFINE([JPG_USES_IMAGEIO])
    else
        PKG_CHECK_MODULES([LIBJPEG], [libjpeg], [dnl
            have_jpg_hdr=yes
            have_jpg_lib=yes
            have_jpg_pc=yes
          ], [dnl
            AC_CHECK_HEADER([jpeglib.h], [
                have_jpg_hdr=yes
                LIBJPEG_CFLAGS=""
            ])
            AC_CHECK_LIB([jpeg], [jpeg_CreateDecompress], [
                have_jpg_lib=yes
                LIBJPEG_LIBS="-ljpeg"
            ])
          ])
        if test x$have_jpg_hdr = xyes -a x$have_jpg_lib = xyes; then
            if test x$enable_jpg = xyes; then
                AC_DEFINE([LOAD_JPG])
                load_jpg=1
            fi

            case "$host" in
                *-*-darwin*)
                    jpg_lib=[`find_lib libjpeg.dylib`]
                    ;;
                *-*-cygwin* | *-*-mingw*)
                    jpg_lib=[`find_lib "libjpeg*.dll"`]
                    ;;
                *)
                    jpg_lib=[`find_lib "libjpeg[0-9]*.so.*"`]
                    if test x$jpg_lib = x; then
                        jpg_lib=[`find_lib "libjpeg.so.*"`]
                    fi
                    ;;
            esac
        else
            AC_MSG_WARN([*** Unable to find JPEG library (http://www.ijg.org/)])
            AC_MSG_WARN([JPG image loading disabled])
        fi
    fi
fi
AC_SUBST([LOAD_JPG], $load_jpg)

load_jxl=0
if test x$enable_jxl = xyes; then
    PKG_CHECK_MODULES([LIBJXL], [libjxl], [dnl
        have_jxl_hdr=yes
        have_jxl_lib=yes
        have_jxl_pc=yes
      ], [dnl
        AC_CHECK_HEADER([jxl/decode.h], [
            have_jxl_hdr=yes
            LIBJXL_CFLAGS=""
        ])
        AC_CHECK_LIB([jxl], [JxlSignatureCheck], [
            have_jxl_lib=yes
            LIBJXL_LIBS="-ljxl"
        ])
      ])
    if test x$have_jxl_hdr = xyes -a x$have_jxl_lib = xyes; then
        if test x$enable_jxl = xyes; then
            AC_DEFINE([LOAD_JXL])
            load_jxl=1
        fi

        case "$host" in
            *-*-darwin*)
                jxl_lib=[`find_lib libjxl.dylib`]
                ;;
            *-*-cygwin* | *-*-mingw*)
                jxl_lib=[`find_lib "libjxl*.dll"`]
                ;;
            *)
                jxl_lib=[`find_lib "libjxl[0-9]*.so.*"`]
                if test x$jxl_lib = x; then
                    jxl_lib=[`find_lib "libjxl.so.*"`]
                fi
                ;;
        esac
    elif test x$enable_jxl = xyes; then
        AC_MSG_WARN([*** Unable to find JXL library (https://jpegxl.info/)])
        AC_MSG_WARN([JXL image loading disabled])
    fi
fi
AC_SUBST([LOAD_JXL], $load_jxl)

load_png=0
if test x$enable_png = xyes; then
    if test x$enable_stb_image = xyes; then
        AC_DEFINE([LOAD_PNG])
        load_png=1
    elif test x$enable_imageio = xyes; then
        AC_DEFINE([LOAD_PNG])
        AC_DEFINE([PNG_USES_IMAGEIO])
        load_png=1
    else
        PKG_CHECK_MODULES([LIBPNG], [libpng], [dnl
            have_png_hdr=yes
            have_png_lib=yes
            have_png_pc=yes
          ], [dnl
            AC_CHECK_HEADER([png.h], [
                have_png_hdr=yes
                LIBPNG_CFLAGS=""
            ])
            AC_CHECK_LIB([png], [png_create_read_struct], [
                have_png_lib=yes
                LIBPNG_LIBS="-lpng -lz"
            ], [], [-lz])
          ])
        if test x$have_png_hdr = xyes -a x$have_png_lib = xyes; then
            AC_DEFINE([LOAD_PNG])
            load_png=1

            case "$host" in
                *-*-darwin*)
                    png_lib=[`find_lib libpng.dylib`]
                    ;;
                *-*-cygwin* | *-*-mingw*)
                    png_lib=[`find_lib "libpng*.dll"`]
                    ;;
                *)
                    png_lib=[`find_lib "libpng[0-9]*.so.*"`]
                    if test x$png_lib = x; then
                        png_lib=[`find_lib "libpng.so.*"`]
                    fi
                    ;;
            esac
        else
            AC_MSG_WARN([*** Unable to find PNG library (http://www.libpng.org/pub/png/)])
            AC_MSG_WARN([PNG image loading disabled])
        fi
    fi
fi
AC_SUBST([LOAD_PNG], $load_png)

load_tif=0
if test x$enable_tif = xyes -a x$enable_imageio != xyes; then
    PKG_CHECK_MODULES([LIBTIFF], [libtiff-4], [dnl
        have_tif_hdr=yes
        have_tif_lib=yes
        have_tif_pc=yes
      ], [dnl
        AC_CHECK_HEADER([tiffio.h], [
            have_tif_hdr=yes
            LIBTIFF_CFLAGS=""
        ])
        AC_CHECK_LIB([tiff], [TIFFClientOpen], [
            have_tif_lib=yes
            LIBTIFF_LIBS="-ltiff -lz"
        ], [], [-lz])
      ])
    if test x$have_tif_hdr = xyes -a x$have_tif_lib = xyes; then
        AC_DEFINE([LOAD_TIF])
        load_tif=1

        case "$host" in
            *-*-darwin*)
                tif_lib=[`find_lib libtiff.dylib`]
                ;;
            *-*-cygwin* | *-*-mingw*)
                tif_lib=[`find_lib "libtiff-*.dll"`]
                ;;
            *)
                tif_lib=[`find_lib "libtiff[0-9]*.so.*"`]
                if test x$tif_lib = x; then
                    tif_lib=[`find_lib "libtiff.so.*"`]
                fi
                ;;
        esac
    else
        AC_MSG_WARN([*** Unable to find Tiff library (http://www.simplesystems.org/libtiff/)])
        AC_MSG_WARN([TIF image loading disabled])
    fi
fi
AC_SUBST([LOAD_TIF], $load_tif)

load_webp=0
if test x$enable_webpdec = xyes; then
  webplib=webpdecoder
else
  webplib=webp
fi
if test x$enable_webp = xyes; then
    PKG_CHECK_MODULES([LIBWEBP], [lib$webplib], [dnl
        have_webp_hdr=yes
        have_webp_lib=yes
        have_webp_pc=yes
      ], [dnl
        AC_CHECK_HEADER([webp/decode.h], [
            have_webp_hdr=yes
            LIBWEBP_CFLAGS=""
        ])
        AC_CHECK_LIB([$webplib], [WebPGetDecoderVersion], [
            have_webp_lib=yes
            LIBWEBP_LIBS="-l$webplib"
        ], [], [-lm])
      ])
    if test x$have_webp_hdr = xyes -a x$have_webp_lib = xyes; then
        AC_DEFINE([LOAD_WEBP])
        load_webp=1

        case "$host" in
            *-*-darwin*)
                webp_lib=[`find_lib lib$webplib.dylib`]
                ;;
            *-*-cygwin* | *-*-mingw*)
                webp_lib=[`find_lib "lib$webplib-*.dll"`]
                ;;
            *)
                webp_lib=[`find_lib "lib$webplib[0-9]*.so.*"`]
                if test x$webp_lib = x; then
                    webp_lib=[`find_lib "lib$webplib.so.*"`]
                fi
                ;;
        esac
    else
        AC_MSG_WARN([*** Unable to find WEBP library (https://developers.google.com/speed/webp)])
        AC_MSG_WARN([WEBP image loading disabled])
    fi
fi
AC_SUBST([LOAD_WEBP], $load_webp)

load_bmp=0
if test x$enable_bmp = xyes; then
    AC_DEFINE([LOAD_BMP])
    load_bmp=1
fi
AC_SUBST([LOAD_BMP], $load_bmp)

load_gif=0
if test x$enable_gif = xyes; then
    AC_DEFINE([LOAD_GIF])
    load_gif=1
fi
AC_SUBST([LOAD_GIF], $load_gif)

load_lbm=0
if test x$enable_lbm = xyes; then
    AC_DEFINE([LOAD_LBM])
    load_lbm=1
fi
AC_SUBST([LOAD_LBM], $load_lbm)

load_pcx=0
if test x$enable_pcx = xyes; then
    AC_DEFINE([LOAD_PCX])
    load_pcx=1
fi
AC_SUBST([LOAD_PCX], $load_pcx)

load_pnm=0
if test x$enable_pnm = xyes; then
    AC_DEFINE([LOAD_PNM])
    load_pnm=1
fi
AC_SUBST([LOAD_PNM], $load_pnm)

load_svg=0
if test x$enable_svg = xyes; then
    AC_DEFINE([LOAD_SVG])
    load_svg=1
fi
AC_SUBST([LOAD_SVG], $load_svg)

load_tga=0
if test x$enable_tga = xyes; then
    AC_DEFINE([LOAD_TGA])
    load_tga=1
fi
AC_SUBST([LOAD_TGA], $load_tga)

load_xcf=0
if test x$enable_xcf = xyes; then
    AC_DEFINE([LOAD_XCF])
    load_xcf=1
fi
AC_SUBST([LOAD_XCF], $load_xcf)

load_xpm=0
if test x$enable_xpm = xyes; then
    AC_DEFINE([LOAD_XPM])
    load_xpm=1
fi
AC_SUBST([LOAD_XPM], $load_xpm)

load_xv=0
if test x$enable_xv = xyes; then
    AC_DEFINE([LOAD_XV])
    load_xv=1
fi
AC_SUBST([LOAD_XV], $load_xv)

load_qoi=0
if test x$enable_qoi = xyes; then
    AC_DEFINE([LOAD_QOI])
    load_qoi=1
fi
AC_SUBST([LOAD_QOI], $load_qoi)

if test x$enable_webp = xyes -a x$have_webp_hdr = xyes -a x$have_webp_lib = xyes; then
    CFLAGS="$LIBWEBP_CFLAGS $CFLAGS"
    if test x$enable_webp_shared = xyes && test x$webp_lib != x; then
        echo "-- dynamic libwebp -> $webp_lib"
        AC_DEFINE_UNQUOTED(LOAD_WEBP_DYNAMIC, "$webp_lib")
    else
        IMG_LIBS="$LIBWEBP_LIBS $IMG_LIBS"
        if test x$have_webp_pc = xyes; then
            PC_REQUIRES="lib$webplib $PC_REQUIRES"
        else
            PC_LIBS="$LIBWEBP_LIBS $PC_LIBS"
        fi
    fi
fi

if test x$enable_avif = xyes -a x$have_avif_hdr = xyes -a x$have_avif_lib = xyes; then
    CFLAGS="$LIBAVIF_CFLAGS $CFLAGS"
    if test x$enable_avif_shared = xyes && test x$avif_lib != x; then
        echo "-- dynamic libavif -> $avif_lib"
        AC_DEFINE_UNQUOTED(LOAD_AVIF_DYNAMIC, "$avif_lib")
    else
        IMG_LIBS="$LIBAVIF_LIBS $IMG_LIBS"
        if test x$have_avif_pc = xyes; then
            PC_REQUIRES="libavif $PC_REQUIRES"
        else
            PC_LIBS="$LIBAVIF_LIBS $PC_LIBS"
        fi
    fi
fi
if test x$enable_tif = xyes -a x$have_tif_hdr = xyes -a x$have_tif_lib = xyes; then
    CFLAGS="$LIBTIFF_CFLAGS $CFLAGS"
    if test x$enable_tif_shared = xyes && test x$tif_lib != x; then
        echo "-- dynamic libtiff -> $tif_lib"
        AC_DEFINE_UNQUOTED(LOAD_TIF_DYNAMIC, "$tif_lib")
    else
        if test x$have_libjpeg = xyes; then
            # Disable dynamic jpeg since we're linking it explicitly
            jpg_lib=''
        fi
        IMG_LIBS="$LIBTIFF_LIBS $IMG_LIBS"
        if test x$have_tif_pc = xyes; then
            PC_REQUIRES="libtiff-4 $PC_REQUIRES"
        else
            PC_LIBS="$LIBTIFF_LIBS $PC_LIBS"
        fi
    fi
fi
if test x$enable_jpg = xyes -a x$have_jpg_hdr = xyes -a x$have_jpg_lib = xyes; then
    CFLAGS="$LIBJPEG_CFLAGS $CFLAGS"
    if test x$enable_jpg_shared = xyes && test x$jpg_lib != x; then
        echo "-- dynamic libjpeg -> $jpg_lib"
        AC_DEFINE_UNQUOTED(LOAD_JPG_DYNAMIC, "$jpg_lib")
    else
        IMG_LIBS="$LIBJPEG_LIBS $IMG_LIBS"
        if test x$have_jpg_pc = xyes; then
            PC_REQUIRES="libjpeg $PC_REQUIRES"
        else
            PC_LIBS="$LIBJPEG_LIBS $PC_LIBS"
        fi
    fi
fi
if test x$enable_jxl = xyes -a x$have_jxl_hdr = xyes -a x$have_jxl_lib = xyes; then
    CFLAGS="$LIBJXL_CFLAGS $CFLAGS"
    if test x$enable_jxl_shared = xyes && test x$jxl_lib != x; then
        echo "-- dynamic libjxl -> $jxl_lib"
        AC_DEFINE_UNQUOTED(LOAD_JXL_DYNAMIC, "$jxl_lib")
    else
        IMG_LIBS="$LIBJXL_LIBS $IMG_LIBS"
        if test x$have_jxl_pc = xyes; then
            PC_REQUIRES="libjxl $PC_REQUIRES"
        else
            PC_LIBS="$LIBJXL_LIBS $PC_LIBS"
        fi
    fi
fi
if test x$enable_png = xyes -a x$have_png_hdr = xyes -a x$have_png_lib = xyes; then
    CFLAGS="$LIBPNG_CFLAGS $CFLAGS"
    if test x$enable_png_shared = xyes && test x$png_lib != x; then
        echo "-- dynamic libpng -> $png_lib"
        AC_DEFINE_UNQUOTED(LOAD_PNG_DYNAMIC, "$png_lib")
    else
        IMG_LIBS="$LIBPNG_LIBS $IMG_LIBS"
        if test x$have_png_pc = xyes; then
            PC_REQUIRES="libpng $PC_REQUIRES"
        else
            PC_LIBS="$LIBPNG_LIBS $PC_LIBS"
        fi
    fi
fi
if test x$enable_save_png = xyes; then
    AC_DEFINE([SDL_IMAGE_SAVE_PNG], 1)
    AC_SUBST([SDL2IMAGE_PNG_SAVE], 1)
else
    AC_DEFINE([SDL_IMAGE_SAVE_PNG], 0)
    AC_SUBST([SDL2IMAGE_PNG_SAVE], 0)
fi
if test x$enable_save_jpg = xyes; then
    AC_DEFINE([SDL_IMAGE_SAVE_JPG], 1)
    AC_SUBST([SDL2IMAGE_JPG_SAVE], 1)
else
    AC_DEFINE([SDL_IMAGE_SAVE_JPG], 0)
    AC_SUBST([SDL2IMAGE_JPG_SAVE], 0)
fi

AC_SUBST([IMG_LIBS])
AC_SUBST([PC_LIBS])
AC_SUBST([PC_REQUIRES])

AM_CONDITIONAL([BUILD_TESTS], [test "x$enable_tests" = xyes])
AM_CONDITIONAL([INSTALL_TESTS], [test "x$enable_installed_tests" = xyes])

dnl Calculate the location of the prefix, relative to the cmake folder
pkg_cmakedir='$libdir/cmake/SDL2_image'
AX_COMPUTE_RELATIVE_PATHS([pkg_cmakedir:prefix:cmake_prefix_relpath])
AC_SUBST([cmake_prefix_relpath])

dnl check for LD --no-undefined option
CheckNoUndef

dnl check for GCC warning options
CheckWarnAll

dnl check for GCC visibility attributes
CheckVisibilityHidden

OBJCFLAGS=$CFLAGS

# Finally create all the generated files
AC_CONFIG_FILES([
Makefile
sdl2_image-config.cmake
sdl2_image-config-version.cmake
SDL2_image.spec
SDL2_image.pc
test/Makefile
])
AC_OUTPUT
