/*
	gui_event.cpp - implementation of the GuiEvent class.
	
	Revision 0
	
	Notes:
			- 
			
	2021/10/15, Maya Posch
*/


#include "gui_event.h"

#include "gui/core/InputManager.h"
#include "gui/app/views/ViewController.h"


// --- SET ITEM ---
void GuiEvent::setItem(FileData* item, Window* window) { 
	this->item = item;
	this->window = window;
}


// --- PROCESS ---
void GuiEvent::process() {
	item->launchItem(window);
}


// --- FINISH ---
void GuiEvent::finish() {
	// Call own destructor.
	delete this;
}
