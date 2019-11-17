# asrun MingW makefile

CXX = g++
CXXFLAGS = -O2 -I../../../../angelscript/include
SRCDIR = ../../source
OBJDIR = obj

SRCNAMES = \
  main.cpp \

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o))) \
  obj/scriptarray.o \
  obj/scripthelper.o \
  obj/scriptstdstring.o \
  obj/scriptstdstringutil.o \
  obj/scriptdictionary.o \
  obj/scriptfile.o \
  obj/scriptfilesystem.o \
  obj/scriptbuilder.o \
  obj/debugger.o \
  obj/contextmgr.o \
  obj/datetime.o 


BIN = ../../bin/asrun.exe
OBJ_D = $(subst /,\,$(OBJ))
BIN_D = $(subst /,\,$(BIN))
DELETER = del /f

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) -static -o $(BIN) $(OBJ) -langelscript -L ../../../../angelscript/lib
	@echo -------------------------------------------------------------------
	@echo Done.

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptarray.o: ../../../../add_on/scriptarray/scriptarray.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scripthelper.o: ../../../../add_on/scripthelper/scripthelper.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptstdstring.o: ../../../../add_on/scriptstdstring/scriptstdstring.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptstdstringutil.o: ../../../../add_on/scriptstdstring/scriptstdstring_utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptdictionary.o: ../../../../add_on/scriptdictionary/scriptdictionary.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptfile.o: ../../../../add_on/scriptfile/scriptfile.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptfilesystem.o: ../../../../add_on/scriptfile/scriptfilesystem.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/scriptbuilder.o: ../../../../add_on/scriptbuilder/scriptbuilder.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/debugger.o: ../../../../add_on/debugger/debugger.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/contextmgr.o: ../../../../add_on/contextmgr/contextmgr.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/datetime.o: ../../../../add_on/datetime/datetime.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	$(DELETER) $(OBJ_D) $(BIN_D)

.PHONY: all clean
