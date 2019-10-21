

#ifndef STREAM_HANDLER_H
#define STREAM_HANDLER_H


#include "types.h"

#include <atomic>


class StreamHandler {
	VideoState* vstate;
	static std::atomic_bool run;
	
	static int read_thread(void *arg);
	
public:
	static VideoState *stream_open(const char *filename, AVInputFormat *iformat, AVFormatContext* context);
	static int stream_component_open(VideoState *is, int stream_index);
	static void stream_close(VideoState *is);
	static int get_master_sync_type(VideoState *is);
	static void stream_toggle_pause(VideoState *is);
	static void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes);
	static void step_to_next_frame(VideoState *is);
	static void stream_cycle_channel(VideoState *is, int codec_type);
#if CONFIG_AVFILTER
	static int opt_add_vfilter(void *optctx, const char *opt, const char *arg);
	
	static void quit();
#endif
};


#endif
