# Makefile for the NymphCast client project.
#
# (c) 2019 Nyanko.ws

GPP = g++
GCC = gcc
MAKEDIR = mkdir -p
RM = rm

INSTALLDIR ?= /usr/local/bin

OUTPUT = nymphcast_server
INCLUDE = -I . -I angelscript/angelscript/include -I angelscript/add_on -I angelscript/json
LIB := -lnymphrpc -lPocoNet -lPocoNetSSL -lPocoUtil -lPocoFoundation -lPocoJSON -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc -lswresample -lavfilter -lSDL2 -lSDL2main -lSDL2_image -Langelscript/angelscript/lib -langelscript -lstdc++fs
CFLAGS := $(INCLUDE) -g3 -std=c++17 -O0

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	CFLAGS := $(CFLAGS) -U__STRICT_ANSI__ -DPOCO_WIN32_UTF8=1
	LIB += -lws2_32
else
	LIB := $(LIB) -pthread -latomic
endif

SOURCES := $(wildcard *.cpp) $(wildcard angelscript/add_on/scriptstdstring/*.cpp) \
			$(wildcard angelscript/add_on/scriptbuilder/*.cpp) \
			$(wildcard angelscript/add_on/scriptarray/*.cpp) \
			$(wildcard angelscript/add_on/scriptdictionary/*.cpp) \
			$(wildcard angelscript/json/*.cpp) 
C_SOURCES := $(wildcard *.c)
OBJECTS := $(addprefix obj/,$(notdir) $(SOURCES:.cpp=.o))
C_OBJECTS := $(addprefix obj/,$(notdir) $(C_SOURCES:.c=.o))

all: makedir angelscript $(OBJECTS) $(C_OBJECTS) bin/$(OUTPUT)

makedir:
	$(MAKEDIR) obj/angelscript/add_on/scriptstdstring
	$(MAKEDIR) obj/angelscript/add_on/scriptbuilder
	$(MAKEDIR) obj/angelscript/add_on/scriptarray
	$(MAKEDIR) obj/angelscript/add_on/scriptdictionary
	$(MAKEDIR) obj/angelscript/json
	$(MAKEDIR) bin
	
angelscript:
	make -C angelscript/angelscript/projects/gnuc/ static

obj/%.o: %.cpp
	$(GPP) -c -o $@ $< $(CFLAGS)

obj/%.o: %.c
	$(GCC) -c -o $@ $< -g3 $(INCLUDE) -std=c11
	
bin/$(OUTPUT):
	$(GPP) -o $@ $(OBJECTS) $(C_OBJECTS) $(LIB)
	
install:
	cp bin/$(OUTPUT) $(INSTALLDIR)/.

clean:
	$(RM) $(OBJECTS) $(C_OBJECTS)
	make -C angelscript/angelscript/projects/gnuc/ clean
	
.PHONY: all clean angelscript makedir
	