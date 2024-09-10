

#include "frame_queue.h"
#include "packet_queue.h"
#include "clock.h"
#include "audio_renderer.h"
#include "decoder.h"
#include "subtitle_handler.h"
#include "video_renderer.h"
#include "sdl_renderer.h"
#include "player.h"
#include "ffplay.h"
#include "../databuffer.h"

#include "stream_handler.h"

// Enable profiling.
//#define PROFILING_SH 1
#ifdef PROFILING_SH
#include <chrono>
#include <fstream>
std::ofstream debugfile;
uint32_t profcount = 0;
#endif


// Static initialisations.
std::atomic_bool StreamHandler::run;
std::atomic<bool> StreamHandler::eof;

AVDictionary *sws_dict;
AVDictionary *swr_opts;
AVDictionary *format_opts, *codec_opts, *resample_opts;

#define GROW_ARRAY(array, nb_elems)\
    array = (const char**) grow_array(array, sizeof(*array), &nb_elems, nb_elems + 1)
	
void* grow_array(void *array, int elem_size, int *size, int new_size) {
    if (new_size >= INT_MAX / elem_size) {
        av_log(NULL, AV_LOG_ERROR, "Array too big.\n");
        return 0;
    }
	
    if (*size < new_size) {
        uint8_t* tmp = (uint8_t*) av_realloc_array(array, new_size, elem_size);
        if (!tmp) {
            av_log(NULL, AV_LOG_ERROR, "Could not alloc buffer.\n");
            return 0;
        }
		
        memset(tmp + *size*elem_size, 0, (new_size-*size) * elem_size);
        *size = new_size;
        return tmp;
    }
	
    return array;
}

void print_error(const char *filename, int err) {
    char errbuf[128];
    const char *errbuf_ptr = errbuf;

    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0) {
        errbuf_ptr = strerror(AVUNERROR(err));
	}
	
    av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, errbuf_ptr);
}


int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec) {
    int ret = avformat_match_stream_specifier(s, st, spec);
    if (ret < 0) { av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec); }
    return ret;
}


AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st, AVCodec *codec)
{
    AVDictionary    *ret = NULL;
    AVDictionaryEntry *t = NULL;
    int            flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM
                                      : AV_OPT_FLAG_DECODING_PARAM;
    char          prefix = 0;
    const AVClass    *cc = avcodec_get_class();

    if (!codec) {
        codec = (AVCodec*) ((s->oformat) ? avcodec_find_encoder(codec_id) 
			: avcodec_find_decoder(codec_id));
	}

    switch (st->codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        prefix  = 'v';
        flags  |= AV_OPT_FLAG_VIDEO_PARAM;
        break;
    case AVMEDIA_TYPE_AUDIO:
        prefix  = 'a';
        flags  |= AV_OPT_FLAG_AUDIO_PARAM;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        prefix  = 's';
        flags  |= AV_OPT_FLAG_SUBTITLE_PARAM;
        break;
    }

    while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) {
        char *p = strchr(t->key, ':');

        /* check stream specification in opt name */
        if (p)
            switch (check_stream_specifier(s, st, p + 1)) {
            case  1: *p = 0; break;
            case  0:         continue;
            default:         break;	// TODO: check for correctness.
            }

        if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
            !codec ||
            (codec->priv_class &&
             av_opt_find(&codec->priv_class, t->key, NULL, flags,
                         AV_OPT_SEARCH_FAKE_OBJ)))
            av_dict_set(&ret, t->key, t->value, 0);
        else if (t->key[0] == prefix &&
                 av_opt_find(&cc, t->key + 1, NULL, flags,
                             AV_OPT_SEARCH_FAKE_OBJ))
            av_dict_set(&ret, t->key + 1, t->value, 0);

        if (p)
            *p = ':';
    }
    return ret;
}

AVDictionary **setup_find_stream_info_opts(AVFormatContext *s, AVDictionary *codec_opts) {
    int i;
    AVDictionary **opts;
    if (!s->nb_streams) { return NULL; }
	//opts = (AVDictionary**) av_mallocz_array(s->nb_streams, sizeof(*opts));
	opts = (AVDictionary**) av_calloc(s->nb_streams, sizeof(*opts));
    if (!opts) {
        av_log(NULL, AV_LOG_ERROR, "Could not alloc memory for stream options.\n");
        return NULL;
    }
	
    for (i = 0; i < s->nb_streams; i++) {
        opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id,
                                    s, s->streams[i], NULL);
	}
	
    return opts;
}


#if CONFIG_AVFILTER
int StreamHandler::opt_add_vfilter(void *optctx, const char *opt, const char *arg) {
    //vfilters_list = (const char**) grow_array(vfilters_list, sizeof(*vfilters_list), &nb_vfilters, nb_vfilters + 1);
	GROW_ARRAY(vfilters_list, nb_vfilters);
    vfilters_list[nb_vfilters - 1] = arg;
    return 0;
}
#endif

static inline
int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1,
                   enum AVSampleFormat fmt2, int64_t channel_count2)
{
    /* If channel count == 1, planar and non-planar formats are the same */
    if (channel_count1 == 1 && channel_count2 == 1)
        return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
    else
        return channel_count1 != channel_count2 || fmt1 != fmt2;
}

/* static inline
int64_t get_valid_channel_layout(int64_t channel_layout, int channels)
{
    if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
        return channel_layout;
    else
        return 0;
} */


int StreamHandler::get_master_sync_type(VideoState *is) {
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st)
            return AV_SYNC_AUDIO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}

extern "C" {
#include <libavfilter/buffersink.h>
}


/* open a given stream. Return 0 if OK */
int StreamHandler::stream_component_open(VideoState *is, int stream_index) {
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    AVCodec *codec;
    const char *forced_codec_name = NULL;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = 0;
    int sample_rate; //, nb_channels;
    AVChannelLayout ch_layout = {};
    //int64_t channel_layout;
    int ret = 0;
    int stream_lowres = lowres;

    if (stream_index < 0 || stream_index >= ic->nb_streams) {
		av_log(NULL, AV_LOG_ERROR, "stream_index: %d, ic->nb_streams: %d.\n", stream_index, ic->nb_streams);
        return -1;
	}

    avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_alloc_context3() failed.\n");
        return AVERROR(ENOMEM);
	}

    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_to_context() failed.\n");
        goto fail;
	}
	
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;

    codec = (AVCodec*) avcodec_find_decoder(avctx->codec_id);

    switch(avctx->codec_type){
        case AVMEDIA_TYPE_AUDIO   : is->last_audio_stream    = stream_index; forced_codec_name =    audio_codec_name; break;
        case AVMEDIA_TYPE_SUBTITLE: is->last_subtitle_stream = stream_index; forced_codec_name = subtitle_codec_name; break;
        case AVMEDIA_TYPE_VIDEO   : is->last_video_stream    = stream_index; forced_codec_name =    video_codec_name; break;
    }
	
    if (forced_codec_name)
        codec = (AVCodec*) avcodec_find_decoder_by_name(forced_codec_name);
	
    if (!codec) {
        if (forced_codec_name) av_log(NULL, AV_LOG_WARNING,
                                      "No codec could be found with name '%s'\n", forced_codec_name);
        else                   av_log(NULL, AV_LOG_WARNING,
                                      "No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
        ret = AVERROR(EINVAL);
        goto fail;
    }

    avctx->codec_id = codec->id;
    if (stream_lowres > codec->max_lowres) {
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
                codec->max_lowres);
        stream_lowres = codec->max_lowres;
    }
	
    avctx->lowres = stream_lowres;

    if (fast)
        avctx->flags2 |= AV_CODEC_FLAG2_FAST;

    opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
	
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
	
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
	
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_open2() failed.\n");
        goto fail;
    }
	
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX)) && t == 0) {
        av_log(NULL, AV_LOG_ERROR, "Option not found.\n");
        ret =  AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }

    is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
#if CONFIG_AVFILTER
        {
            AVFilterContext *sink;

            is->audio_filter_src.freq           = avctx->sample_rate;
            //is->audio_filter_src.channels       = avctx->channels;
            //is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
			ret = av_channel_layout_copy(&is->audio_filter_src.ch_layout, &avctx->ch_layout);
            if (ret < 0)
                goto fail;
            is->audio_filter_src.fmt            = avctx->sample_fmt;
            if ((ret = AudioRenderer::configure_audio_filters(is, afilters, 0)) < 0) {
				av_log(NULL, AV_LOG_ERROR, "Failed to configure audio filters: %d.\n", ret);
                goto fail;
			}
			
            sink = is->out_audio_filter;
            sample_rate    = av_buffersink_get_sample_rate(sink);
            //nb_channels    = av_buffersink_get_channels(sink);
            //channel_layout = av_buffersink_get_channel_layout(sink);
			ret = av_buffersink_get_ch_layout(sink, &ch_layout);
            if (ret < 0)
                goto fail;
        }
#else
        sample_rate    = avctx->sample_rate;
        nb_channels    = avctx->channels;
        channel_layout = avctx->channel_layout;
#endif

        /* prepare audio output */
        //if ((ret = AudioRenderer::audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0) {
		if ((ret = AudioRenderer::audio_open(is, &ch_layout, sample_rate, &is->audio_tgt)) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Failed to open audio output.\n");
            goto fail;
		}
		
        is->audio_hw_buf_size = ret;
        is->audio_src = is->audio_tgt;
        is->audio_buf_size  = 0;
        is->audio_buf_index = 0;

        /* init averaging filter */
        is->audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
        is->audio_diff_avg_count = 0;
        /* since we do not have a precise anough audio FIFO fullness,
           we correct audio sync only if larger than this threshold */
        is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

        is->audio_stream = stream_index;
        is->audio_st = ic->streams[stream_index];

        if ((ret = DecoderC::decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread)) < 0)
            goto fail;
		
        //if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek) {
		if (is->ic->iformat->flags & AVFMT_NOTIMESTAMPS) {
            is->auddec.start_pts = is->audio_st->start_time;
            is->auddec.start_pts_tb = is->audio_st->time_base;
        }
        if ((ret = DecoderC::decoder_start(&is->auddec, AudioRenderer::audio_thread, "audio_decoder", is)) < 0)
            goto out;
        SDL_PauseAudioDevice(audio_dev, 0);
        break;
    case AVMEDIA_TYPE_VIDEO:
        is->video_stream = stream_index;
        is->video_st = ic->streams[stream_index];

        DecoderC::decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
        if ((ret = DecoderC::decoder_start(&is->viddec, VideoRenderer::video_thread, "video_decoder", is)) < 0)
            goto out;
        is->queue_attachments_req = 1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        is->subtitle_stream = stream_index;
        is->subtitle_st = ic->streams[stream_index];

        DecoderC::decoder_init(&is->subdec, avctx, &is->subtitleq, is->continue_read_thread);
        if ((ret = DecoderC::decoder_start(&is->subdec, SubtitleHandler::subtitle_thread, "subtitle_decoder", is)) < 0)
            goto out;
        break;
    default:
        break;
    }
    goto out;

fail:
    avcodec_free_context(&avctx);
out:
    av_dict_free(&opts);

    return ret;
}

static void stream_component_close(VideoState *is, int stream_index)
{
    AVFormatContext *ic = is->ic;
    AVCodecParameters *codecpar;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return;
    codecpar = ic->streams[stream_index]->codecpar;

    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        DecoderC::decoder_abort(&is->auddec, &is->sampq);
		av_log(NULL, AV_LOG_INFO, "Closing audio device...\n");
        SDL_CloseAudioDevice(audio_dev);
        DecoderC::decoder_destroy(&is->auddec);
        swr_free(&is->swr_ctx);
        av_freep(&is->audio_buf1);
        is->audio_buf1_size = 0;
        is->audio_buf = NULL;

        if (is->rdft) {
            av_rdft_end(is->rdft);
            av_freep(&is->rdft_data);
            is->rdft = NULL;
            is->rdft_bits = 0;
        }
        break;
    case AVMEDIA_TYPE_VIDEO:
        DecoderC::decoder_abort(&is->viddec, &is->pictq);
        DecoderC::decoder_destroy(&is->viddec);
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        DecoderC::decoder_abort(&is->subdec, &is->subpq);
        DecoderC::decoder_destroy(&is->subdec);
        break;
    default:
        break;
    }

    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        is->audio_st = NULL;
        is->audio_stream = -1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        is->video_st = NULL;
        is->video_stream = -1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        is->subtitle_st = NULL;
        is->subtitle_stream = -1;
        break;
    default:
        break;
    }
}

void StreamHandler::stream_close(VideoState *is) {
    /* XXX: use a special url_shutdown call to abort parse cleanly */
    is->abort_request = 1;
    SDL_WaitThread(is->read_tid, NULL);

    /* close each stream */
    if (is->audio_stream >= 0)
		av_log(NULL, AV_LOG_INFO, "Closing audio stream component...\n");
        stream_component_close(is, is->audio_stream);
    if (is->video_stream >= 0)
        stream_component_close(is, is->video_stream);
    if (is->subtitle_stream >= 0)
        stream_component_close(is, is->subtitle_stream);

    avformat_close_input(&is->ic);

    PacketQueueC::packet_queue_destroy(&is->videoq);
    PacketQueueC::packet_queue_destroy(&is->audioq);
    PacketQueueC::packet_queue_destroy(&is->subtitleq);

    /* free all pictures */
    FrameQueueC::frame_queue_destroy(&is->pictq);
    FrameQueueC::frame_queue_destroy(&is->sampq);
    FrameQueueC::frame_queue_destroy(&is->subpq);
    SDL_DestroyCond(is->continue_read_thread);
    sws_freeContext(is->img_convert_ctx);
    sws_freeContext(is->sub_convert_ctx);
    av_free(is->filename);
    if (is->vis_texture)
        SDL_DestroyTexture(is->vis_texture);
    if (is->vid_texture)
        SDL_DestroyTexture(is->vid_texture);
    if (is->sub_texture)
        SDL_DestroyTexture(is->sub_texture);
    av_free(is);
	is = 0;
}


static int decode_interrupt_cb(void *ctx) {
    VideoState *is = (VideoState*) ctx;
    return is->abort_request;
}

static int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
    return stream_id < 0 ||
           queue->abort_request ||
           (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
           queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

static int is_realtime(AVFormatContext *s)
{
    if(   !strcmp(s->iformat->name, "rtp")
       || !strcmp(s->iformat->name, "rtsp")
       || !strcmp(s->iformat->name, "sdp")
    )
        return 1;

    if(s->pb && (   !strncmp(s->url, "rtp:", 4)
                 || !strncmp(s->url, "udp:", 4)
                )
    )
        return 1;
    return 0;
}

/* pause or resume the video */
void StreamHandler::stream_toggle_pause(VideoState *is) {
    if (is->paused) {
		av_log(NULL, AV_LOG_INFO, "Toggle: currently paused.\n");
        is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
        if (is->read_pause_return != AVERROR(ENOSYS)) {
            is->vidclk.paused = 0;
        }
        ClockC::set_clock(&is->vidclk, ClockC::get_clock(&is->vidclk), is->vidclk.serial);
    }
	
	av_log(NULL, AV_LOG_INFO, "Toggle: toggling paused state.\n");
    ClockC::set_clock(&is->extclk, ClockC::get_clock(&is->extclk), is->extclk.serial);
    is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

/* seek in the stream */
void StreamHandler::stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes) {
	av_log(NULL, AV_LOG_INFO, "StreamHandler::stream_seek called.\n");
    if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_rel = rel;
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        is->seek_req = 1;
        SDL_CondSignal(is->continue_read_thread);
    }
}

void StreamHandler::step_to_next_frame(VideoState *is) {
    /* if the stream is paused unpause it, then step */
    if (is->paused)
        stream_toggle_pause(is);
    is->step = 1;
}


static double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}

/* static void update_video_pts(VideoState *is, double pts, int64_t pos, int serial) {
    // update current video pts
    ClockC::set_clock(&is->vidclk, pts, serial);
    ClockC::sync_clock_to_slave(&is->extclk, &is->vidclk);
} */


/* this thread gets the stream from the disk or the network */
int StreamHandler::read_thread(void *arg) {
#ifdef PROFILING_SH
	if (!debugfile.is_open()) {
		debugfile.open("profiling_read_thread.txt");
	}
#endif

    VideoState *is = (VideoState*) arg;
	AVFormatContext* ic = is->ic;
	
    int err, i, ret;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket pkt1, *pkt = &pkt1;
    int64_t stream_start_time;
    int pkt_in_play_range = 0;
    AVDictionaryEntry *t;
    SDL_mutex *wait_mutex = SDL_CreateMutex();
    int scan_all_pmts_set = 0;
    int64_t pkt_ts;

    if (!wait_mutex) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    memset(st_index, -1, sizeof(st_index));
    is->last_video_stream = is->video_stream = -1;
    is->last_audio_stream = is->audio_stream = -1;
    is->last_subtitle_stream = is->subtitle_stream = -1;
    is->eof = 0;

    if (ic == 0) {
		ic = avformat_alloc_context();
		if (!ic) {
			av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
			ret = AVERROR(ENOMEM);
			goto fail;
		}
		
		ic->interrupt_callback.callback = decode_interrupt_cb;
		ic->interrupt_callback.opaque = is;
		is->ic = ic;
	}
	
    if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }
	
	// Open the input file or stream.
	// If in slave mode, ignore any errors here.
#ifdef __ANDROID__
    err = avformat_open_input(&ic, is->filename, is->iformat, NULL);
#else
    err = avformat_open_input(&ic, is->filename, is->iformat, &format_opts);
#endif
    if (err < 0) {
        print_error(is->filename, err);
		if (serverMode != NCS_MODE_SLAVE) {
			ret = -1;
			goto fail;
		}
    }
	
	// Log stream info.
#ifdef __ANDROID__
	av_log(NULL, AV_LOG_INFO, "Format %s, duration %lld us", ic->iformat->name, ic->duration);
#else
	av_log(NULL, AV_LOG_INFO, "Format %s, duration %lld us", ic->iformat->long_name, ic->duration);
#endif
	
    if (scan_all_pmts_set)
        av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

    if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret = AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }

    if (genpts)
        ic->flags |= AVFMT_FLAG_GENPTS;

    av_format_inject_global_side_data(ic);

    if (find_stream_info) {
#ifdef __ANDROID__
		// FIXME: skipping setup_find_stream_info_opts due to segfault. Similar to issue on ESP32. 

		// Extract info on the individual audio/video & subtitle streams.
        err = avformat_find_stream_info(ic, NULL);
#else
        AVDictionary **opts = setup_find_stream_info_opts(ic, codec_opts);
        int orig_nb_streams = ic->nb_streams;

		// Extract info on the individual audio/video & subtitle streams.
        err = avformat_find_stream_info(ic, opts);

        for (i = 0; i < orig_nb_streams; i++) { av_dict_free(&opts[i]); }
        av_freep(&opts);
#endif

        if (err < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not find codec parameters\n", is->filename);
            ret = -1;
            goto fail;
        }
    }

    if (ic->pb) {
        ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
	}

    if (seek_by_bytes < 0) {
        seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);
	}

    is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    if (!window_title && (t = av_dict_get(ic->metadata, "title", NULL, 0))) {
        window_title = av_asprintf("%s - %s", t->value, input_filename);
	}

    /* if seeking requested, we execute it */
    if (start_time != AV_NOPTS_VALUE) {
        int64_t timestamp;

        timestamp = start_time;
        // add the stream start time
        if (ic->start_time != AV_NOPTS_VALUE)
            timestamp += ic->start_time;
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                    is->filename, (double)timestamp / AV_TIME_BASE);
        }
    }

    is->realtime = is_realtime(ic);

    if (show_status) {
        av_dump_format(ic, 0, is->filename, 0);
	}

    for (i = 0; i < ic->nb_streams; i++) {
        AVStream *st = ic->streams[i];
        enum AVMediaType type = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
        if (type >= 0 && wanted_stream_spec[type] && st_index[type] == -1) {
            if (avformat_match_stream_specifier(ic, st, wanted_stream_spec[type]) > 0) {
                st_index[type] = i;
			}
		}
    }
	
    for (i = 0; i < AVMEDIA_TYPE_NB; i++) {
        if (wanted_stream_spec[i] && st_index[i] == -1) {
            av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", wanted_stream_spec[i], av_get_media_type_string((AVMediaType) i));
            st_index[i] = INT_MAX;
        }
    }

    if (!video_disable) {
        st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(ic, 
															AVMEDIA_TYPE_VIDEO,
															st_index[AVMEDIA_TYPE_VIDEO], 
															-1, NULL, 0);
	}
								
    if (!audio_disable) {
		av_log(NULL, AV_LOG_WARNING, "Finding audio stream...\n");
        st_index[AVMEDIA_TYPE_AUDIO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                                st_index[AVMEDIA_TYPE_AUDIO],
                                st_index[AVMEDIA_TYPE_VIDEO],
                                NULL, 0);
		if (st_index[AVMEDIA_TYPE_AUDIO] == AVERROR_STREAM_NOT_FOUND) {
			// No stream found with this type.
			av_log(NULL, AV_LOG_WARNING, "No stream found with type 'audio'.\n");
		}
		else if (st_index[AVMEDIA_TYPE_AUDIO] == AVERROR_DECODER_NOT_FOUND) {
			// Streams were found, but no decoder.
			av_log(NULL, AV_LOG_WARNING, "No decoder was found for the found audio stream.\n");
		}
		else if (st_index[AVMEDIA_TYPE_AUDIO] < 0) {
			// General error.
			av_log(NULL, AV_LOG_WARNING, "av_find_best_stream() general error.\n");
		}
		else {
			av_log(NULL, AV_LOG_WARNING, "Found audio stream: %d.\n", st_index[AVMEDIA_TYPE_AUDIO]);
		}
	}
	
    if (!video_disable && !subtitle_disable)
        st_index[AVMEDIA_TYPE_SUBTITLE] =
            av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
                                st_index[AVMEDIA_TYPE_SUBTITLE],
                                (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                 st_index[AVMEDIA_TYPE_AUDIO] :
                                 st_index[AVMEDIA_TYPE_VIDEO]),
                                NULL, 0);

    is->show_mode = show_mode;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        AVStream *st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        AVCodecParameters *codecpar = st->codecpar;
        AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
        if (codecpar->width)
            SdlRenderer::set_default_window_size(codecpar->width, codecpar->height, sar);
    }

    /* open the streams */
	av_log(NULL, AV_LOG_INFO, "A: %d, V: %d, S: %d\n", st_index[AVMEDIA_TYPE_AUDIO],
													st_index[AVMEDIA_TYPE_VIDEO],
													st_index[AVMEDIA_TYPE_SUBTITLE]);
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        if (StreamHandler::stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]) != 0) {
			av_log(NULL, AV_LOG_ERROR, "Failed to open audio stream.");
		}
    }

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = StreamHandler::stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
    }
	
    if (is->show_mode == SHOW_MODE_NONE) {
        is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;
	}

    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
        StreamHandler::stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
    }

    if (is->video_stream < 0 && is->audio_stream < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n",
               is->filename);
        ret = -1;
        goto fail;
    }

    if (infinite_buffer < 0 && is->realtime) { infinite_buffer = 1; }
	
	
	// Set new title after clearing it.
	file_meta.setTitle("");
	if (t = av_dict_get(ic->metadata, "title", NULL, 0)) {
		file_meta.setTitle(t->value);
	}

	file_meta.setArtist(""); // clear old artist.
    if (t = av_dict_get(ic->metadata, "author", NULL, 0)) {
		file_meta.setArtist(t->value);
	}
    else if (t = av_dict_get(ic->metadata, "artist", NULL, 0)) {
		file_meta.setArtist(t->value);
	}
	
	file_meta.setDuration(is->ic->duration / AV_TIME_BASE); // Convert to seconds.
	
	// Update clients with status.
	sendGlobalStatusUpdate();
	
	// Buffering ahead can now commence as all headers etc. should have been seeked to.
	DataBuffer::startBufferAhead();
	
	// TODO: Start slave playback here if we're in master mode. In slave mode wait here.
	if (serverMode == NCS_MODE_SLAVE) {
		slavePlayMutex.lock();
		slavePlayCon.wait(slavePlayMutex);
		slavePlayMutex.unlock();
	}
	else if (serverMode == NCS_MODE_MASTER) {
		//
		startSlavePlayback();
	}

	// Start the main processing loop.
	run = true;
	eof = false;
	while (run) {
        if (is->abort_request) { break; }
        if (is->paused != is->last_paused) {
            is->last_paused = is->paused;
            if (is->paused) { is->read_pause_return = av_read_pause(ic); }
            else { av_read_play(ic); }
        }
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
        if (is->paused && 
                (!strcmp(ic->iformat->name, "rtsp") ||
                 (ic->pb && !strncmp(input_filename, "mmsh:", 5)))) {
            /* wait 10 ms to avoid trying to get another packet */
            /* XXX: horrible */
            SDL_Delay(10);
            continue;
        }
#endif
        if (is->seek_req) {
			av_log(NULL, AV_LOG_INFO, "Seek request: %d, target: %d\n", is->seek_rel, is->seek_pos);
            int64_t seek_target = is->seek_pos;
            int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: INT64_MIN;
            int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: INT64_MAX;
// FIXME the +-2 is due to rounding being not done in the correct direction in generation
//      of the seek_pos/seek_rel variables

            ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR,
                       "%s: error while seeking\n", is->ic->url);
            } else {
                if (is->audio_stream >= 0) {
                    PacketQueueC::packet_queue_flush(&is->audioq);
                    PacketQueueC::packet_queue_put(&is->audioq, &flush_pkt);
                }
                if (is->subtitle_stream >= 0) {
                    PacketQueueC::packet_queue_flush(&is->subtitleq);
                    PacketQueueC::packet_queue_put(&is->subtitleq, &flush_pkt);
                }
                if (is->video_stream >= 0) {
                    PacketQueueC::packet_queue_flush(&is->videoq);
                    PacketQueueC::packet_queue_put(&is->videoq, &flush_pkt);
                }
                if (is->seek_flags & AVSEEK_FLAG_BYTE) {
                   ClockC::set_clock(&is->extclk, NAN, 0);
                } else {
                   ClockC::set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
                }
            }
            is->seek_req = 0;
            is->queue_attachments_req = 1;
            is->eof = 0;
            if (is->paused) {
                StreamHandler::step_to_next_frame(is);
			}
			
			// Send update to remotes.
			sendGlobalStatusUpdate();
        }
		
        if (is->queue_attachments_req) {
            if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket copy = { 0 };
                if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0)
                    goto fail;
                PacketQueueC::packet_queue_put(&is->videoq, &copy);
                PacketQueueC::packet_queue_put_nullpacket(&is->videoq, is->video_stream);
            }
            is->queue_attachments_req = 0;
        }

        /* if the queue are full, no need to read more */
        if (infinite_buffer<1 &&
              (is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
            || (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
                stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
                stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq)))) {
            /* wait 10 ms */
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
			
            continue;
        }
		
        if (!is->paused &&
            (!is->audio_st || (is->auddec.finished == is->audioq.serial && FrameQueueC::frame_queue_nb_remaining(&is->sampq) == 0)) &&
            (!is->video_st || (is->viddec.finished == is->videoq.serial && FrameQueueC::frame_queue_nb_remaining(&is->pictq) == 0))) {
            if (loop != 1 && (!loop || --loop)) {
                StreamHandler::stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0);
            } 
			else if (autoexit) {
				av_log(NULL, AV_LOG_INFO, "Auto-exit is true. Exiting...\n");
                ret = AVERROR_EOF;
                goto fail;
            }
			
			av_log(NULL, AV_LOG_INFO, "Would have quit here if auto-exit was enabled.\n");
        }
		
#ifdef PROFILING_SH
		debugfile << "Reading frame.\t";
		std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
#endif

        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
			// FIXME: hack.
			if (ret == AVERROR_EOF) { eof = true; break; }
			
			av_log(NULL, AV_LOG_WARNING, "av_read_frame() returned <0, no EOF.\n");
			
			// EOF or error. 
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0)
                    PacketQueueC::packet_queue_put_nullpacket(&is->videoq, is->video_stream);
                if (is->audio_stream >= 0)
                    PacketQueueC::packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
                if (is->subtitle_stream >= 0)
                    PacketQueueC::packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
                is->eof = 1;
            }
			
            if (ic->pb && ic->pb->error) { break; }
            SDL_LockMutex(wait_mutex);
            SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
            SDL_UnlockMutex(wait_mutex);
            continue;
        } else {
            is->eof = 0;
        }

#ifdef PROFILING_SH
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		debugfile << "Duration: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "µs.\n";
#endif
		
        /* check if packet is in play range specified by user, then queue, otherwise discard */
        stream_start_time = ic->streams[pkt->stream_index]->start_time;
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        pkt_in_play_range = duration == AV_NOPTS_VALUE ||
                (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                av_q2d(ic->streams[pkt->stream_index]->time_base) -
                (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
                <= ((double)duration / 1000000);
				
        if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
            PacketQueueC::packet_queue_put(&is->audioq, pkt);
        } 
		else if (pkt->stream_index == is->video_stream && pkt_in_play_range
                   && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            PacketQueueC::packet_queue_put(&is->videoq, pkt);
        } 
		else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
            PacketQueueC::packet_queue_put(&is->subtitleq, pkt);
        } 
		else {
            av_packet_unref(pkt);
			av_log(NULL, AV_LOG_WARNING, "Discarded packet, not in range.\n");
        }
    }

    ret = 0;
fail:
	// Disable player events.
	SdlRenderer::playerEvents(false);
	
	if (ic && !is->ic) {
		av_log(NULL, AV_LOG_INFO, "Goto 'fail': avformat_close_input()...\n");
		avformat_close_input(&ic);
	}
	
	Player::quit();
	//SDL_Event event;
	//event.type = SDL_KEYDOWN;
	//event.key.keysym.sym = SDLK_ESCAPE;
	//SDL_PushEvent(&event);
	
	av_log(NULL, AV_LOG_INFO, "Goto 'fail'. Exiting main loop...\n");
	
	AudioRenderer::quit();
	VideoRenderer::quit();
	
	// Signal the player thread that the playback has ended.
	playerCon.signal();
	
	SDL_DestroyMutex(wait_mutex);
	
#ifdef PROFILING
	if (debugfile.is_open()) {
		debugfile.flush();
		debugfile.close();
	}
#endif
	
	return 0;
}

VideoState* StreamHandler::stream_open(const char *filename, AVInputFormat *iformat, 	
																AVFormatContext* context) {
    VideoState *is;

    is = (VideoState*) av_mallocz(sizeof(VideoState));
    if (!is)
        return 0;
    is->filename = av_strdup(filename);
    if (!is->filename) {
        stream_close(is);
        return 0;
	}
	
    is->iformat = iformat;
    is->ytop    = 0;
    is->xleft   = 0;

    /* start video display */
    if (FrameQueueC::frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0) {
        stream_close(is);
        return 0;
	}
	
    if (FrameQueueC::frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0) {
        stream_close(is);
        return 0;
	}
	
    if (FrameQueueC::frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0) {
        stream_close(is);
        return 0;
	}
	

    if (PacketQueueC::packet_queue_init(&is->videoq) < 0 ||
        PacketQueueC::packet_queue_init(&is->audioq) < 0 ||
        PacketQueueC::packet_queue_init(&is->subtitleq) < 0) {
        stream_close(is);
        return 0;
	}
	

    if (!(is->continue_read_thread = SDL_CreateCond())) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
        stream_close(is);
        return 0;
    }
	
	// Set via global variable.
	int startup_volume = audio_volume;

    ClockC::init_clock(&is->vidclk, &is->videoq.serial);
    ClockC::init_clock(&is->audclk, &is->audioq.serial);
    ClockC::init_clock(&is->extclk, &is->extclk.serial);
    is->audio_clock_serial = -1;
    if (startup_volume < 0)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", startup_volume);
    if (startup_volume > 100)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", startup_volume);
    startup_volume = av_clip(startup_volume, 0, 100);
    startup_volume = av_clip(SDL_MIX_MAXVOLUME * startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
    is->audio_volume = startup_volume;
    is->muted = 0;
    is->av_sync_type = av_sync_type;
	is->ic = context;
    is->read_tid     = SDL_CreateThread(read_thread, "read_thread", is);
    if (!is->read_tid) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
        stream_close(is);
        return 0;
    }
    return is;
}

void StreamHandler::stream_cycle_channel(VideoState *is, int codec_type) {
    AVFormatContext *ic = is->ic;
    int start_index, stream_index;
    int old_index;
    AVStream *st;
    AVProgram *p = NULL;
    int nb_streams = is->ic->nb_streams;

    if (codec_type == AVMEDIA_TYPE_VIDEO) {
        start_index = is->last_video_stream;
        old_index = is->video_stream;
    } else if (codec_type == AVMEDIA_TYPE_AUDIO) {
        start_index = is->last_audio_stream;
        old_index = is->audio_stream;
    } else {
        start_index = is->last_subtitle_stream;
        old_index = is->subtitle_stream;
    }
    stream_index = start_index;

    if (codec_type != AVMEDIA_TYPE_VIDEO && is->video_stream != -1) {
        p = av_find_program_from_stream(ic, NULL, is->video_stream);
        if (p) {
            nb_streams = p->nb_stream_indexes;
            for (start_index = 0; start_index < nb_streams; start_index++)
                if (p->stream_index[start_index] == stream_index)
                    break;
            if (start_index == nb_streams)
                start_index = -1;
            stream_index = start_index;
        }
    }

    for (;;) {
        if (++stream_index >= nb_streams)
        {
            if (codec_type == AVMEDIA_TYPE_SUBTITLE)
            {
                stream_index = -1;
                is->last_subtitle_stream = -1;
                goto the_end;
            }
            if (start_index == -1)
                return;
            stream_index = 0;
        }
        if (stream_index == start_index)
            return;
        st = is->ic->streams[p ? p->stream_index[stream_index] : stream_index];
        if (st->codecpar->codec_type == codec_type) {
            /* check that parameters are OK */
            switch (codec_type) {
            case AVMEDIA_TYPE_AUDIO:
                if (st->codecpar->sample_rate != 0 &&
                    st->codecpar->ch_layout.nb_channels != 0)
                    goto the_end;
                break;
            case AVMEDIA_TYPE_VIDEO:
            case AVMEDIA_TYPE_SUBTITLE:
                goto the_end;
            default:
                break;
            }
        }
    }
 the_end:
    if (p && stream_index != -1)
        stream_index = p->stream_index[stream_index];
    av_log(NULL, AV_LOG_INFO, "Switch %s stream from #%d to #%d\n",
           av_get_media_type_string((AVMediaType) codec_type),
           old_index,
           stream_index);

    stream_component_close(is, old_index);
    StreamHandler::stream_component_open(is, stream_index);
}


void StreamHandler::quit() {
	run = false;
#ifdef PROFILING_SH
	if (debugfile.is_open()) {
		debugfile.flush();
		debugfile.close();
	}
#endif
}
