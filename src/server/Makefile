# Makefile for the NymphCast server project.
#
# (c) 2019 Nyanko.ws


ifdef PLATFORM
include platforms/$(PLATFORM).mk
TARGET := $(PLATFORM)
CROSSC := 1
else
# Get the compiler's (GCC or Clang) target triplet and use that as platform.
TARGET := $(shell g++ -dumpmachine)
endif

ifdef TOOLCHAIN
#include Makefile.$(TARGET)
include toolchain/$(TOOLCHAIN).mk
else
GPP = g++
GCC = gcc
STRIP = strip
MAKEDIR = mkdir -p
RM = rm
endif

$(info TARGET: $(TARGET))

export TARGET
export GPP
export PLATFORM_FLAGS

USYS := $(shell uname -s)
UMCH := $(shell uname -m)

TARGET_BIN := $(TARGET)/

# Include the file with the versioning information ('VERSION' variable).
include version.mk
VERSIONINFO = -D__VERSION="\"$(VERSION)\""

OUTPUT = nymphcast_server
INCLUDE = -I . -I ffplay -I angelscript/angelscript/include -I angelscript/add_on \
			-I angelscript/json -I angelscript/regexp \
			-I gui/core -I gui/core/animations  -I gui/core/components  -I gui/core/guis \
			-I gui/core/math  -I gui/core/renderers  -I gui/core/resources  -I gui/core/utils \
			-I gui/app -I gui/app/animations -I gui/app/components -I gui/app/guis \
			-I gui/app/scrapers -I gui/app/views -I gui/app/gamelist \
			-I gui/core/nanosvg
ifdef CROSSC
INCLUDE += -I $(SYSROOT)/usr/include/SDL2 -I $(SYSROOT)/usr/include/freetype2
else 
INCLUDE += `sdl2-config --cflags` \
			`pkg-config --cflags freetype2`
endif

LIB := -lnymphrpc -lPocoUtil -lPocoNet -lPocoNetSSL -lPocoJSON -lPocoData -lPocoDataSQLite \
		-lPocoFoundation -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc \
		-lswresample -lavfilter -lSDL2_image -Langelscript/angelscript/lib-$(TARGET) -langelscript \
		-lstdc++fs -lfreetype \
			 -lcurl -lfreeimage \
			-lnymphcast -lPocoNet -lPocoUtil -lPocoFoundation
			# -lvlc

STATIC_LIB := -Wl,-Bstatic -lnymphrpc -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc \
		-lswresample -lavfilter -lSDL2_image -Langelscript/angelscript/lib-$(TARGET) -langelscript \
		-lstdc++fs -lfreetype -lcurl -lfreeimage -lnymphcast \
		`pkg-config --libs freetype2` `sdl2-config --libs` \
		 -Wl,-Bdynamic -lPocoNet -lPocoNetSSL -lPocoUtil -lPocoFoundation -lPocoJSON -lPocoData \
		-lPocoDataSQLite -lPocoNet -lPocoUtil -lPocoFoundation
		# -lvlc
ifdef CROSSC
LIB := -L $(SYSROOT)/usr/lib $(PLATFORM_LDFLAGS) -lfreetype -lSDL2main -lSDL2 $(LIB) -lpthread -lc
else
LIB += `pkg-config --libs freetype2` `sdl2-config --libs`
endif

FLAGS := -ffunction-sections -fdata-sections -g3 -O0 $(PLATFORM_FLAGS)
CFLAGS := $(FLAGS) $(INCLUDE) -std=c11 $(PLATFORM_FLAGS)
CPPFLAGS := $(FLAGS) $(INCLUDE) -std=c++17 $(PLATFORM_FLAGS) $(VERSIONINFO)
LDFLAGS := -Wl,--gc-sections -Wl,-Map,bin/$(TARGET_BIN)$(OUTPUT).map $(PLATFORM_LDFLAGS) $(LIB)

# Check for a static build.
ifdef STATIC
	TARGET_BIN := $(TARGET)-static/
	TARGET := $(TARGET)-static
	CPPFLAGS := $(CPPFLAGS) -DPOCO_STATIC
	#LDFLAGS := -Wl,--gc-sections -Wl,-Map,bin/$(TARGET_BIN)$(OUTPUT).map $(PLATFORM_LDFLAGS) $(STATIC_LIB)
	LDFLAGS := -static $(LDFLAGS)
endif

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	CPPFLAGS += -U__STRICT_ANSI__ -DPOCO_WIN32_UTF8=1
	LDFLAGS += -lws2_32 -lopengl32 -lwinmm -lole32
	OUTPUT := $(OUTPUT).exe
else
	LDFLAGS += -pthread -latomic -lGL -lasound
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
C_SOURCES := 	$(wildcard *.c) $(wildcard ffplay/*.c)
OBJECTS := 		$(addprefix obj/$(TARGET_BIN),$(notdir) $(SOURCES:.cpp=.o))
GUI_OBJECTS := 	$(addprefix obj/$(TARGET_BIN),$(notdir) $(GUI_SOURCES:.cpp=.o))
C_OBJECTS := 	$(addprefix obj/$(TARGET_BIN),$(notdir) $(C_SOURCES:.c=.o))

PREFIX ?= /usr/local
ifdef OS
# Assume 64-bit MSYS2
PREFIX = /mingw64
endif

ifeq ($(PREFIX),/usr/local)
	CONFDIR := $(PREFIX)/etc
else
	CONFDIR := /etc
endif

SED_REPLACE := -e 's:@BIN@:$(PREFIX)/bin/$(OUTPUT):g' \
	-e 's:@CONFIG@:$(CONFDIR)/nymphcast/nymphcast_config.ini:g' \
	-e 's:@APPS@:$(PREFIX)/share/nymphcast/apps/:g' \
	-e 's:@WALLPAPERS@:$(PREFIX)/share/nymphcast/wallpapers/:g'

all: makedir bin/$(TARGET_BIN)$(OUTPUT)

makedir:
	$(MAKEDIR) obj/$(TARGET_BIN)ffplay
	$(MAKEDIR) obj/$(TARGET_BIN)angelscript/add_on/scriptstdstring
	$(MAKEDIR) obj/$(TARGET_BIN)angelscript/add_on/scriptbuilder
	$(MAKEDIR) obj/$(TARGET_BIN)angelscript/add_on/scriptarray
	$(MAKEDIR) obj/$(TARGET_BIN)angelscript/add_on/scriptdictionary
	$(MAKEDIR) obj/$(TARGET_BIN)angelscript/json
	$(MAKEDIR) obj/$(TARGET_BIN)angelscript/regexp
	$(MAKEDIR) obj/$(TARGET_BIN)gui/core/animations
	$(MAKEDIR) obj/$(TARGET_BIN)gui/core/components
	$(MAKEDIR) obj/$(TARGET_BIN)gui/core/guis
	$(MAKEDIR) obj/$(TARGET_BIN)gui/core/math
	$(MAKEDIR) obj/$(TARGET_BIN)gui/core/renderers
	$(MAKEDIR) obj/$(TARGET_BIN)gui/core/resources
	$(MAKEDIR) obj/$(TARGET_BIN)gui/core/utils
	$(MAKEDIR) obj/$(TARGET_BIN)gui/core/nanosvg/src
	$(MAKEDIR) obj/$(TARGET_BIN)gui/app/components
	$(MAKEDIR) obj/$(TARGET_BIN)gui/app/guis
	$(MAKEDIR) obj/$(TARGET_BIN)gui/app/scrapers
	$(MAKEDIR) obj/$(TARGET_BIN)gui/app/views/gamelist
	$(MAKEDIR) obj/$(TARGET_BIN)gui/app/pugixml/src
	$(MAKEDIR) obj/$(TARGET_BIN)lcdapi/api
	$(MAKEDIR) obj/$(TARGET_BIN)lcdapi/sensors
	$(MAKEDIR) bin/$(TARGET)
	
angelscript:
	make -C angelscript/angelscript/projects/gnuc/ static
	
gui: $(GUI_OBJECTS)

obj/$(TARGET_BIN)%.o: %.cpp
	$(GPP) -c -o $@ $< $(CPPFLAGS)

obj/$(TARGET_BIN)%.o: %.c
	$(GCC) -c -o $@ $< -g3 $(CFLAGS)
	
bin/$(TARGET_BIN)$(OUTPUT): angelscript $(OBJECTS) $(C_OBJECTS) $(GUI_OBJECTS)
	$(GPP) -o $@ $(OBJECTS) $(GUI_OBJECTS) $(C_OBJECTS) $(LDFLAGS)
	cp $@ $@.debug
	$(STRIP) -S --strip-unneeded $@

clean: clean-objects clean-angelscript clean-gui

clean-objects:
	$(RM) $(OBJECTS) $(C_OBJECTS)
	
clean-angelscript:
	make -C angelscript/angelscript/projects/gnuc/ clean
	
clean-gui:
	$(RM) $(GUI_OBJECTS)
	
.PHONY: all clean angelscript makedir gui
	
.PHONY: install
install:
	install -d $(DESTDIR)$(PREFIX)/bin/ \
		-d $(DESTDIR)$(CONFDIR)/nymphcast/ \
		-d $(DESTDIR)$(PREFIX)/share/nymphcast/apps/ \
		-d $(DESTDIR)$(PREFIX)/share/nymphcast/wallpapers/ \
		-d $(DESTDIR)$(CONFDIR)/avahi/services/
	install -m 755 bin/$(TARGET_BIN)$(OUTPUT) $(DESTDIR)$(PREFIX)/bin/
	install -m 644 *.ini $(DESTDIR)$(CONFDIR)/nymphcast/
	cp -r apps/* $(DESTDIR)$(PREFIX)/share/nymphcast/apps/
	install -m 644 wallpapers/*.jpg $(DESTDIR)$(PREFIX)/share/nymphcast/wallpapers/

.PHONY: install-systemd
install-systemd:
	#install -d $(DESTDIR)$(CONFDIR)/systemd/system/ 
	sed ${SED_REPLACE} systemd/nymphcast.service > /etc/systemd/system/nymphcast.service

.PHONY: install-openrc
install-openrc:
	install -d $(DESTDIR)$(CONFDIR)/init.d/	
	sed ${SED_REPLACE} openrc/nymphcast > $(DESTDIR)$(CONFDIR)/init.d/nymphcast
	chmod 0755 $(DESTDIR)$(CONFDIR)/init.d/nymphcast


# Construct an archive for Raspbian: audio-only, or full.
ar-rasp-full: bin/$(TARGET_BIN)$(OUTPUT)
	rm -rf out/tmp
	rm -f out/nymphcast_raspbian.zip
	$(MAKEDIR) out/tmp
	cp -r raspbian/. out/tmp/
	cp bin/$(TARGET_BIN)$(OUTPUT) out/tmp/payload/usr/local/bin/.
	cp -a /usr/lib/libnymphrpc.* out/tmp/payload/usr/lib/.
	cp -a /usr/lib/libnymphcast.* out/tmp/payload/usr/lib/.
	$(MAKEDIR) out/tmp/payload/etc/nymphcast
	cp nymphcast_screensaver_config.ini out/tmp/payload/etc/nymphcast/nymphcast_config.ini
	$(MAKEDIR) out/tmp/payload/usr/local/share/nymphcast
	cp -r apps/ out/tmp/payload/usr/local/share/nymphcast/.
	cp -r wallpapers/ out/tmp/payload/usr/local/share/nymphcast/.
	#tar -cvJf out/nymphcast_raspbian.xz -C out/tmp/ .
	cd out/tmp && zip --symlinks -vr ../nymphcast_raspbian.zip . && cd -
	
ar-rasp-audio: bin/$(TARGET_BIN)$(OUTPUT)
	rm -rf out/tmp
	rm -f out/nymphcast_audio_raspbian.zip
	$(MAKEDIR) out/tmp
	cp -r raspbian/. out/tmp/
	cp bin/$(TARGET_BIN)$(OUTPUT) out/tmp/payload/usr/local/bin/.
	cp -a /usr/lib/libnymphrpc.* out/tmp/payload/usr/lib/.
	cp -a /usr/lib/libnymphcast.* out/tmp/payload/usr/lib/.
	$(MAKEDIR) out/tmp/payload/etc/nymphcast
	cp nymphcast_audio_config.ini out/tmp/payload/etc/nymphcast/nymphcast_config.ini
	$(MAKEDIR) out/tmp/payload/usr/local/share/nymphcast
	cp -r apps/ out/tmp/payload/usr/local/share/nymphcast/.
	#tar -cvJf out/nymphcast_raspbian.xz -C out/tmp/ .
	cd out/tmp && zip --symlinks -vr ../nymphcast_audio_raspbian.zip . && cd -

# Package up the compiled project and dependencies.
# For use on Linux-compatible platforms.
package:
	rm -rf out/nymphcast
	$(MAKEDIR) out/nymphcast/bin
	$(MAKEDIR) out/nymphcast/lib
	$(MAKEDIR) out/nymphcast/systemd
	cp bin/$(TARGET_BIN)$(OUTPUT) out/nymphcast/bin/.
	cp -a /usr/lib/libnymphrpc.* out/nymphcast/lib/.
	cp -a /usr/lib/libnymphcast.* out/nymphcast/lib/.
	cp nymphcast_*_config.ini out/nymphcast/.
	cp -r apps/ out/nymphcast/.
	cp -r wallpapers/ out/nymphcast/.
	cp systemd/nymphcast_filled.service out/nymphcast/systemd/nymphcast.service
	cp install.sh out/nymphcast/.
	#tar -C bin/$(TARGET_BIN) -cvzf bin/$(OUTPUT)-$(VERSION)-$(USYS)-$(UMCH).tar.gz $(OUTPUT)
	tar -C out/ -cvzf out/$(OUTPUT)-$(VERSION)-$(USYS)-$(UMCH).tar.gz nymphcast
