

#include "screensaver.h"

#include <filesystem> 		// C++17
#include <iostream>
#include <fstream>

#include "ffplay/sdl_renderer.h"

namespace fs = std::filesystem;


// Static variables.
std::atomic<bool> ScreenSaver::active = {false};
std::atomic<bool> ScreenSaver::firstRun = {true};
ChronoTrigger ScreenSaver::ct;
std::vector<std::string> ScreenSaver::images;
int ScreenSaver::imageId = 0;
std::string ScreenSaver::dataPath;



void ScreenSaver::changeImage(int) {
	if (firstRun) {		
		firstRun = false;
	}
	
	// Load current image.
	std::cout << "Changing image to " << images[imageId] << std::endl;
	SdlRenderer::screensaverUpdate(images[imageId++]);
	if (!(imageId < images.size())) { imageId = 0; }
}


void ScreenSaver::cleanUp() {
	// 
}


void ScreenSaver::setDataPath(std::string path) {
	dataPath = path;
	
	// Read in image names.
	// If the path ends with a '.list` file, assume it's a text document containing one image name
	// per line, and read this is instead of iterating the folder.
	std::string l = ".list";
	std::string line;
	if (path.compare(path.size() - 5, 5, l) == 0) {
		// Read in list file.
		std::ifstream FILE(path);
		while (std::getline(FILE, line)) {
			images.push_back(line);
		}
	}
	else {
		for (const fs::directory_entry& entry : fs::directory_iterator(dataPath)) {
			std::string ext = entry.path().extension().string();
			if (ext == ".txt" || ext == ".sh") { continue; }
			images.push_back(entry.path().string());
		}
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
}


void ScreenSaver::stop() {
	if (!active) { return; }
	
	std::cout << "Stopping timer..." << std::endl;
	
	// Stop timer.
	ct.stop();
	
	active = false;
	firstRun = true;
}
