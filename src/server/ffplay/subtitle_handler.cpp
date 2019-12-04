

#include "subtitle_handler.h"

#include "frame_queue.h"
#include "decoder.h"


int SubtitleHandler::subtitle_thread(void *arg) {
    VideoState *is = (VideoState*) arg;
    Frame *sp;
    int got_subtitle;
    double pts;

    for (;;) {
        if (!(sp = FrameQueueC::frame_queue_peek_writable(&is->subpq)))
            return 0;

        if ((got_subtitle = DecoderC::decoder_decode_frame(&is->subdec, NULL, &sp->sub)) < 0)
            break;

        pts = 0;

        if (got_subtitle && sp->sub.format == 0) {
            if (sp->sub.pts != AV_NOPTS_VALUE)
                pts = sp->sub.pts / (double)AV_TIME_BASE;
            sp->pts = pts;
            sp->serial = is->subdec.pkt_serial;
            sp->width = is->subdec.avctx->width;
            sp->height = is->subdec.avctx->height;
            sp->uploaded = 0;

            /* now we can update the picture count */
            FrameQueueC::frame_queue_push(&is->subpq);
        } else if (got_subtitle) {
            avsubtitle_free(&sp->sub);
        }
    }
    return 0;
}
