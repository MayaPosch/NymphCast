/*
	chronotrigger.h - Header for the ChronoTrigger class.
	
	Revision 0
	
	Features:
			- Simple class that implements a periodic, resettable timer.
			
	Notes:
			- 
			
	2019/09/19 - Maya Posch
*/


#ifndef CHRONO_TRIGGER_H
#define CHRONO_TRIGGER_H


#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>


class ChronoTrigger {
	std::thread thread;
	std::mutex mutex;
	std::condition_variable cv;
	std::atomic_bool running;
	std::atomic_bool restarting;
	std::atomic_bool signaled;
	std::atomic_bool stopping;
	std::function<void(uint32_t)> cb;
	uint32_t data;
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	std::chrono::time_point<std::chrono::steady_clock> endTime;
	
	void run(uint32_t interval, bool single);
	
public:
	void setCallback(std::function<void(int)> cb, uint32_t data);
	bool start(uint32_t interval, bool single = false);
	void restart();
	void finish();
	void stop();
};

#endif
