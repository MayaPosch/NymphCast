
CXX ?= g++

INCLUDES := -I /usr/include/pipewire-0.3 -I /usr/include/spa-0.2
CXX_FLAGS := $(INCLUDES) -fPIC

LIBS := -lnymphcast
LDFLAGS += -shared $(LIBS)

VERSION := 0.1
OUTPUT_DISC := -Wl,-soname,module-nymphcast-discover.so.$(VERSION)
OUTPUT_SINK := -Wl,-soname,module-nymphcast-sink.so.$(VERSION)

all: discover-module sink-module

makedir:
	mkdir -p bin

discover-module: makedir
	$(CXX) -o bin/module-nymphcast-discover.so.$(VERSION) module-nymphcast-discover.cpp $(CXX_FLAGS) $(LDFLAGS)
	
sink-module: makedir
	$(CXX) -o bin/module-nymphcast-sink.so.$(VERSION) module-nymphcast-sink.cpp $(CXX_FLAGS) $(LDFLAGS)

