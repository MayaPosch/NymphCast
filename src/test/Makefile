


CPPFLAGS := -std=c++17 -g3 -O0
SDL_LIBS := -lSDL2 -lSDL2main -lSDL2_image

all: makedirs test_screensaver


makedirs:
	mkdir -p bin/


test_screensaver:
	g++ -o bin/test_screensaver -I../. ../server/screensaver.cpp ../server/chronotrigger.cpp test_screensaver.cpp $(CPPFLAGS) $(SDL_LIBS)
	cp ../server/green.jpg bin/green.jpg
	cp ../server/forest_brook.jpg bin/forest_brook.jpg
	
	
clean:
	
