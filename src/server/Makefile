# Makefile for the NymphCast client project.
#
# (c) 2019 Nyanko.ws

GPP = g++
GCC = gcc
MAKEDIR = mkdir -p
RM = rm

OUTPUT = nymphcast_server
INCLUDE = -I .
LIB := -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc -lswresample -lavfilter -lSDL2 -lSDL2main
CFLAGS := $(INCLUDE) -g3 -std=c++17

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	CFLAGS := $(CFLAGS) -U__STRICT_ANSI__
	LIB += -lws2_32
else
	LIB := $(LIB) -pthread
endif

SOURCES := $(wildcard *.cpp)
C_SOURCES := $(wildcard *.c)
OBJECTS := $(addprefix obj/,$(notdir) $(SOURCES:.cpp=.o))
C_OBJECTS := $(addprefix obj/,$(notdir) $(C_SOURCES:.c=.o))

all: makedir $(OBJECTS) $(C_OBJECTS) bin/$(OUTPUT)

makedir:
	$(MAKEDIR) obj
	$(MAKEDIR) bin

obj/%.o: %.cpp
	$(GPP) -c -o $@ $< $(CFLAGS)

obj/%.o: %.c
	$(GCC) -c -o $@ $< -g3 $(INCLUDE) -std=c11
	
bin/$(OUTPUT):
	$(GPP) -o $@ $(OBJECTS) $(C_OBJECTS) $(LIB)

clean:
	$(RM) $(OBJECTS) $(C_OBJECTS)
	