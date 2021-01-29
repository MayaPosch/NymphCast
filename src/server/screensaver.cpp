

#include "screensaver.h"

#include <thread>
#include <filesystem> 		// C++17
#include <iostream>

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
	std::cout << "Changing image to " << images[imageId] << std::endl;
	SdlRenderer::image_display(images[imageId++]);
	if (!(imageId < images.size())) { imageId = 0; }
}


void ScreenSaver::cleanUp() {
	// 
}


void ScreenSaver::setDataPath(std::string path) {
	dataPath = path;
	
	// Read in image names.
	for (const fs::directory_entry& entry : fs::directory_iterator(dataPath)) {
        images.push_back(entry.path().string());
	}
}


void ScreenSaver::start(uint32_t changeSecs) {
	if (active) { return; }
	
	std::cout << "Found " << images.size() << " wallpapers." << std::endl;
	
	// Display initial image.
	changeImage(0);
	
	// Cycle through the wallpapers until stopped.
	ct.setCallback(ScreenSaver::changeImage, 0);
	ct.setStopCallback(ScreenSaver::cleanUp);
	ct.start(changeSecs * 1000);	// Convert to milliseconds.
		
	active = true;
	
	// Start SDL loop.
	SdlRenderer::run_event_loop();
}


void ScreenSaver::stop() {
	if (!active) { return; }
	
	std::cout << "Stopping SDL loop..." << std::endl;
	
	// Stop SDL event loop.
	SdlRenderer::stop_event_loop();
	
	std::cout << "Stopping timer..." << std::endl;
	
	// Stop timer.
	ct.stop();
	
	active = false;
	firstRun = true;
}
