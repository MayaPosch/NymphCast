/*
	gui.h - Source file for the GUI module.
	
	Revision 0
	
	2021/06/01, Maya Posch
*/


#include "gui.h"


#include "ffplay/sdl_renderer.h"

#include <GL/glew.h>


bool Gui::init(std::string document) {
	SdlRenderer::initGui(document);
	
	/* GLenum err = glewInit();

	if (err != GLEW_OK) {
		fprintf(stderr, "GLEW ERROR: %s\n", glewGetErrorString(err));
		return false;
	}
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	glMatrixMode(GL_PROJECTION | GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, window_width, window_height, 0, 0, 1);

	RmlUiSDL2Renderer Renderer(SdlRenderer::renderer, SdlRenderer::window);
	RmlUiSDL2SystemInterface SystemInterface; */
	
	return true;
}


// --- START ---
bool Gui::start() {
	SdlRenderer::run_gui_loop();
	
	return true;
}


// --- STOP ---
bool Gui::stop() {
	SdlRenderer::stop_gui_loop();
	
	return true;
}


// --- QUIT ---
bool Gui::quit() {
	SdlRenderer::quitGui();
	
	return true;
}
