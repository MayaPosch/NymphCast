# Makefile for the NymphCast client project.
#
# (c) 2019 Nyanko.ws

GCC = g++
MAKEDIR = mkdir -p
RM = rm

OUTPUT = nymphcast_server
INCLUDE = -I .
LIB := -lnymphrpc -lportaudio -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON
CFLAGS := $(INCLUDE) -g3

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	CFLAGS := $(CFLAGS) -U__STRICT_ANSI__
	#LIBS += -lws2_32
endif

SOURCES := $(wildcard *.cpp)
OBJECTS := $(addprefix obj/,$(notdir) $(SOURCES:.cpp=.o))

all: makedir $(OBJECTS) bin/$(OUTPUT)

makedir:
	$(MAKEDIR) obj
	$(MAKEDIR) bin

obj/%.o: %.cpp
	$(GCC) -c -o $@ $< $(CFLAGS)
	
bin/$(OUTPUT):
	$(GCC) -o $@ $(OBJECTS) $(LIB)

clean:
	$(RM) $(OBJECTS)
	