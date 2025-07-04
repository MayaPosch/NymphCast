
CXX ?= g++
LIBS := -lnymphcast
LDFLAGS += -shared $(LIBS)

VERSION := 0.1
OUTPUT_DISC := -Wl,-soname,module-nymphcast-discover.so.$(VERSION)
OUTPUT_SINK := -Wl,-soname,module-nymphcast-sink.so.$(VERSION)

all: discover-module sink-module

discover-module:
	(CXX) -o module-nymphcast-discover.so.$(VERSION) module-nymphcast-discover.cpp $(LDFLAGS)
	
sink-module:
	(CXX) -o module-nymphcast-sink.so.$(VERSION) module-nymphcast-sink.cpp $(LDFLAGS)

