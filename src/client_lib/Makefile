# Makefile for the NymphRC library.
#
# Allows one to build the library as a .a file
#
# (c) 2019 Nyanko.ws

export TOP := $(CURDIR)

GCC = g++
MAKEDIR = mkdir -p
RM = rm
AR = ar

OUTPUT = libnymphcast.a
INCLUDE = -I src
#-DPOCO_WIN32_UTF8
LIBS := -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON -lstdc++fs
CFLAGS := $(INCLUDE) -g3 -std=c++17 -O0

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
	CFLAGS := $(CFLAGS) -U__STRICT_ANSI__
	LIBS += -lws2_32
endif

SOURCES := $(wildcard *.cpp)
OBJECTS := $(addprefix obj/,$(notdir) $(SOURCES:.cpp=.o))

all: lib

lib: makedir $(OBJECTS) lib/$(OUTPUT)
	
obj/%.o: %.cpp
	$(GCC) -c -o $@ $< $(CFLAGS) $(LIBS)
	
lib/$(OUTPUT): $(OBJECTS)
	-rm -f $@
	$(AR) rcs $@ $^
	
makedir:
	$(MAKEDIR) lib
	$(MAKEDIR) obj
	
test: test-client test-server
	
test-client:
	$(MAKE) -C ./test/nymph_test_client
	
test-server:
	$(MAKE) -C ./test/nymph_test_server

clean: clean-lib clean-test

clean-test: clean-test-client clean-test-server

clean-lib:
	$(RM) $(OBJECTS)
	
clean-test-client:
	$(MAKE) -C ./test/nymph_test_client clean
	
clean-test-server:
	$(MAKE) -C ./test/nymph_test_server clean
	