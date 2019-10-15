

#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H


#include "types.h"


class VideoRenderer {
	//
	
public:
	static int video_thread(void *arg);
	static void video_refresh(void *opaque, double *remaining_time);
};


#endif