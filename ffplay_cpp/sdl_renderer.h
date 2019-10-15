

#ifndef SDL_RENDERER_H
#define SDL_RENDERER_H

#include "types.h"


class SdlRenderer {
	//
	
public:
	static void set_default_window_size(int width, int height, AVRational sar);
	static void video_audio_display(VideoState *s);
	static void video_image_display(VideoState *is);
};


#endif