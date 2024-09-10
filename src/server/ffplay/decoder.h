

#ifndef DECODER_H
#define DECODER_H


#include "types.h"


class DecoderC {
	//
	
public:
	static int decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond);
	static int decoder_start(Decoder *d, int (*fn)(void *), const char *thread_name, void* arg);
	static void decoder_abort(Decoder *d, FrameQueue *fq);
	static void decoder_destroy(Decoder *d);
	static int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub);
};


#endif