# Makefile for the NymphCast client library.
#
# Allows one to build the library as a .a file
#
# (c) 2019 Nyanko.ws

export TOP := $(CURDIR)

ifdef ANDROID
TOOLCHAIN_PREFIX := arm-linux-androideabi-
ARCH := android-armv7/
ifdef OS
TOOLCHAIN_POSTFIX := .cmd
endif
else ifdef ANDROID64
TOOLCHAIN_PREFIX := aarch64-linux-android-
ARCH := android-aarch64/
ifdef OS
TOOLCHAIN_POSTFIX := .cmd
endif
endif

ifdef ANDROID
#GCC := $(TOOLCHAIN_PREFIX)g++$(TOOLCHAIN_POSTFIX)
GCC := armv7a-linux-androideabi21-clang++$(TOOLCHAIN_POSTFIX)
MAKEDIR = mkdir -p
RM = rm
AR = $(TOOLCHAIN_PREFIX)ar
else ifdef ANDROID64
GCC := aarch64-linux-android21-clang++$(TOOLCHAIN_POSTFIX)
MAKEDIR = mkdir -p
RM = rm
AR = $(TOOLCHAIN_PREFIX)ar
else ifdef WASM
GCC = emc++
MAKEDIR = mkdir -p
RM = rm
AR = ar 
else
GCC = g++
MAKEDIR = mkdir -p
RM = rm
AR = ar
endif

OUTPUT := libnymphcast
VERSION := 0.1
INCLUDE := -I src
LIBS := -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON 
#-lstdc++fs
CFLAGS := $(INCLUDE) -g3 -std=c++17 -O0
SHARED_FLAGS := -fPIC -shared -Wl,-soname,$(OUTPUT).so.$(VERSION)

ifdef ANDROID
CFLAGS += -fPIC
endif

ifdef ANDROID64
CFLAGS += -fPIC -fno-strict-aliasing
endif

# Check for MinGW and patch up POCO
# The OS variable is only set on Windows.
ifdef OS
ifndef ANDROID
ifndef ANDROID64
	CFLAGS := $(CFLAGS) -U__STRICT_ANSI__ -DPOCO_WIN32_UTF8
	LIBS += -lws2_32
endif
endif
endif

SOURCES := $(wildcard *.cpp)
OBJECTS := $(addprefix obj/static/$(ARCH),$(notdir) $(SOURCES:.cpp=.o))
SHARED_OBJECTS := $(addprefix obj/shared/$(ARCH),$(notdir) $(SOURCES:.cpp=.o))

all: lib

lib: makedir lib/$(ARCH)$(OUTPUT).a lib/$(ARCH)$(OUTPUT).so.$(VERSION)
	
obj/static/$(ARCH)%.o: %.cpp
	$(GCC) -c -o $@ $< $(CFLAGS)
	
obj/shared/$(ARCH)%.o: %.cpp
	$(GCC) -c -o $@ $< $(CFLAGS) $(SHARED_FLAGS) $(LIBS)
	
lib/$(ARCH)$(OUTPUT).a: $(OBJECTS)
	-rm -f $@
	$(AR) rcs $@ $^
	
lib/$(ARCH)$(OUTPUT).so.$(VERSION): $(SHARED_OBJECTS)
	$(GCC) -o $@ $(CFLAGS) $(SHARED_FLAGS) $(SHARED_OBJECTS) $(LIBS)
	
makedir:
	$(MAKEDIR) lib
	$(MAKEDIR) obj/shared
	$(MAKEDIR) obj/static
ifdef ARCH
	$(MAKEDIR) lib/$(ARCH)
	$(MAKEDIR) obj/shared/$(ARCH)
	$(MAKEDIR) obj/static/$(ARCH)
endif
	
test: test-client test-server
	
test-client:
	$(MAKE) -C ./test/nymph_test_client
	
test-server:
	$(MAKE) -C ./test/nymph_test_server

clean: clean-lib 
#clean-test

clean-test: clean-test-client clean-test-server

clean-lib:
	$(RM) $(OBJECTS) $(SHARED_OBJECTS)
	
clean-test-client:
	$(MAKE) -C ./test/nymph_test_client clean
	
clean-test-server:
	$(MAKE) -C ./test/nymph_test_server clean
	
PREFIX ?= /usr

.PHONY: install
install:
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 644 lib/$(OUTPUT).a $(DESTDIR)$(PREFIX)/lib/
	install -m 644 lib/$(OUTPUT).so.$(VERSION) $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 644 nymphcast_client.h $(DESTDIR)$(PREFIX)/include/
	cd $(DESTDIR)$(PREFIX)/lib && \
		if [ -f $(OUTPUT).so ]; then \
			rm $(OUTPUT).so; \
		fi && \
		ln -s $(OUTPUT).so.$(VERSION) $(OUTPUT).so
