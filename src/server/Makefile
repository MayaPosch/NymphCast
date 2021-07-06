# Makefile for the NymphCast server project.
#
# (c) 2019 Nyanko.ws


ifdef PLATFORM
include platforms/$(PLATFORM).mk
TARGET := $(PLATFORM)
else
# Get the compiler's (GCC or Clang) target triplet and use that as platform.
TARGET := $(shell g++ -dumpmachine)
$(info TARGET: $(TARGET))
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

export TARGET
export GPP

TARGET_BIN := $(TARGET)/

# Include the file with the versioning information ('VERSION' variable).
include version
VERSIONINFO = -D__VERSION="\"$(VERSION)\""

OUTPUT = nymphcast_server
INCLUDE = -I . -I ffplay -I angelscript/angelscript/include -I angelscript/add_on \
			-I angelscript/json -I angelscript/regexp `sdl2-config --cflags` \
			-I gui/core -I gui/core/animations  -I gui/core/components  -I gui/core/guis \
			-I gui/core/math  -I gui/core/renderers  -I gui/core/resources  -I gui/core/utils \
			-I gui/app -I gui/app/animations -I gui/app/components -I gui/app/guis \
			-I gui/app/scrapers -I gui/app/views -I gui/app/gamelist \
			`pkg-config --cflags freetype2` `pkg-config --cflags eigen3` \
			-I gui/core/nanosvg
LIB := -lnymphrpc -lPocoNet -lPocoNetSSL -lPocoUtil -lPocoFoundation -lPocoJSON -lPocoData \
		-lPocoDataSQLite -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc \
		-lswresample -lavfilter -lSDL2_image -Langelscript/angelscript/lib-$(TARGET) -langelscript \
		-Lrmlui/lib-$(TARGET) -lrmlui -lstdc++fs `sdl2-config --libs` -lfreetype \
			`pkg-config --libs freetype2` `pkg-config --libs eigen3` -lcurl -lvlc -lfreeimage \
			-lnymphcast -lPocoNet -lPocoUtil -lPocoFoundation
FLAGS := -ffunction-sections -fdata-sections -g3 -O0 $(PLATFORM_FLAGS)
CFLAGS := $(FLAGS) $(INCLUDE) -std=c11 $(PLATFORM_FLAGS)
CPPFLAGS := $(FLAGS) $(INCLUDE) -std=c++17 $(PLATFORM_FLAGS) $(VERSIONINFO) -DRMLUI_STATIC_LIB
LDFLAGS := -Wl,--gc-sections $(PLATFORM_LDFLAGS) $(LIB)

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	CPPFLAGS += -U__STRICT_ANSI__ -DPOCO_WIN32_UTF8=1
	LDFLAGS += -lws2_32 -lglew32 -lopengl32 -lwinmm -lole32
	OUTPUT := $(OUTPUT).exe
else
	LDFLAGS += -pthread -latomic -lGLEW -lGL
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
			$(wildcard gui/core/*.cpp) \
			$(wildcard gui/core/animations/*.cpp) \
			$(wildcard gui/core/components/*.cpp) \
			$(wildcard gui/core/guis/*.cpp) \
			$(wildcard gui/core/math/*.cpp) \
			$(wildcard gui/core/renderers/*.cpp) \
			$(wildcard gui/core/resources/*.cpp) \
			$(wildcard gui/core/utils/*.cpp) \
			$(wildcard gui/core/nanosvg/src/*.cpp) \
			$(wildcard gui/app/*.cpp) \
			$(wildcard gui/app/components/*.cpp) \
			$(wildcard gui/app/guis/*.cpp) \
			$(wildcard gui/app/scrapers/*.cpp) \
			$(wildcard gui/app/views/*.cpp) \
			$(wildcard gui/app/views/gamelist/*.cpp) \
			$(wildcard gui/app/pugixml/src/*.cpp)
C_SOURCES := $(wildcard *.c) $(wildcard ffplay/*.c)
OBJECTS := $(addprefix obj/$(TARGET_BIN),$(notdir) $(SOURCES:.cpp=.o))
C_OBJECTS := $(addprefix obj/$(TARGET_BIN),$(notdir) $(C_SOURCES:.c=.o))

PREFIX ?= /usr/local

ifeq ($(PREFIX),/usr/local)
	CONFDIR := $(PREFIX)/etc
else
	CONFDIR := /etc
endif

SED_REPLACE := -e 's:@BIN@:$(DESTDIR)$(PREFIX)/bin/$(TARGET_BIN)$(OUTPUT):g' \
	-e 's:@CONFIG@:$(DESTDIR)$(CONFDIR)/nymphcast/nymphcast_config.ini:g' \
	-e 's:@APPS@:$(DESTDIR)$(PREFIX)/share/nymphcast/apps/:g' \
	-e 's:@WALLPAPERS@:$(DESTDIR)$(PREFIX)/share/nymphcast/wallpapers/:g'

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
	$(MAKEDIR) bin/$(TARGET)
	
angelscript:
	make -C angelscript/angelscript/projects/gnuc/ static
	
rmlui:
	make -C rmlui/ static

obj/$(TARGET_BIN)%.o: %.cpp
	$(GPP) -c -o $@ $< $(CPPFLAGS)

obj/$(TARGET_BIN)%.o: %.c
	$(GCC) -c -o $@ $< -g3 $(CFLAGS)
	
bin/$(TARGET_BIN)$(OUTPUT): angelscript $(OBJECTS) $(C_OBJECTS)
	$(GPP) -o $@ $(OBJECTS) $(C_OBJECTS) $(LDFLAGS)
	cp $@ $@.debug
	$(STRIP) -S --strip-unneeded $@

clean: clean-objects clean-angelscript clean-rmlui

clean-objects:
	$(RM) $(OBJECTS) $(C_OBJECTS)
	
clean-angelscript:
	make -C angelscript/angelscript/projects/gnuc/ clean
	
clean-rmlui:
	make -C rmlui/ clean
	
.PHONY: all clean angelscript makedir rmlui
	
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
	install -m 644 *.jpg $(DESTDIR)$(PREFIX)/share/nymphcast/wallpapers/

#install -m 644 avahi/nymphcast.service $(DESTDIR)$(CONFDIR)/avahi/services/

.PHONY: install-systemd
install-systemd:
	install -d $(DESTDIR)$(CONFDIR)/systemd/system/ 
	sed ${SED_REPLACE} systemd/nymphcast.service > $(DESTDIR)$(CONFDIR)/systemd/system/nymphcast.service

.PHONY: install-openrc
install-openrc:
	install -d $(DESTDIR)$(CONFDIR)/init.d/	
	sed ${SED_REPLACE} openrc/nymphcast > $(DESTDIR)$(CONFDIR)/init.d/nymphcast


# Construct an archive for Raspbian: audio-only, or full.
ar-rasp-full: bin/$(TARGET_BIN)$(OUTPUT)
	$(MAKEDIR) out/tmp
	cp -r raspbian/. out/tmp/
	cp bin/$(TARGET_BIN)$(OUTPUT) out/tmp/payload/usr/local/bin/.
	$(MAKEDIR) out/tmp/payload/etc/nymphcast
	cp nymphcast_video_config.ini out/tmp/payload/etc/nymphcast/nymphcast_config.ini
	$(MAKEDIR) out/tmp/payload/usr/local/share/nymphcast
	cp -r apps/ out/tmp/payload/usr/local/share/nymphcast/.
	cp -r wallpapers/ out/tmp/payload/usr/local/share/nymphcast/.
	cp forest_brook.jpg out/tmp/payload/usr/local/share/nymphcast/wallpapers/.
	cp green.jpg out/tmp/payload/usr/local/share/nymphcast/wallpapers/.
	#tar -cvJf out/nymphcast_raspbian.xz -C out/tmp/ .
	cd out/tmp && zip -vr ../nymphcast_raspbian.zip . && cd -
	
ar-rasp-audio: bin/$(TARGET_BIN)$(OUTPUT)
	rm -rf out/tmp
	rm -f out/nymphcast_audio_raspbian.zip
	$(MAKEDIR) out/tmp
	cp -r raspbian/. out/tmp/
	cp bin/$(TARGET_BIN)$(OUTPUT) out/tmp/payload/usr/local/bin/.
	$(MAKEDIR) out/tmp/payload/etc/nymphcast
	cp nymphcast_audio_config.ini out/tmp/payload/etc/nymphcast/nymphcast_config.ini
	$(MAKEDIR) out/tmp/payload/usr/local/share/nymphcast
	cp -r apps/ out/tmp/payload/usr/local/share/nymphcast/.
	#tar -cvJf out/nymphcast_raspbian.xz -C out/tmp/ .
	cd out/tmp && zip -vr ../nymphcast_audio_raspbian.zip . && cd -
