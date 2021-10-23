

#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"

#include <atomic>


class Player {
	static VideoState* cur_stream;
	//Config config;
	
	static std::atomic_bool run;
	static double remaining_time;
	
public:
	Player();
	bool start(Config config);
	
	static void setVideoState(VideoState* vs);
	
	static void quit();
	static void event_loop(VideoState *cur_stream);
	static void refresh_loop(VideoState* is);
	static bool process_event(SDL_Event &event);
	static void run_updates();
};


#endif
