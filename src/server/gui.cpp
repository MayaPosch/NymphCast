/*
	gui.h - Source file for the GUI module.
	
	Revision 0
	
	2021/06/01, Maya Posch
*/


#include "gui.h"


#include "ffplay/sdl_renderer.h"

#include <GL/glew.h>


// Static definitions.
std::thread* Gui::sdl = 0;


bool Gui::init(std::string document) {
	SdlRenderer::initGui(document);
	
	return true;
}


// --- START ---
bool Gui::start() {
	sdl = new std::thread(SdlRenderer::run_gui_loop);
	
	return true;
}


// --- STOP ---
bool Gui::stop() {
	SdlRenderer::stop_gui_loop();
	
	// Wait for thread to rejoin.
	sdl->join();
	
	// Delete thread object.
	delete sdl;
	
	return true;
}


// --- QUIT ---
bool Gui::quit() {
	SdlRenderer::quitGui();
	
	return true;
}
