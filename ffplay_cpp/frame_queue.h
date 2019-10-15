

#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H


#include "types.h"


class FrameQueueC {
	FrameQueue *f;
	
public:
	static int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);
	static void frame_queue_destroy(FrameQueue *f);
	static void frame_queue_signal(FrameQueue *f);
	static Frame *frame_queue_peek(FrameQueue *f);
	static Frame *frame_queue_peek_next(FrameQueue *f);
	static Frame *frame_queue_peek_last(FrameQueue *f);
	static Frame *frame_queue_peek_writable(FrameQueue *f);
	static Frame *frame_queue_peek_readable(FrameQueue *f);
	static void frame_queue_push(FrameQueue *f);
	static void frame_queue_next(FrameQueue *f);
	static int frame_queue_nb_remaining(FrameQueue *f);
	static int64_t frame_queue_last_pos(FrameQueue *f);
};

#endif
