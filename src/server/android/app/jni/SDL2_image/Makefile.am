# Makefile.am for the SDL sample image loading library and viewer
ACLOCAL_AMFLAGS = -I acinclude

lib_LTLIBRARIES = libSDL2_image.la

libSDL2_imageincludedir = $(includedir)/SDL2
libSDL2_imageinclude_HEADERS =	\
	SDL_image.h

if USE_IMAGEIO
IMAGEIO_SOURCE = IMG_ImageIO.m
endif

libSDL2_image_la_SOURCES =	\
	IMG.c			\
	IMG_avif.c		\
	IMG_bmp.c		\
	IMG_gif.c		\
	IMG_jpg.c		\
	IMG_jxl.c		\
	IMG_lbm.c		\
	IMG_pcx.c		\
	IMG_png.c		\
	IMG_pnm.c		\
	IMG_qoi.c		\
	IMG_svg.c		\
	IMG_stb.c		\
	IMG_tga.c		\
	IMG_tif.c		\
	IMG_xcf.c		\
	IMG_xpm.c		\
	IMG_xv.c		\
	IMG_webp.c		\
	IMG_WIC.c		\
	$(IMAGEIO_SOURCE)	\
	miniz.h			\
	nanosvg.h		\
	nanosvgrast.h		\
	qoi.h			\
	stb_image.h		\
	tiny_jpeg.h

EXTRA_DIST = \
	.gitmodules		\
	Android.mk		\
	CHANGES.txt 		\
	CMakeLists.txt		\
	IMG_ImageIO.m		\
	IMG_xxx.c		\
	LICENSE.txt 		\
	Makefile.os2		\
	README-emscripten.txt 	\
	README.txt 		\
	SDL2_image.spec		\
	SDL2_imageConfig.cmake.in \
	VisualC			\
	VisualC-WinRT		\
	Xcode			\
	autogen.sh		\
	cmake			\
	external		\
	mingw			\
	version.rc


libSDL2_image_la_LDFLAGS = 	\
	-no-undefined		\
	-release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) $(LT_EXTRA)
libSDL2_image_la_LIBADD = $(IMG_LIBS)
if USE_VERSION_RC
libSDL2_image_la_DEPENDENCIES = version.o
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = SDL2_image.pc

.rc.o:
	$(RC) $< $@

noinst_PROGRAMS = showimage showanim

showimage_LDADD = libSDL2_image.la
showanim_LDADD = libSDL2_image.la

SUBDIRS = .

if BUILD_TESTS
SUBDIRS += test
endif

# Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: distcheck

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz

dist-hook:
	ls $(distdir)/external | fgrep -v download.sh | while read dir; do rm -rf "$(distdir)/external/$$dir"; done

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(libdir)/cmake/SDL2_image
	$(INSTALL) -m 644 sdl2_image-config.cmake $(DESTDIR)$(libdir)/cmake/SDL2_image
	$(INSTALL) -m 644 sdl2_image-config-version.cmake $(DESTDIR)$(libdir)/cmake/SDL2_image

uninstall-hook:
	rm $(DESTDIR)$(libdir)/cmake/SDL2_image/sdl2_image-config.cmake
	rm $(DESTDIR)$(libdir)/cmake/SDL2_image/sdl2_image-config-version.cmake
	rm -r $(DESTDIR)$(libdir)/cmake/SDL2_image
