

#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"

#include <atomic>


class Player {
	VideoState state;
	//Config config;
	
	static std::atomic_bool run;
	
public:
	Player();
	bool start(Config config);

	static void quit();
	static void event_loop(VideoState *cur_stream);
};


#endif
