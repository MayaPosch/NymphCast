

#ifndef SCREENSAVER_H
#define SCREENSAVER_H


#include <atomic>
#include <vector>
#include <string>

#include "chronotrigger.h"


class ScreenSaver {
	static std::atomic<bool> active;
	static std::atomic<bool> firstRun;
	static ChronoTrigger ct;
	static ChronoTrigger sdl_ct;
	static std::vector<std::string> images;
	static int imageId;
	static std::string dataPath;
	
	static void changeImage(int);
	static void cleanUp();
	
public:
	static void setDataPath(std::string path);
	static void start(uint32_t changeSecs);
	static void stop();
};


#endif
