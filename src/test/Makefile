
GPP = g++
GCC = gcc

TARGET_BIN := 
OUTPUT := test_ffplay_local_file

CPPFLAGS := -std=c++17 -g3 -O0 -pthread
SDL_FLAGS := `sdl2-config --cflags`
SDL_LIBS := `sdl2-config --libs` -lSDL2_image

FFPLAY_SRC := ../server/ffplay/audio_renderer.cpp \
				../server/ffplay/clock.cpp \
				../server/ffplay/decoder.cpp \
				../server/ffplay/frame_queue.cpp \
				../server/ffplay/packet_queue.cpp \
				../server/ffplay/player.cpp \
				../server/ffplay/sdl_renderer.cpp \
				../server/ffplay/stream_handler.cpp \
				../server/ffplay/subtitle_handler.cpp \
				../server/ffplay/video_renderer.cpp
FFPLAY_SRC_C := ../server/ffplay/cmdutils.c
FFPLAY_OBJ := $(addprefix obj/$(TARGET_BIN),$(notdir) $(FFPLAY_SRC:.cpp=.o))
FFPLAY_OBJ_C := $(addprefix obj/$(TARGET_BIN),$(notdir) $(FFPLAY_SRC_C:.c=.o))
FFPLAY_FLAGS := -I ../server/ffplay
FFPLAY_LD := -lPocoFoundation -lswscale -lavcodec -lavdevice -lavformat -lavutil -lpostproc \
							-lswresample -lavfilter $(SDL_LIBS)

#$(wildcard ../server/ffplay/*.cpp)


all: makedirs test_screensaver test_databuffer test_databuffer_mm


makedirs:
	mkdir -p bin/
	mkdir -p obj/
	mkdir -p server/ffplay
	
test_databuffer:
	g++ -o bin/test_databuffer -I../. ../server/databuffer.cpp test_databuffer.cpp $(CPPFLAGS) 
	
test_databuffer_mm:
	g++ -o bin/test_databuffer_mm -I../. ../server/databuffer.cpp test_databuffer_mm.cpp $(CPPFLAGS) 

test_screensaver:
	g++ -o bin/test_screensaver -I../. ../server/screensaver.cpp ../server/chronotrigger.cpp test_screensaver.cpp $(CPPFLAGS) $(SDL_LIBS)
	cp ../server/green.jpg bin/green.jpg
	cp ../server/forest_brook.jpg bin/forest_brook.jpg
	
test_databuffer_mport:
	g++ -o bin/test_db_mp -I. test_databuffer_multi_port.cpp ../server/databuffer.cpp ../server/chronotrigger.cpp ../server/ffplaydummy.cpp $(CPPFLAGS) -lPocoFoundation
	
test_ffplay_local_file: makedirs $(FFPLAY_OBJ) $(FFPLAY_OBJ_C) obj/test_ffplay_local_file.o bin/test_ffplay_local_file
	
obj/$(TARGET_BIN)%.o: %.cpp
	$(GPP) -c -o $@ $< $(CPPFLAGS) $(SDL_FLAGS) $(FFPLAY_FLAGS) -DTESTING

obj/$(TARGET_BIN)%.o: %.c
	$(GCC) -c -o $@ $< -g3 $ $(SDL_FLAGS) $(FFPLAY_FLAGS)
	
bin/$(TARGET_BIN)$(OUTPUT):
	$(GPP) -o $@ $(FFPLAY_OBJ) obj/$(OUTPUT).o $(FFPLAY_OBJ_C) $(LDFLAGS) $(FFPLAY_LD)
	
clean:
	
