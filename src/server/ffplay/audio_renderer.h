

#ifndef AUDIO_RENDERER_H
#define AUDIO_RENDERER_H


#include "types.h"


#include <atomic>


class AudioRenderer {
	static std::atomic<bool> run;
	
public:
	//static int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params);
	static int audio_open(void *opaque, AVChannelLayout* wanted_channel_layout, int wanted_sample_rate, struct AudioParams *audio_hw_params);
	static int audio_thread(void *arg);
	static int configure_audio_filters(VideoState *is, const char *afilters, int force_output_format);
	
	static void quit();
};


#endif