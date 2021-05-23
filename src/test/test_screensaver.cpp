/*
	Simple screensaver test.
*/


#include "../server/screensaver.h"

#include <thread>

#ifdef main
#undef main
#endif

class SdlRenderer {
	static std::atomic<bool> run_events;
	
public:
	static bool init();
	static void quit();
	static void run_event_loop();
	static void stop_event_loop();
};


int main() {
	// Start the ScreenSaver, let it run for a bit, then terminate it.
	// Set transition time to 1 second.
	ScreenSaver::start(1);
	
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(30s);
	
	ScreenSaver::stop();
	
	return 0;
}
