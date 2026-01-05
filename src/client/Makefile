# Makefile for the NymphCast client project.
#
# (c) 2019 Nyanko.ws

CXX = g++
MAKEDIR = mkdir -p
RM = rm

# Include the file with the versioning information ('VERSION' variable).
include version.mk
VERSIONINFO = -D__VERSION="\"$(VERSION)\""

OUTPUT = nymphcast_client
INCLUDE = -I .
LDFLAGS := -lnymphcast -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON -lstdc++fs
CXXFLAGS := $(INCLUDE) -g3 -std=c++17 $(VERSIONINFO)

ifeq ($(CXX),g++)
	CXXFLAGS += -fext-numeric-literals
endif

ifndef NPOCO
# Current Poco has troubles with MinGW, so network init has to be done manually.
CXXFLAGS += -DPOCO_NO_AUTOMATIC_LIB_INIT
endif

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	#CXXFLAGS := $(CFLAGS)
	LIB += -lws2_32
endif

SOURCES := $(wildcard *.cpp)
OBJECTS := $(addprefix obj/,$(notdir) $(SOURCES:.cpp=.o))

all: makedir bin/$(OUTPUT)

makedir:
	$(MAKEDIR) obj
	$(MAKEDIR) bin

obj/%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)
	
bin/$(OUTPUT): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) $(OBJECTS)
	
