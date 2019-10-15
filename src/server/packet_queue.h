

#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H


#include "types.h"


class PacketQueueC {
	PacketQueue *f;
	
public:
	static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt);
	static int packet_queue_put(PacketQueue *q, AVPacket *pkt);
	static int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);
	static int packet_queue_init(PacketQueue *q);
	static void packet_queue_flush(PacketQueue *q);
	static void packet_queue_destroy(PacketQueue *q);
	static void packet_queue_abort(PacketQueue *q);
	static void packet_queue_start(PacketQueue *q);
	static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);
};

#endif
