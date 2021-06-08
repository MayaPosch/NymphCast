

LIBNAME=librmlui
ARLIB=$(LIBNAME).a

OBJDIR = obj-$(TARGET)
LIBDIR = lib-$(TARGET)
CXX := $(GPP)
# On i686 architecture you may need to add -march=i686 if you get 
# an undefined symbol for __sync_sub_and_fetch_4 in as_atomic.cpp.
CXXFLAGS += -Wall -fPIC -fno-strict-aliasing -DRMLUI_STATIC_LIB -std=c++14 -g3
ifdef OS
	INCLUDES := -I/mingw64/include/freetype2
else
	INCLUDES := -I/usr/include/freetype2
endif

AR ?= ar
RANLIB ?= ranlib

SRCNAMES = $(wildcard source/Core/*.cpp) \
			$(wildcard source/Core/Elements/*.cpp) \
			$(wildcard source/Core/FontEngineDefault/*.cpp) \
			$(wildcard source/Debugger/*.cpp) \
			#$(wildcard source/SVG/*.cpp)

OBJ = $(addprefix $(OBJDIR)/, $(SRCNAMES:.cpp=.o))


all: static

static: $(LIBDIR) $(OBJDIR) $(LIBDIR)/$(ARLIB)

$(OBJDIR):
	mkdir -p $(OBJDIR)/source/Core/Elements
	mkdir -p $(OBJDIR)/source/Core/FontEngineDefault
	mkdir -p $(OBJDIR)/source/Debugger
	#mkdir -p $(OBJDIR)/source/SVG

$(LIBDIR):
	mkdir -p "$(LIBDIR)"

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ -c $<

$(LIBDIR)/$(ARLIB): $(OBJ)
	$(AR) r "$(LIBDIR)"/$(ARLIB) $(OBJ)
	$(RANLIB) "$(LIBDIR)"/$(ARLIB)

clean:
	rm $(OBJ)
