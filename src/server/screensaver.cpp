

#include "screensaver.h"

#include <thread>
#include <filesystem> 		// C++17

namespace fs = std::filesystem;


// Static variables.
std::atomic<bool> ScreenSaver::active = {false};
std::atomic<bool> ScreenSaver::firstRun = {true};
SDL_Window* ScreenSaver::window = 0;
SDL_Renderer* ScreenSaver::renderer = 0;
SDL_Texture* ScreenSaver::texture = 0;
ChronoTrigger ScreenSaver::ct;
ChronoTrigger ScreenSaver::sdl_ct;
//std::vector<std::string> ScreenSaver::images = { "green.jpg", "forest_brook.jpg" };
std::vector<std::string> ScreenSaver::images;
int ScreenSaver::imageId = 0;
std::string ScreenSaver::dataPath;



void ScreenSaver::changeImage(int) {
	if (firstRun) {
		// Set up SDL.
		SDL_Init(SDL_INIT_VIDEO);
		IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
		
		// Create window.
		window = SDL_CreateWindow("SDL2 Displaying Image",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);
		
		// Create renderer.
		renderer = SDL_CreateRenderer(window, -1, 0);
		if (!renderer) {
			fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
			return;
		}
		
		firstRun = false;
	}
	
	// Load current image.
	if (texture) { SDL_DestroyTexture(texture); texture = 0; }
	
	fprintf(stdout, "Changing image to %s\n", images[imageId].data());
	
	texture = IMG_LoadTexture(renderer, images[imageId++].data());
	if (!(imageId < images.size())) { imageId = 0; }
	
	//SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, 0, 0);
	SDL_RenderPresent(renderer);
	
	SDL_Event event;
	SDL_PollEvent(&event);
}


void ScreenSaver::sdlLoop(int) {
	// Keep the SDL event queue happy.
	if (!firstRun) {
		SDL_Event event;
		SDL_PollEvent(&event);
	}
}


void ScreenSaver::cleanUp() {
	// Clean up SDL.
	fprintf(stderr, "Destroying texture...\n");
	SDL_DestroyTexture(texture);
	fprintf(stderr, "Destroying renderer...\n");
	SDL_DestroyRenderer(renderer);
	fprintf(stderr, "Destroying window...\n");
	SDL_DestroyWindow(window);
	
	fprintf(stderr, "Quitting...\n");
	
	IMG_Quit();
	/*SDL_Quit() */;
}


void ScreenSaver::setDataPath(std::string path) {
	dataPath = path;
}


void ScreenSaver::start(uint32_t changeSecs) {
	if (active) { return; }
	
	// Read in image names.
	for (const fs::directory_entry& entry : fs::directory_iterator(dataPath)) {
        images.push_back(entry.path().string());
	}
	
	fprintf(stdout, "Found %d wallpapers.", images.size());
	
	// Display initial image.
	changeImage(0);
	
	// Cycle through the wallpapers until stopped.
	ct.setCallback(ScreenSaver::changeImage, 0);
	ct.setStopCallback(ScreenSaver::cleanUp);
	ct.start(changeSecs * 1000);	// Convert to milliseconds.
	
	/* sdl_ct.setCallback(ScreenSaver::sdlLoop, 0);
	ct.start(500); */
	
	active = true;
	
	/* while (active) {
		SDL_Event event;
		SDL_PollEvent(&event);
	} */
}


void ScreenSaver::stop() {
	if (!active) { return; }
	
	fprintf(stderr, "Stopping timer...\n");
	
	// Stop timer.
	ct.stop();
	//sdl_ct.stop();
	
	active = false;
	firstRun = true;
}
