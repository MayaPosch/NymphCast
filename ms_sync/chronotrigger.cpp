/*
	chronotrigger.cpp - Source for the ChronoTrigger class.
	
	Revision 0
	
	Features:
			- Simple class that implements a periodic, resettable timer.
			
	Notes:
			- 
			
	2019/09/19 - Maya Posch
*/


#include "chronotrigger.h"


void ChronoTrigger::setCallback(std::function<void(int)> cb, uint32_t data) {
	this->cb = cb;
	this->data = data;
}


void ChronoTrigger::setStopCallback(std::function<void()> cb) {
	this->stopCb = cb;
	stopCbSet = true;
}


// --- START ---
// Start the processing thread.
bool ChronoTrigger::start(uint32_t interval, bool single) {
	thread = std::thread(&ChronoTrigger::run, this, interval, single);
	
	return true;
}


// --- RUN ---
// If single-shot is set, wait for the 'interval' time period, then stop.
// Else, loop until finish() or stop() is called.
// Interval is in milliseconds.
void ChronoTrigger::run(uint32_t interval, bool single) {
	running = true;
	signaled = false;
	restarting = false;
	stopping = false;
	while (true) {
		// Wait in the condition variable until the wait ends, or the condition is signalled.
		startTime = std::chrono::steady_clock::now();
		endTime = startTime + std::chrono::milliseconds(interval);
		while (std::chrono::steady_clock::now() < endTime) {
			// Loop to deal with spurious wake-ups.
			std::unique_lock<std::mutex> lk(mutex);
			if (cv.wait_until(lk, endTime) == std::cv_status::timeout) { break; }
			if (signaled) { signaled = false; break; }
		}
		
		// Check flags.
		if (restarting) {
			// Reset, return to beginning of loop.
			running = true;
			signaled = false;
			restarting = false;
			stopping = false;
			continue;
		}
		
		if (stopping) { // Stop was called.
			if (stopCbSet) { stopCb(); }			
			break;
		}
		
		// Call the callback with the user data.
		cb(data);
		
		// Exit if single-shot is true.
		if (single) { break; }
		
		// Exit if finish() has been called.
		if (!running) { break; }
	}
}


// --- RESTART ---
void ChronoTrigger::restart() {
	restarting = true;
	signaled = true;
	cv.notify_all();
}


// --- FINISH ---
// Allow the timer to finish and trigger the callback one last time before stopping.
void ChronoTrigger::finish() {
	running = false;
}


// --- STOP ---
// Signal the condition variable, end the timer.
void ChronoTrigger::stop() {
	signaled = true;
	stopping = true;
	cv.notify_all();
	
	thread.join();
}
