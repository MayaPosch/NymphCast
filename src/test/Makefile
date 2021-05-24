


CPPFLAGS := -std=c++17 -g3 -O0 -pthread
SDL_FLAGS := `sdl2-config --cflags`
SDL_LIBS := `sdl2-config --libs` -lSDL2_image

all: makedirs test_screensaver test_databuffer test_databuffer_mm


makedirs:
	mkdir -p bin/
	
test_databuffer:
	g++ -o bin/test_databuffer -I../. ../server/databuffer.cpp test_databuffer.cpp $(CPPFLAGS) 
	
test_databuffer_mm:
	g++ -o bin/test_databuffer_mm -I../. ../server/databuffer.cpp test_databuffer_mm.cpp $(CPPFLAGS) 

test_screensaver:
	g++ -o bin/test_screensaver -I../. ../server/screensaver.cpp ../server/chronotrigger.cpp test_screensaver.cpp $(CPPFLAGS) $(SDL_LIBS)
	cp ../server/green.jpg bin/green.jpg
	cp ../server/forest_brook.jpg bin/forest_brook.jpg
	
	
clean:
	
