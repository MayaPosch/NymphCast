# Makefile for the NymphCast client project.
#
# (c) 2019 Nyanko.ws

ifdef TARGET
include Makefile.$(TARGET)
else
GPP = g++
GCC = gcc
MAKEDIR = mkdir -p
RM = rm
endif

ifdef PLATFORM
include platforms/Makefile.$(PLATFORM)
endif

# Include the file with the versioning information ('VERSION' variable).
include version
VERSIONINFO = -D__VERSION="\"$(VERSION)\""

OUTPUT = nymphcast_server
INCLUDE = -I . -I ffplay -I angelscript/angelscript/include -I angelscript/add_on -I angelscript/json `sdl2-config --cflags`
LIB := -lnymphrpc -lPocoNet -lPocoNetSSL -lPocoUtil -lPocoFoundation -lPocoJSON -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc -lswresample -lavfilter -lSDL2_image -Langelscript/angelscript/lib -langelscript -lstdc++fs `sdl2-config --libs`
FLAGS := -ffunction-sections -fdata-sections -g3 -O0
CFLAGS := $(FLAGS) $(INCLUDE) -std=c11 $(PLATFORM_FLAGS)
CPPFLAGS := $(FLAGS) $(INCLUDE) -std=c++17 $(PLATFORM_FLAGS) $(VERSIONINFO)
LDFLAGS := -Wl,--gc-sections $(LIB)

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
			$(wildcard angelscript/json/*.cpp) 
C_SOURCES := $(wildcard *.c) $(wildcard ffplay/*.c)
OBJECTS := $(addprefix obj/,$(notdir) $(SOURCES:.cpp=.o))
C_OBJECTS := $(addprefix obj/,$(notdir) $(C_SOURCES:.c=.o))

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

all: bin/$(OUTPUT)

makedir:
	$(MAKEDIR) obj/ffplay
	$(MAKEDIR) obj/angelscript/add_on/scriptstdstring
	$(MAKEDIR) obj/angelscript/add_on/scriptbuilder
	$(MAKEDIR) obj/angelscript/add_on/scriptarray
	$(MAKEDIR) obj/angelscript/add_on/scriptdictionary
	$(MAKEDIR) obj/angelscript/json
	$(MAKEDIR) bin
	
angelscript:
	make -C angelscript/angelscript/projects/gnuc/ static

obj/%.o: %.cpp
	$(GPP) -c -o $@ $< $(CPPFLAGS)

obj/%.o: %.c
	$(GCC) -c -o $@ $< -g3 $(CFLAGS)
	
bin/$(OUTPUT): angelscript makedir $(OBJECTS) $(C_OBJECTS)
	$(GPP) -o $@ $(OBJECTS) $(C_OBJECTS) $(LDFLAGS)
	cp $@ $@.debug
	strip -S --strip-unneeded $@

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
	install -m 644 avahi/nymphcast.service $(DESTDIR)$(CONFDIR)/avahi/services/

.PHONY: install-systemd
install-systemd:
	install -d $(DESTDIR)$(CONFDIR)/systemd/system/ 
	sed ${SED_REPLACE} systemd/nymphcast.service > $(DESTDIR)$(CONFDIR)/systemd/system/nymphcast.service

.PHONY: install-openrc
install-openrc:
	install -d $(DESTDIR)$(CONFDIR)/init.d/	
	sed ${SED_REPLACE} openrc/nymphcast > $(DESTDIR)$(CONFDIR)/init.d/nymphcast
