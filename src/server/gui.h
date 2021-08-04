/*
	gui.h - Header file for the GUI module.
	
	Revision 0
	
	2021/06/01, Maya Posch
*/


#ifndef NC_GUI_H
#define NC_GUI_H


#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "gui/core/Window.h"
#include "gui/app/SystemScreenSaver.h"

#include "nymphcast_client.h"


class Gui {
	static std::thread* sdl;
	static std::atomic<bool> running;
	static Window window;
	static SystemScreenSaver* screensaver;
	static std::string resourceFolder;
	
	static bool verifyHomeFolderExists();
	
public:
	static NymphCastClient* client;
	static std::condition_variable resumeCv;
	static std::mutex resumeMtx;
	static std::atomic<bool> active;
	
	static bool init(std::string resFolder);
	static bool start();
	static bool stop();
	static bool quit();
};


#endif
