

#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H


#include "types.h"

#include <atomic>


class VideoRenderer {
	static std::atomic<bool> run;
	
public:
	static int video_thread(void *arg);
	static void video_refresh(void *opaque, double *remaining_time);
	
	static void quit();
};


#endif