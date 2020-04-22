

#include "screensaver.h"

#include <thread>
#include <filesystem> 		// C++17

#include "ffplay/sdl_renderer.h"

namespace fs = std::filesystem;


// Static variables.
std::atomic<bool> ScreenSaver::active = {false};
std::atomic<bool> ScreenSaver::firstRun = {true};
ChronoTrigger ScreenSaver::ct;
ChronoTrigger ScreenSaver::sdl_ct;
std::vector<std::string> ScreenSaver::images;
int ScreenSaver::imageId = 0;
std::string ScreenSaver::dataPath;



void ScreenSaver::changeImage(int) {
	if (firstRun) {		
		firstRun = false;
	}
	
	// Load current image.
	fprintf(stdout, "Changing image to %s\n", images[imageId].data());
	SdlRenderer::image_display(images[imageId++]);
	if (!(imageId < images.size())) { imageId = 0; }
}


void ScreenSaver::cleanUp() {
	// 
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
		
	active = true;
}


void ScreenSaver::stop() {
	if (!active) { return; }
	
	fprintf(stderr, "Stopping timer...\n");
	
	// Stop timer.
	ct.stop();
	
	active = false;
	firstRun = true;
}
