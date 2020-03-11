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

OUTPUT = nymphcast_server
INCLUDE = -I . -I ffplay -I angelscript/angelscript/include -I angelscript/add_on -I angelscript/json `sdl2-config --cflags`
LIB := -lnymphrpc -lPocoNet -lPocoNetSSL -lPocoUtil -lPocoFoundation -lPocoJSON -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc -lswresample -lavfilter -lSDL2_image -Langelscript/angelscript/lib -langelscript -lstdc++fs `sdl2-config --libs`
FLAGS := -ffunction-sections -fdata-sections -g3 -Og
CFLAGS := $(FLAGS) $(INCLUDE) -std=c11 $(PLATFORM_FLAGS)
CPPFLAGS := $(FLAGS) $(INCLUDE) -std=c++17 $(PLATFORM_FLAGS)
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
	
ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

.PHONY: install
install:
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 bin/$(OUTPUT) $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/etc/nymphcast/
	install -m 644 *.ini $(DESTDIR)$(PREFIX)/etc/nymphcast/
	install -d $(DESTDIR)$(PREFIX)/share/nymphcast/apps/
	cp -r apps/* $(DESTDIR)$(PREFIX)/share/nymphcast/apps/
	install -d $(DESTDIR)$(PREFIX)/share/nymphcast/wallpapers/
	install -m 644 *.jpg $(DESTDIR)$(PREFIX)/share/nymphcast/wallpapers/
	install -d $(DESTDIR)$(PREFIX)/etc/avahi/services/
	install -m 644 avahi/nymphcast.service $(DESTDIR)$(PREFIX)/etc/avahi/services/

.PHONY: install-systemd
install-systemd:
	install -d $(DESTDIR)/etc/systemd/system/ 
	install -m 644 systemd/nymphcast.service $(DESTDIR)/etc/systemd/system/

.PHONY: install-openrc
install-openrc:
	install -d $(DESTDIR)/etc/init.d/
	install -m 755 openrc/nymphcast $(DESTDIR)$(PREFIX)/etc/init.d/
