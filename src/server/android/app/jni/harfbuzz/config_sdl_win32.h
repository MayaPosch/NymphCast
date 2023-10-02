/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* The normal alignment of `struct{char;}', in bytes. */
#define ALIGNOF_STRUCT_CHAR__ 1

/* Define to 1 if you have the `atexit' function. */
#define HAVE_ATEXIT 1

/* Have cairo graphics library */
#define HAVE_CAIRO 1

/* Have cairo-ft support in cairo graphics library */
/* #undef HAVE_CAIRO_FT */

/* Define to 1 if you have the
   `cairo_user_font_face_set_render_color_glyph_func' function. */
/* #undef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC */

/* Have chafa terminal graphics library */
/* #undef HAVE_CHAFA */

/* Have Core Text backend */
/* #undef HAVE_CORETEXT */

/* define if the compiler supports basic C++11 syntax */
/* #define HAVE_CXX11 1 */

/* Have DirectWrite library */
/* #undef HAVE_DIRECTWRITE */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <dwrite_1.h> header file. */
/* #undef HAVE_DWRITE_1_H */

/* Have FreeType 2 library */
#define HAVE_FREETYPE 1

/* Define to 1 if you have the `FT_Done_MM_Var' function. */
#define HAVE_FT_DONE_MM_VAR 1

/* Define to 1 if you have the `FT_Get_Transform' function. */
#define HAVE_FT_GET_TRANSFORM 1

/* Define to 1 if you have the `FT_Get_Var_Blend_Coordinates' function. */
#define HAVE_FT_GET_VAR_BLEND_COORDINATES 1

/* Define to 1 if you have the `FT_Set_Var_Blend_Coordinates' function. */
#define HAVE_FT_SET_VAR_BLEND_COORDINATES 1

/* Have GDI library */
/* #undef HAVE_GDI */

/* Define to 1 if you have the `getpagesize' function. */
/* #undef HAVE_GETPAGESIZE */

/* Have glib2 library */
/* #undef HAVE_GLIB */

/* Have gobject2 library */
/* #undef HAVE_GOBJECT */

/* Have Graphite2 library */
/* #undef HAVE_GRAPHITE2 */

/* Have ICU library */
/* #undef HAVE_ICU */

/* Use hb-icu Unicode callbacks */
/* #undef HAVE_ICU_BUILTIN */

/* Define to 1 if you have the <inttypes.h> header file. */
#if defined(__MINGW32__)
#define HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the `isatty' function. */
/* #undef HAVE_ISATTY */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define to 1 if you have the `mprotect' function. */
/* #undef HAVE_MPROTECT */

/* Define to 1 if you have the `newlocale' function. */
/* #undef HAVE_NEWLOCALE */

/* Have POSIX threads */
/* #undef HAVE_PTHREAD */

/* Have PTHREAD_PRIO_INHERIT. */
/* #undef HAVE_PTHREAD_PRIO_INHERIT */

/* Define to 1 if you have the `sincosf' function. */
/* #undef HAVE_SINCOSF */

/* Define to 1 if you have the <stdbool.h> header file. */
#ifdef __MINGW32__ /**/
#define HAVE_STDBOOL_H 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#if defined(__MINGW32__) || (defined _MSC_VER && _MSC_VER >= 1600)
#define HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `sysconf' function. */
/* #undef HAVE_SYSCONF */

/* Define to 1 if you have the <sys/mman.h> header file. */
/* #undef HAVE_SYS_MMAN_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Have Uniscribe library */
#define HAVE_UNISCRIBE 1

/* Define to 1 if you have the <unistd.h> header file. */
#ifdef __MINGW32__
#define HAVE_UNISTD_H 1
#endif

/* Define to 1 if you have the `uselocale' function. */
/* #undef HAVE_USELOCALE */

/* Define to 1 if you have the <usp10.h> header file. */
#define HAVE_USP10_H 1

/* Have wasm-micro-runtime library */
/* #undef HAVE_WASM */

/* Define to 1 if you have the <wasm_export.h> header file. */
/* #undef HAVE_WASM_EXPORT_H */

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

/* Define to 1 if you have the <xlocale.h> header file. */
/* #undef HAVE_XLOCALE_H */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
/* #define LT_OBJDIR ".libs/" */

/* Define to the address where bug reports for this package should be sent. */
/* #define PACKAGE_BUGREPORT "https://github.com/harfbuzz/harfbuzz/issues/new" */

/* Define to the full name of this package. */
/* #define PACKAGE_NAME "HarfBuzz" */

/* Define to the full name and version of this package. */
/* #define PACKAGE_STRING "HarfBuzz 8.1.1" */

/* Define to the one symbol short name of this package. */
/* #define PACKAGE_TARNAME "harfbuzz" */

/* Define to the home page for this package. */
/* #define PACKAGE_URL "http://harfbuzz.org/" */

/* Define to the version of this package. */
/* #define PACKAGE_VERSION "8.0.1" */

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */
