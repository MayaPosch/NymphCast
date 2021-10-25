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
void GuiEvent::setItem(FileData* item, Vector3f center) { 
	this->item = item;
	this->center = center;
	single = false;
}


void GuiEvent::setItem(FileData* item) { 
	this->item = item;
	this->center = center;
	single = true;
}


// --- PROCESS ---
void GuiEvent::process() {
	if (single) {
		ViewController::get()->launchItem(item);
	}
	else {
		ViewController::get()->launchItem(item, center);
	}
}


// --- FINISH ---
void GuiEvent::finish() {
	// Call own destructor.
	delete this;
}
