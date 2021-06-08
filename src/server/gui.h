/*
	gui.h - Header file for the GUI module.
	
	Revision 0
	
	2021/06/01, Maya Posch
*/


#ifndef NC_GUI_H
#define NC_GUI_H


#include <string>
#include <thread>


class Gui {
	static std::thread* sdl;
	
public:
	static bool init(std::string document);
	static bool start();
	static bool stop();
	static bool quit();
};


#endif
