# Makefile for the NymphCast client project.
#
# (c) 2019 Nyanko.ws

GCC = g++
MAKEDIR = mkdir -p
RM = rm

# Include the file with the versioning information ('VERSION' variable).
include version
VERSIONINFO = -D__VERSION="\"$(VERSION)\""

OUTPUT = nymphcast_client
INCLUDE = -I .
LIB := -lnymphcast -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON -lstdc++fs
CFLAGS := $(INCLUDE) -g3 -std=c++17 $(VERSIONINFO)

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	CFLAGS := $(CFLAGS) -U__STRICT_ANSI__
	LIB += -lws2_32
endif

SOURCES := $(wildcard *.cpp)
OBJECTS := $(addprefix obj/,$(notdir) $(SOURCES:.cpp=.o))

all: makedir bin/$(OUTPUT)

makedir:
	$(MAKEDIR) obj
	$(MAKEDIR) bin

obj/%.o: %.cpp
	$(GCC) -c -o $@ $< $(CFLAGS)
	
bin/$(OUTPUT): $(OBJECTS)
	$(GCC) -o $@ $(OBJECTS) $(LIB)

clean:
	$(RM) $(OBJECTS)
	
