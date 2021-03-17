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
			-I angelscript/json -I angelscript/regexp `sdl2-config --cflags`
LIB := -lnymphrpc -lPocoNet -lPocoNetSSL -lPocoUtil -lPocoFoundation -lPocoJSON -lPocoData -lPocoDataSQLite -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc -lswresample -lavfilter -lSDL2_image -Langelscript/angelscript/lib-$(TARGET) -langelscript -lstdc++fs `sdl2-config --libs`
FLAGS := -ffunction-sections -fdata-sections -g3 -O0 $(PLATFORM_FLAGS)
CFLAGS := $(FLAGS) $(INCLUDE) -std=c11 $(PLATFORM_FLAGS)
CPPFLAGS := $(FLAGS) $(INCLUDE) -std=c++17 $(PLATFORM_FLAGS) $(VERSIONINFO)
LDFLAGS := -Wl,--gc-sections $(PLATFORM_LDFLAGS) $(LIB)

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	CPPFLAGS += -U__STRICT_ANSI__ -DPOCO_WIN32_UTF8=1
	LDFLAGS += -lws2_32
	OUTPUT := $(OUTPUT).exe
else
	LDFLAGS += -pthread -latomic
endif

SOURCES := $(wildcard *.cpp) \
			$(wildcard ffplay/*.cpp) \
			$(wildcard angelscript/add_on/scriptstdstring/*.cpp) \
			$(wildcard angelscript/add_on/scriptbuilder/*.cpp) \
			$(wildcard angelscript/add_on/scriptarray/*.cpp) \
			$(wildcard angelscript/add_on/scriptdictionary/*.cpp) \
			$(wildcard angelscript/json/*.cpp) \
			$(wildcard angelscript/regexp/*.cpp)
C_SOURCES := $(wildcard *.c) $(wildcard ffplay/*.c)
OBJECTS := $(addprefix obj/$(TARGET_BIN),$(notdir) $(SOURCES:.cpp=.o))
C_OBJECTS := $(addprefix obj/$(TARGET_BIN),$(notdir) $(C_SOURCES:.c=.o))

PREFIX ?= /usr/local

ifeq ($(PREFIX),/usr/local)
	CONFDIR := $(PREFIX)/etc
else
	CONFDIR := /etc
endif

SED_REPLACE := -e 's:@BIN@:$(DESTDIR)$(PREFIX)/bin/$(OUTPUT):g' \
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
	$(MAKEDIR) bin/$(TARGET)
	
angelscript:
	make -C angelscript/angelscript/projects/gnuc/ static

obj/$(TARGET_BIN)%.o: %.cpp
	$(GPP) -c -o $@ $< $(CPPFLAGS)

obj/$(TARGET_BIN)%.o: %.c
	$(GCC) -c -o $@ $< -g3 $(CFLAGS)
	
bin/$(TARGET_BIN)$(OUTPUT): angelscript $(OBJECTS) $(C_OBJECTS)
	$(GPP) -o $@ $(OBJECTS) $(C_OBJECTS) $(LDFLAGS)
	cp $@ $@.debug
	$(STRIP) -S --strip-unneeded $@

clean:
	$(RM) $(OBJECTS) $(C_OBJECTS)
	make -C angelscript/angelscript/projects/gnuc/ clean
	
.PHONY: all clean angelscript makedir
	
.PHONY: install
install:
	install -d $(DESTDIR)$(PREFIX)/bin/ \
		-d $(DESTDIR)$(CONFDIR)/nymphcast/ \
		-d $(DESTDIR)$(PREFIX)/share/nymphcast/apps/ \
		-d $(DESTDIR)$(PREFIX)/share/nymphcast/wallpapers/ \
		-d $(DESTDIR)$(CONFDIR)/avahi/services/
	install -m 755 bin/$(OUTPUT) $(DESTDIR)$(PREFIX)/bin/
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
ar-rasp-full: bin/$(OUTPUT)
	$(MAKEDIR) out/tmp
	cp -r raspbian/. out/tmp/
	cp bin/$(OUTPUT) out/tmp/payload/usr/local/bin/.
	$(MAKEDIR) out/tmp/payload/etc/nymphcast
	cp nymphcast_video_config.ini out/tmp/payload/etc/nymphcast/nymphcast_config.ini
	$(MAKEDIR) out/tmp/payload/usr/local/share/nymphcast
	cp -r apps/ out/tmp/payload/usr/local/share/nymphcast/.
	cp -r wallpapers/ out/tmp/payload/usr/local/share/nymphcast/.
	cp forest_brook.jpg out/tmp/payload/usr/local/share/nymphcast/wallpapers/.
	cp green.jpg out/tmp/payload/usr/local/share/nymphcast/wallpapers/.
	#tar -cvJf out/nymphcast_raspbian.xz -C out/tmp/ .
	cd out/tmp && zip -vr ../nymphcast_raspbian.zip . && cd -
	
ar-rasp-audio: bin/$(OUTPUT)
	rm -rf out/tmp
	rm -f out/nymphcast_audio_raspbian.zip
	$(MAKEDIR) out/tmp
	cp -r raspbian/. out/tmp/
	cp bin/$(OUTPUT) out/tmp/payload/usr/local/bin/.
	$(MAKEDIR) out/tmp/payload/etc/nymphcast
	cp nymphcast_audio_config.ini out/tmp/payload/etc/nymphcast/nymphcast_config.ini
	$(MAKEDIR) out/tmp/payload/usr/local/share/nymphcast
	cp -r apps/ out/tmp/payload/usr/local/share/nymphcast/.
	#tar -cvJf out/nymphcast_raspbian.xz -C out/tmp/ .
	cd out/tmp && zip -vr ../nymphcast_audio_raspbian.zip . && cd -
