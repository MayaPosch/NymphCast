

#include "screensaver.h"

#include <filesystem> 		// C++17
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>

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
#if __ANDROID__
		// Use the SDL file API as it handles Android assets directly.
		SDL_RWops* FILE = SDL_RWFromFile("file.txt", "r");
		if(FILE) {
			// get the size of the file
			Sint64 size = SDL_RWsize(FILE);
			if (size < 10) {
				std::cerr << "Empty wallpaper list file. Skipping." << std::endl;
				return;
			}

			// read the file into a buffer
			char* buffer = new char[size];
			SDL_RWread(FILE, buffer, sizeof(char), size);
			
			// Parse the lines in the buffer.
			char* cline = strtok(buffer, "\n");
			while (cline != NULL) {
				cline[strcspn(cline, "\r\n")] = 0;
				line = std::string(cline);
				images.push_back(line);
				cline = strtok(NULL, "\n");
			}
			
			// Close the file & delete temporary buffer.
			SDL_RWclose(FILE);
			delete[] buffer;
		}
		else {
				std::cerr << "Cannot open wallpapers list file." << std::endl;
				return;
		}
#else
		/* std::ifstream FILE(path);
		while (std::getline(FILE, line)) {
			images.push_back(line);
		} */
		FILE* pFile;
		char cline[100];
		pFile = fopen(path.c_str(), "r");
		if (pFile == NULL) { 
			std::cerr << "Error opening file" << std::endl;
			return;
		}
		
		while (fgets(cline, 100, pFile) != NULL) {
			cline[strcspn(cline, "\r\n")] = 0;
			line = std::string(cline);
			images.push_back(line);
		}
			
		fclose (pFile);
#endif
	}
	else {
		for (const fs::directory_entry& entry : fs::directory_iterator(dataPath)) {
			std::string ext = entry.path().extension().string();
			if (ext == ".txt" || ext == ".sh" || ext == ".list") { continue; }
			images.push_back(entry.path().string());
		}
	}
}


void ScreenSaver::start(uint32_t changeSecs) {
	if (active) { return; }
	
	std::cout << "Found " << images.size() << " wallpapers." << std::endl;
	
	if (images.size() == 0) {
		// Aborting screensaver...
		std::cerr << "Abort screensaver due to lack of images.";
		return;
	}
	
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
