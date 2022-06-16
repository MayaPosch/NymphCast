# Makefile for the NymphCast Android build.
#
#
# (c) 2022 Nyanko.ws

export TOP := $(CURDIR)

ifndef ANDROID_ABI_LEVEL
ANDROID_ABI_LEVEL := 24
endif

ifdef ANDROID
TOOLCHAIN_PREFIX := arm-linux-androideabi-
ARCH := android-armv7/
ifdef OS
TOOLCHAIN_POSTFIX := .cmd
endif
else ifdef ANDROID64
TOOLCHAIN_PREFIX := aarch64-linux-android-
ARCH := android-aarch64/
ifdef OS
TOOLCHAIN_POSTFIX := .cmd
endif
else ifdef ANDROIDX86
TOOLCHAIN_PREFIX := i686-linux-android-
ARCH := android-i686/
ifdef OS
TOOLCHAIN_POSTFIX := .cmd
endif
else ifdef ANDROIDX64
TOOLCHAIN_PREFIX := x86_64-linux-android-
ARCH := android-x86_64/
ifdef OS
TOOLCHAIN_POSTFIX := .cmd
endif
endif

ifndef ARCH
ARCH := $(shell g++ -dumpmachine)/
endif

USYS := $(shell uname -s)
UMCH := $(shell uname -m)


ifdef ANDROID
#GCC := $(TOOLCHAIN_PREFIX)g++$(TOOLCHAIN_POSTFIX)
GCC := armv7a-linux-androideabi$(ANDROID_ABI_LEVEL)-clang++$(TOOLCHAIN_POSTFIX)
CC := armv7a-linux-androideabi$(ANDROID_ABI_LEVEL)-clang$(TOOLCHAIN_POSTFIX)
MAKEDIR = mkdir -p
RM = rm
AR = $(TOOLCHAIN_PREFIX)ar
STRIP = $(TOOLCHAIN_PREFIX)strip
else ifdef ANDROID64
GCC := aarch64-linux-android$(ANDROID_ABI_LEVEL)-clang++$(TOOLCHAIN_POSTFIX)
CC := aarch64-linux-android$(ANDROID_ABI_LEVEL)-clang$(TOOLCHAIN_POSTFIX)
MAKEDIR = mkdir -p
RM = rm
AR = $(TOOLCHAIN_PREFIX)ar
STRIP = $(TOOLCHAIN_PREFIX)strip
else ifdef ANDROIDX86
GCC := i686-linux-android$(ANDROID_ABI_LEVEL)-clang++$(TOOLCHAIN_POSTFIX)
CC := i686-linux-android$(ANDROID_ABI_LEVEL)-clang$(TOOLCHAIN_POSTFIX)
MAKEDIR = mkdir -p
RM = rm
AR = $(TOOLCHAIN_PREFIX)ar
STRIP = $(TOOLCHAIN_PREFIX)strip
else ifdef ANDROIDX64
GCC := x86_64-linux-android$(ANDROID_ABI_LEVEL)-clang++$(TOOLCHAIN_POSTFIX)
CC := x86_64-linux-android$(ANDROID_ABI_LEVEL)-clang$(TOOLCHAIN_POSTFIX)
MAKEDIR = mkdir -p
RM = rm
AR = $(TOOLCHAIN_PREFIX)ar
STRIP = $(TOOLCHAIN_PREFIX)strip
else ifdef WASM
GCC = emc++
CC = em
MAKEDIR = mkdir -p
RM = rm
AR = ar 
STRIP = strip
else
GCC = g++
CC = gcc
MAKEDIR = mkdir -p
RM = rm
AR = ar
STRIP = strip
endif


TARGET := $(ARCH)
GPP := $(GCC)
export TARGET
export GPP
#export PLATFORM_FLAGS


OUTPUT = libnymphcastserver
VERSION = 0.1


# Use -soname on Linux/BSD, -install_name on Darwin (MacOS).
SONAME = -soname
#LIBNAME = $(OUTPUT).so.$(VERSION)
LIBNAME = $(OUTPUT).so
ifeq ($(shell uname -s),Darwin)
	SONAME = -install_name
	LIBNAME = $(OUTPUT).0.dylib
endif


# Include the file with the versioning information ('VERSION' variable).
include version.mk
VERSIONINFO = -D__VERSION="\"$(VERSION)\""


INCLUDE = -I . -I ffplay -I angelscript/angelscript/include -I angelscript/add_on \
			-I angelscript/json -I angelscript/regexp \
			-I gui/core -I gui/core/animations  -I gui/core/components  -I gui/core/guis \
			-I gui/core/math  -I gui/core/renderers  -I gui/core/resources  -I gui/core/utils \
			-I gui/app -I gui/app/animations -I gui/app/components -I gui/app/guis \
			-I gui/app/scrapers -I gui/app/views -I gui/app/gamelist \
			-I gui/core/nanosvg \
			-I$(NDK_HOME)/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/SDL2 \
			-I$(NDK_HOME)/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/freetype2 \
			-I$(NDK_HOME)/sources/android/native_app_glue
LIBS := -lnymphrpc -lPocoUtil -lPocoNet -lPocoNetSSL -lPocoJSON -lPocoData -lPocoDataSQLite \
		-lPocoFoundation -lswscale -lavcodec -lavdevice -lavformat -lavutil \
		-lswresample -lavfilter -lSDL2_image -Langelscript/angelscript/lib-$(TARGET) -langelscript \
		-lcurl -lfreeimage \
		-lSDL2main -lSDL2 \
		-lnymphcast -lPocoNet -lPocoUtil -lPocoFoundation
		# -lstdc++fs -lfreetype \ -> FreeType is in the FfmpegKit binaries.
		# -lpostproc
FLAGS := -Dmain=SDL_main -ffunction-sections -fdata-sections -g3 -O1
CFLAGS := $(FLAGS) $(INCLUDE) -g3 -std=c11
CPPFLAGS := $(FLAGS) $(INCLUDE) -std=c++17 $(VERSIONINFO)
SHARED_FLAGS := -fPIC -shared -Wl,$(SONAME),$(LIBNAME)
LDFLAGS := -Wl,--gc-sections -Wl,-Map,bin/shared/$(ARCH)/$(OUTPUT).map $(PLATFORM_LDFLAGS) $(LIB)

ifdef ANDROID
CFLAGS += -fPIC
else ifdef ANDROID64
#CFLAGS += -fPIC
else ifdef ANDROIDX86
CFLAGS += -fPIC
else ifdef ANDROIDX64
#CFLAGS += -fPIC
endif

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
ifndef ANDROID
ifndef ANDROID64
ifndef ANDROIDX86
ifndef ANDROIDX64
	CFLAGS := $(CFLAGS) -U__STRICT_ANSI__ -DPOCO_WIN32_UTF8
	LIBS += -lws2_32
endif
endif
endif
endif
else
	LIBS += -pthread
	UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        MAKE := gmake
    endif
endif

# Check which version of OpenGL or OpenGL ES to use.
# TODO: implement
CPPFLAGS += -DUSE_OPENGL_14

SOURCES := $(wildcard *.cpp) \
			$(wildcard ffplay/*.cpp) \
			$(wildcard angelscript/add_on/scriptstdstring/*.cpp) \
			$(wildcard angelscript/add_on/scriptbuilder/*.cpp) \
			$(wildcard angelscript/add_on/scriptarray/*.cpp) \
			$(wildcard angelscript/add_on/scriptdictionary/*.cpp) \
			$(wildcard angelscript/json/*.cpp) \
			$(wildcard angelscript/regexp/*.cpp) \
			$(wildcard lcdapi/api/*.cpp) \
			$(wildcard lcdapi/sensors/*.cpp)
C_SOURCES := $(NDK_HOME)/sources/android/native_app_glue/android_native_app_glue.c
GUI_SOURCES := 	$(wildcard gui/core/*.cpp) \
				$(wildcard gui/core/animations/*.cpp) \
				$(wildcard gui/core/components/*.cpp) \
				$(wildcard gui/core/guis/*.cpp) \
				$(wildcard gui/core/math/*.cpp) \
				$(wildcard gui/core/nanosvg/src/*.cpp) \
				$(wildcard gui/core/renderers/*.cpp) \
				$(wildcard gui/core/resources/*.cpp) \
				$(wildcard gui/core/utils/*.cpp) \
				$(wildcard gui/app/*.cpp) \
				$(wildcard gui/app/components/*.cpp) \
				$(wildcard gui/app/guis/*.cpp) \
				$(wildcard gui/app/scrapers/*.cpp) \
				$(wildcard gui/app/views/*.cpp) \
				$(wildcard gui/app/views/gamelist/*.cpp) \
				$(wildcard gui/app/pugixml/src/*.cpp)
OBJECTS := 		$(addprefix obj/shared/$(ARCH),$(notdir) $(SOURCES:.cpp=.o))
GUI_OBJECTS := 	$(addprefix obj/shared/$(ARCH),$(notdir) $(GUI_SOURCES:.cpp=.o))
C_OBJECTS := 	$(addprefix obj/shared/$(ARCH)sources/android/native_app_glue/,$(notdir $(C_SOURCES:.c=.o)))

all: lib

lib: makedir android_glue lib/$(ARCH)$(LIBNAME)
	
obj/static/$(ARCH)%.o: %.cpp
	$(GCC) -c -o $@ $< $(CPPFLAGS)
	
# %.c:
	# $(CC) -c -o $@ obj/shared/$(ARCH)sources/android/native_app_glue/%.o $(CFLAGS)

android_glue:
	$(CC) -c -o obj/shared/$(ARCH)sources/android/native_app_glue/android_native_app_glue.o $(NDK_HOME)/sources/android/native_app_glue/android_native_app_glue.c $(CFLAGS)
	
obj/shared/$(ARCH)%.o: %.cpp
	$(GCC) -c -o $@ $< $(SHARED_FLAGS) $(CPPFLAGS)
	
lib/$(ARCH)$(OUTPUT).a: $(OBJECTS) $(GUI_OBJECTS)
	-rm -f $@
	$(AR) rcs $@ $^
	
makedir:
	$(MAKEDIR) lib/$(ARCH)
	#$(MAKEDIR) obj/static/$(ARCH)src
	$(MAKEDIR) obj/shared/$(ARCH)ffplay
	$(MAKEDIR) obj/shared/$(ARCH)angelscript/add_on/scriptstdstring
	$(MAKEDIR) obj/shared/$(ARCH)angelscript/add_on/scriptbuilder
	$(MAKEDIR) obj/shared/$(ARCH)angelscript/add_on/scriptarray
	$(MAKEDIR) obj/shared/$(ARCH)angelscript/add_on/scriptdictionary
	$(MAKEDIR) obj/shared/$(ARCH)angelscript/json
	$(MAKEDIR) obj/shared/$(ARCH)angelscript/regexp
	$(MAKEDIR) obj/shared/$(ARCH)gui/core/animations
	$(MAKEDIR) obj/shared/$(ARCH)gui/core/components
	$(MAKEDIR) obj/shared/$(ARCH)gui/core/guis
	$(MAKEDIR) obj/shared/$(ARCH)gui/core/math
	$(MAKEDIR) obj/shared/$(ARCH)gui/core/renderers
	$(MAKEDIR) obj/shared/$(ARCH)gui/core/resources
	$(MAKEDIR) obj/shared/$(ARCH)gui/core/utils
	$(MAKEDIR) obj/shared/$(ARCH)gui/core/nanosvg/src
	$(MAKEDIR) obj/shared/$(ARCH)gui/app/components
	$(MAKEDIR) obj/shared/$(ARCH)gui/app/guis
	$(MAKEDIR) obj/shared/$(ARCH)gui/app/scrapers
	$(MAKEDIR) obj/shared/$(ARCH)gui/app/views/gamelist
	$(MAKEDIR) obj/shared/$(ARCH)gui/app/pugixml/src
	$(MAKEDIR) obj/shared/$(ARCH)lcdapi/api
	$(MAKEDIR) obj/shared/$(ARCH)lcdapi/sensors
	$(MAKEDIR) obj/shared/$(ARCH)sources/android/native_app_glue
	
angelscript:
	make -C angelscript/angelscript/projects/gnuc/ static
	
gui: $(GUI_OBJECTS)
	
lib/$(ARCH)$(LIBNAME): angelscript $(OBJECTS) $(GUI_OBJECTS)
	$(GCC) -o $@ $(CFLAGS) $(SHARED_FLAGS) $(GUI_OBJECTS) $(C_OBJECTS) $(LIBS)
	cp $@ $@.debug
	$(STRIP) -S --strip-unneeded $@
	
test: test-client test-server
	
test-client: lib
	$(MAKE) -C ./test/nymph_test_client
	
test-server: lib
	$(MAKE) -C ./test/nymph_test_server

clean: clean-lib clean-angelscript clean-gui

clean-test: clean-test-client clean-test-server

clean-lib:
	$(RM) $(OBJECTS) $(GUI_OBJECTS)
	
clean-angelscript:
	make -C angelscript/angelscript/projects/gnuc/ clean
	
clean-gui:
	$(RM) $(GUI_OBJECTS)
	
clean-test-client:
	$(MAKE) -C ./test/nymph_test_client clean
	
clean-test-server:
	$(MAKE) -C ./test/nymph_test_server clean

PREFIX ?= /usr
ifdef OS
# Assume 64-bit MSYS2
PREFIX = /mingw64
endif

.PHONY: install angelscript gui
# install:
	# install -d $(DESTDIR)$(PREFIX)/lib/
	# install -m 644 lib/$(ARCH)$(OUTPUT).a $(DESTDIR)$(PREFIX)/lib/
# ifndef OS
	# install -m 644 lib/$(ARCH)$(OUTPUT).so.$(VERSION) $(DESTDIR)$(PREFIX)/lib/
# endif
	# install -d $(DESTDIR)$(PREFIX)/include/nymph
	# install -m 644 src/*.h $(DESTDIR)$(PREFIX)/include/nymph/
# ifndef OS
	# cd $(DESTDIR)$(PREFIX)/lib && \
		# if [ -f $(OUTPUT).so ]; then \
			# rm $(OUTPUT).so; \
		# fi && \
		# ln -s $(OUTPUT).so.$(VERSION) $(OUTPUT).so
# endif

# package:
	# tar -C lib/$(ARCH) -cvzf lib/$(OUTPUT)-$(VERSION)-$(USYS)-$(UMCH).tar.gz $(OUTPUT).a $(OUTPUT).so.$(VERSION)
