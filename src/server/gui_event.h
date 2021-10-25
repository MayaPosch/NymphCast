/*
	gui_event.h - header file for the GuiEvent class.
	
	Revision 0
	
	Notes:
			- 
			
	2021/10/15, Maya Posch
*/


#ifndef GUI_EVENT_H
#define GUI_EVENT_H


#include <nymph/abstract_request.h>

#include <SDL_events.h>

#include "gui/app/FileData.h"
#include "gui/core/Window.h"

#include <string>
#include <atomic>
#include <condition_variable>
#include <mutex>


class GuiEvent : public AbstractRequest {
	FileData* item;
	Window* window;
	std::string loggerName;
	
public:
	GuiEvent() { loggerName = "GUI_Event"; }
	void setItem(FileData* item, Window* window);
	void process();
	void finish();
};

#endif
