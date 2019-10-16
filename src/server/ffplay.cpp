/*
	av_io.cpp - Implementation file for the static AV_IO class.
	
	Revision 0
	
	Notes:
			- 
		
	2019/10/15 - Maya Posch
*/



#include "ffplay.h"


// Globals
DataBuffer media_buffer;

#include "types.h"

/* options specified by the user */
AVInputFormat *file_iformat;
const char *input_filename;
const char *window_title;
int default_width  = 640;
int default_height = 480;
int screen_width  = 0;
int screen_height = 0;
int screen_left = SDL_WINDOWPOS_CENTERED;
int screen_top = SDL_WINDOWPOS_CENTERED;
int audio_disable;
int video_disable;
int subtitle_disable;
const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
int seek_by_bytes = -1;
float seek_interval = 10;
int display_disable;
int borderless;
int alwaysontop;
int startup_volume = 100;
int show_status = 1;
int av_sync_type = AV_SYNC_AUDIO_MASTER;
int64_t start_time = AV_NOPTS_VALUE;
int64_t duration = AV_NOPTS_VALUE;
int fast = 0;
int genpts = 0;
int lowres = 0;
int decoder_reorder_pts = -1;
int autoexit;
int exit_on_keydown;
int exit_on_mousedown;
int loop = 1;
int framedrop = -1;
int infinite_buffer = -1;
enum ShowMode show_mode = SHOW_MODE_NONE;
const char *audio_codec_name;
const char *subtitle_codec_name;
const char *video_codec_name;
double rdftspeed = 0.02;
int64_t cursor_last_shown;
int cursor_hidden = 0;
#if CONFIG_AVFILTER
const char **vfilters_list = NULL;
int nb_vfilters = 0;
char *afilters = NULL;
#endif
int autorotate = 1;
int find_stream_info = 1;
int filter_nbthreads = 0;

/* current context */
int is_full_screen;
int64_t audio_callback_time;

unsigned sws_flags = SWS_BICUBIC;

AVPacket flush_pkt;


#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "types.h"

#include "player.h"
#include "stream_handler.h"
#include "sdl_renderer.h"

/* #ifdef _WIN32
#include <windows.h>
#endif */


static void do_exit(VideoState *is)
{
    if (is) {
        StreamHandler::stream_close(is);
    }
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    uninit_opts();
#if CONFIG_AVFILTER
    av_freep(&vfilters_list);
#endif
    avformat_network_deinit();
    if (show_status)
        printf("\n");
    SDL_Quit();
    av_log(NULL, AV_LOG_QUIET, "%s", "");
    exit(0);
}

static void sigterm_handler(int sig)
{
    exit(123);
}


static int opt_frame_size(void *optctx, const char *opt, const char *arg)
{
    av_log(NULL, AV_LOG_WARNING, "Option -s is deprecated, use -video_size.\n");
    return opt_default(NULL, "video_size", arg);
}

static int opt_width(void *optctx, const char *opt, const char *arg)
{
    screen_width = parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX);
    return 0;
}

static int opt_height(void *optctx, const char *opt, const char *arg)
{
    screen_height = parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX);
    return 0;
}

static int opt_format(void *optctx, const char *opt, const char *arg)
{
    file_iformat = av_find_input_format(arg);
    if (!file_iformat) {
        av_log(NULL, AV_LOG_FATAL, "Unknown input format: %s\n", arg);
        return AVERROR(EINVAL);
    }
    return 0;
}

static int opt_frame_pix_fmt(void *optctx, const char *opt, const char *arg)
{
    av_log(NULL, AV_LOG_WARNING, "Option -pix_fmt is deprecated, use -pixel_format.\n");
    return opt_default(NULL, "pixel_format", arg);
}

static int opt_sync(void *optctx, const char *opt, const char *arg)
{
    if (!strcmp(arg, "audio"))
        av_sync_type = AV_SYNC_AUDIO_MASTER;
    else if (!strcmp(arg, "video"))
        av_sync_type = AV_SYNC_VIDEO_MASTER;
    else if (!strcmp(arg, "ext"))
        av_sync_type = AV_SYNC_EXTERNAL_CLOCK;
    else {
        av_log(NULL, AV_LOG_ERROR, "Unknown value for %s: %s\n", opt, arg);
        exit(1);
    }
    return 0;
}

static int opt_seek(void *optctx, const char *opt, const char *arg)
{
    start_time = parse_time_or_die(opt, arg, 1);
    return 0;
}

static int opt_duration(void *optctx, const char *opt, const char *arg)
{
    duration = parse_time_or_die(opt, arg, 1);
    return 0;
}

static int opt_show_mode(void *optctx, const char *opt, const char *arg)
{
    show_mode = (ShowMode) !strcmp(arg, "video") ? SHOW_MODE_VIDEO :
                !strcmp(arg, "waves") ? SHOW_MODE_WAVES :
                !strcmp(arg, "rdft" ) ? SHOW_MODE_RDFT  :
                (ShowMode) parse_number_or_die(opt, arg, OPT_INT, 0, SHOW_MODE_NB-1);
    return 0;
}

static void opt_input_file(void *optctx, const char *filename)
{
    if (input_filename) {
        av_log(NULL, AV_LOG_FATAL,
               "Argument '%s' provided as input filename, but '%s' was already specified.\n",
                filename, input_filename);
        exit(1);
    }
    if (!strcmp(filename, "-"))
        filename = "pipe:";
    input_filename = filename;
}

static int opt_codec(void *optctx, const char *opt, const char *arg)
{
   const char *spec = strchr(opt, ':');
   if (!spec) {
       av_log(NULL, AV_LOG_ERROR,
              "No media specifier was specified in '%s' in option '%s'\n",
               arg, opt);
       return AVERROR(EINVAL);
   }
   spec++;
   switch (spec[0]) {
   case 'a' :    audio_codec_name = arg; break;
   case 's' : subtitle_codec_name = arg; break;
   case 'v' :    video_codec_name = arg; break;
   default:
       av_log(NULL, AV_LOG_ERROR,
              "Invalid media specifier '%s' in option '%s'\n", spec, opt);
       return AVERROR(EINVAL);
   }
   return 0;
}


static int dummy;
const char program_name[] = "ffplay";
const int program_birth_year = 2003;

static const OptionDef options[] = {
    CMDUTILS_COMMON_OPTIONS
    { "x", HAS_ARG, { .func_arg = opt_width }, "force displayed width", "width" },
    { "y", HAS_ARG, { .func_arg = opt_height }, "force displayed height", "height" },
    { "s", HAS_ARG | OPT_VIDEO, { .func_arg = opt_frame_size }, "set frame size (WxH or abbreviation)", "size" },
    { "fs", OPT_BOOL, { &is_full_screen }, "force full screen" },
    { "an", OPT_BOOL, { &audio_disable }, "disable audio" },
    { "vn", OPT_BOOL, { &video_disable }, "disable video" },
    { "sn", OPT_BOOL, { &subtitle_disable }, "disable subtitling" },
    { "ast", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_AUDIO] }, "select desired audio stream", "stream_specifier" },
    { "vst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_VIDEO] }, "select desired video stream", "stream_specifier" },
    { "sst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE] }, "select desired subtitle stream", "stream_specifier" },
    { "ss", HAS_ARG, { .func_arg = opt_seek }, "seek to a given position in seconds", "pos" },
    { "t", HAS_ARG, { .func_arg = opt_duration }, "play  \"duration\" seconds of audio/video", "duration" },
    { "bytes", OPT_INT | HAS_ARG, { &seek_by_bytes }, "seek by bytes 0=off 1=on -1=auto", "val" },
    { "seek_interval", OPT_FLOAT | HAS_ARG, { &seek_interval }, "set seek interval for left/right keys, in seconds", "seconds" },
    { "nodisp", OPT_BOOL, { &display_disable }, "disable graphical display" },
    { "noborder", OPT_BOOL, { &borderless }, "borderless window" },
    { "alwaysontop", OPT_BOOL, { &alwaysontop }, "window always on top" },
    { "volume", OPT_INT | HAS_ARG, { &startup_volume}, "set startup volume 0=min 100=max", "volume" },
    { "f", HAS_ARG, { .func_arg = opt_format }, "force format", "fmt" },
    { "pix_fmt", HAS_ARG | OPT_EXPERT | OPT_VIDEO, { .func_arg = opt_frame_pix_fmt }, "set pixel format", "format" },
    { "stats", OPT_BOOL | OPT_EXPERT, { &show_status }, "show status", "" },
    { "fast", OPT_BOOL | OPT_EXPERT, { &fast }, "non spec compliant optimizations", "" },
    { "genpts", OPT_BOOL | OPT_EXPERT, { &genpts }, "generate pts", "" },
    { "drp", OPT_INT | HAS_ARG | OPT_EXPERT, { &decoder_reorder_pts }, "let decoder reorder pts 0=off 1=on -1=auto", ""},
    { "lowres", OPT_INT | HAS_ARG | OPT_EXPERT, { &lowres }, "", "" },
    { "sync", HAS_ARG | OPT_EXPERT, { .func_arg = opt_sync }, "set audio-video sync. type (type=audio/video/ext)", "type" },
    { "autoexit", OPT_BOOL | OPT_EXPERT, { &autoexit }, "exit at the end", "" },
    { "exitonkeydown", OPT_BOOL | OPT_EXPERT, { &exit_on_keydown }, "exit on key down", "" },
    { "exitonmousedown", OPT_BOOL | OPT_EXPERT, { &exit_on_mousedown }, "exit on mouse down", "" },
    { "loop", OPT_INT | HAS_ARG | OPT_EXPERT, { &loop }, "set number of times the playback shall be looped", "loop count" },
    { "framedrop", OPT_BOOL | OPT_EXPERT, { &framedrop }, "drop frames when cpu is too slow", "" },
    { "infbuf", OPT_BOOL | OPT_EXPERT, { &infinite_buffer }, "don't limit the input buffer size (useful with realtime streams)", "" },
    { "window_title", OPT_STRING | HAS_ARG, { &window_title }, "set window title", "window title" },
    { "left", OPT_INT | HAS_ARG | OPT_EXPERT, { &screen_left }, "set the x position for the left of the window", "x pos" },
    { "top", OPT_INT | HAS_ARG | OPT_EXPERT, { &screen_top }, "set the y position for the top of the window", "y pos" },
#if CONFIG_AVFILTER
    { "vf", OPT_EXPERT | HAS_ARG, { .func_arg = StreamHandler::opt_add_vfilter }, "set video filters", "filter_graph" },
    { "af", OPT_STRING | HAS_ARG, { &afilters }, "set audio filters", "filter_graph" },
#endif
    { "rdftspeed", OPT_INT | HAS_ARG| OPT_AUDIO | OPT_EXPERT, { &rdftspeed }, "rdft speed", "msecs" },
    { "showmode", HAS_ARG, { .func_arg = opt_show_mode}, "select show mode (0 = video, 1 = waves, 2 = RDFT)", "mode" },
    { "default", HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT, { .func_arg = opt_default }, "generic catch all option", "" },
    { "i", OPT_BOOL, { &dummy}, "read specified file", "input_file"},
    { "codec", HAS_ARG, { .func_arg = opt_codec}, "force decoder", "decoder_name" },
    { "acodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {    &audio_codec_name }, "force audio decoder",    "decoder_name" },
    { "scodec", HAS_ARG | OPT_STRING | OPT_EXPERT, { &subtitle_codec_name }, "force subtitle decoder", "decoder_name" },
    { "vcodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {    &video_codec_name }, "force video decoder",    "decoder_name" },
    { "autorotate", OPT_BOOL, { &autorotate }, "automatically rotate video", "" },
    { "find_stream_info", OPT_BOOL | OPT_INPUT | OPT_EXPERT, { &find_stream_info },
        "read and decode the streams to fill missing information with heuristics" },
    { "filter_threads", HAS_ARG | OPT_INT | OPT_EXPERT, { &filter_nbthreads }, "number of filter threads per graph" },
    { NULL, },
};

static void show_usage(void)
{
    av_log(NULL, AV_LOG_INFO, "Simple media player\n");
    av_log(NULL, AV_LOG_INFO, "usage: %s [options] input_file\n", program_name);
    av_log(NULL, AV_LOG_INFO, "\n");
}

void show_help_default(const char *opt, const char *arg)
{
    av_log_set_callback(log_callback_help);
    show_usage();
    show_help_options(options, "Main options:", 0, OPT_EXPERT, 0);
    show_help_options(options, "Advanced options:", OPT_EXPERT, 0, 0);
    printf("\n");
    show_help_children(avcodec_get_class(), AV_OPT_FLAG_DECODING_PARAM);
    show_help_children(avformat_get_class(), AV_OPT_FLAG_DECODING_PARAM);
#if !CONFIG_AVFILTER
    show_help_children(sws_get_class(), AV_OPT_FLAG_ENCODING_PARAM);
#else
    show_help_children(avfilter_get_class(), AV_OPT_FLAG_FILTERING_PARAM);
#endif
    printf("\nWhile playing:\n"
           "q, ESC              quit\n"
           "f                   toggle full screen\n"
           "p, SPC              pause\n"
           "m                   toggle mute\n"
           "9, 0                decrease and increase volume respectively\n"
           "/, *                decrease and increase volume respectively\n"
           "a                   cycle audio channel in the current program\n"
           "v                   cycle video channel\n"
           "t                   cycle subtitle channel in the current program\n"
           "c                   cycle program\n"
           "w                   cycle video filters or show modes\n"
           "s                   activate frame-step mode\n"
           "left/right          seek backward/forward 10 seconds or to custom interval if -seek_interval is set\n"
           "down/up             seek backward/forward 1 minute\n"
           "page down/page up   seek backward/forward 10 minutes\n"
           "right mouse click   seek to percentage in file corresponding to fraction of width\n"
           "left double-click   toggle full screen\n"
           );
}


int Ffplay::media_read(void* opaque, uint8_t* buf, int buf_size) {
	if (buf_size == 0) { return AVERROR_EOF; }
	
    DataBuffer* db = static_cast<DataBuffer*>(opaque); 
	
    // Fill the buffer from the memory buffer.
	// Return the read length.
	uint32_t bytesToCopy = 0;
	
	// Check if we're headed for a buffer underrun.
	if (db->buffBytesLeft < buf_size && !db->eof) {
		db->requestCondition.signal(); // Ask for more data.
		db->bufferDelayMutex.lock();
		db->bufferDelayCondition.tryWait(db->bufferDelayMutex, 150);
	}

	if (db->buffBytesLeft >= buf_size) {  	// At least as many bytes remaining as requested
		bytesToCopy = buf_size;
	} 
	else if (db->buffBytesLeft < buf_size) {	// Fewer than requested number of bytes remaining
		bytesToCopy = db->buffBytesLeft;
	} 
	else {
		return AVERROR_EOF;   // No bytes left to copy
	}

	// Each slot has a limited depth. Check that we can copy the whole requested buffer
	// from a single slot, only updating the index into that slot.
	// Else, copy what is left in the current slot into the buffer, then copy the rest from the
	// next slot.
	if (db->slotBytesLeft < bytesToCopy) {
					
		uint32_t nextBytes = bytesToCopy;
		
		// Copy the rest of the bytes in the slot, move onto the next slot.
		uint32_t byteCount = 0;
		uint32_t bytesWritten = 0;
		bool nextSlot = false;
		while (nextBytes > 0) {
			if (db->slotBytesLeft < nextBytes) {
				// Cannot copy the remainder of the requested bytes in one go yet.
				// Just copy what we can.
				byteCount = db->slotBytesLeft;
				nextBytes -= db->slotBytesLeft;
				nextSlot = true;
			}
			else {
				// Copy the remaining bytes into the buffer.
				byteCount = nextBytes;
				nextBytes = 0;
				if (db->slotBytesLeft == byteCount) {
					nextSlot = true;
				}
			}
			
			// Debug
			std::cout << "Reading from slot " << db->currentSlot << std::endl;
			std::cout << "Index: " << db->currentIndex << "\t/\t" << db->slotSize 
						<< ", \tCopy: " << bytesToCopy << "\t/\t" << db->slotBytesLeft << std::endl;
			
			
			db->mutex.lock();
			std::copy(db->data[db->currentSlot].begin() + db->currentIndex, 
					(db->data[db->currentSlot].begin() + db->currentIndex) + byteCount, 
					(buf + bytesWritten));
			db->mutex.unlock();
							
			db->slotBytesLeft -= byteCount;
			bytesWritten += byteCount;
			if (nextSlot) {
				nextSlot = false;
				db->currentSlot++;
				if (!(db->currentSlot < db->numSlots)) { db->currentSlot = 0; }
				db->slotSize = db->data[db->currentSlot].length();
				db->currentIndex = 0;
				db->slotBytesLeft = db->slotSize.load();
				db->freeSlots++; // The used buffer slot just became available for more data.
			}
			else {
				db->currentIndex += byteCount;
			}
		}
	}
	else {
		// Debug
		std::cout << "Reading from slot " << db->currentSlot << std::endl;
		std::cout << "Index: " << db->currentIndex << "\t/\t" << db->slotSize 
					<< ", \tCopy: " << bytesToCopy << "\t/\t" << db->slotBytesLeft << std::endl;
		
		// Just copy the bytes from the slot, adjusting the index and bytes left count.
		db->mutex.lock();
		std::copy(db->data[db->currentSlot].begin() + db->currentIndex, 
				(db->data[db->currentSlot].begin() + db->currentIndex) + bytesToCopy, 
				buf);
		db->mutex.unlock();
		db->currentIndex += bytesToCopy;    // Increment bytes read count
		db->slotBytesLeft -= bytesToCopy;
	}
	
	db->buffBytesLeft -= bytesToCopy;
	
	// If there are free slots in the buffer, request more data from the client.
	if (!db->requestInFlight && !(db->eof) && db->freeSlots > 0) {
		db->requestCondition.signal();
	}
	
	// Debug
	//OUT_FILE.write((const char*) buf, bytesToCopy);
	
	if (bytesToCopy == 0) { return AVERROR_EOF; }
	
	return bytesToCopy;
}


int64_t Ffplay::media_seek(void* opaque, int64_t pos, int whence) {
    DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
	// Try to find the index in the buffered data. If unavailable, read from file.
	//db->index = offset;
	
	// TODO: implement.
 
    // Return the new position:
    return 0;
}



// --- RUN ---
void Ffplay::run() {
	// 
	int flags;
    VideoState *is;

    init_dynload();
	
	// Fake command line arguments.
	std::vector<std::string> arguments = {"nymphcast", "-loglevel", "trace"};
	std::vector<char*> argv;
	for (int i = 0; i < arguments.size(); i++) {
		const std::string& arg = arguments[i];
		argv.push_back((char*) arg.data());
	}
	
	int argc = argv.size() - 1;
	/* char* argv[argc] = { "nymphcast",
						"-loglevel",
						"trace" }; */

    av_log_set_flags(AV_LOG_SKIP_REPEATED);
    parse_loglevel(argc, argv.data(), options);

    /* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    //avformat_network_init();

    init_opts();
	
	signal(SIGINT , sigterm_handler); /* Interrupt (ANSI).    */
    signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

    show_banner(argc, argv.data(), options);
    parse_options(NULL, argc, argv.data(), options, opt_input_file);
		
	 
	// Create internal Buffer for FFmpeg:
	const int iBufSize = 16 * 1024 * 1024; // 16 MB
	uint8_t* pBuffer = new uint8_t[iBufSize];
	 
	// Allocate the AVIOContext:
	// The fourth parameter (pStream) is a user parameter which will be passed to our callback functions
	AVIOContext* pIOCtx = avio_alloc_context(pBuffer, iBufSize,  // internal Buffer and its size
											 0,                  // bWriteable (1=true,0=false) 
											 &media_buffer,          // user data ; will be passed to our callback functions
											 media_read, 
											 0,                  // Write callback function (not used in this example) 
											 media_seek); 
	 
	// Allocate the AVFormatContext.
	AVFormatContext* pCtx = avformat_alloc_context();
	 
	// Set the IOContext.
	pCtx->pb = pIOCtx;
	
	
	// Determine the input format.
	uint64_t ulReadBytes = 0;

	// Create the ProbeData structure for av_probe_input_format.
	AVProbeData probeData;
	media_buffer.dataMutex.lock();
	probeData.buf = (unsigned char*) media_buffer.data[0].data();
	probeData.buf_size = media_buffer.data[0].size();
	probeData.filename = "";
	 
	// Determine the input-format:
	pCtx->iformat = av_probe_input_format(&probeData, 1);
	media_buffer.dataMutex.unlock();
	
	pCtx->flags = AVFMT_FLAG_CUSTOM_IO;
	
	// Start player.
	
	// Debug
	//CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//CoInitialize(NULL);
/* #ifdef _WIN32
	CoInitialize(NULL);
#endif */
	
	if (display_disable) {
        video_disable = 1;
    }
	
    flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    if (audio_disable)
        flags &= ~SDL_INIT_AUDIO;
    else {
        /* Try to work around an occasional ALSA buffer underflow issue when the
         * period size is NPOT due to ALSA resampling by forcing the buffer size. */
        if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
            SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE","1", 1);
    }
	
    if (display_disable)
        flags &= ~SDL_INIT_VIDEO;
	
    if (SDL_Init (flags)) {
        av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
        av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
        exit(1);
    }

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *) &flush_pkt;

	// Init SDL.
	if (!display_disable) {
        int flags = SDL_WINDOW_HIDDEN;
        if (alwaysontop)
#if SDL_VERSION_ATLEAST(2,0,5)
            flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#else
            av_log(NULL, AV_LOG_WARNING, "Your SDL version doesn't support SDL_WINDOW_ALWAYS_ON_TOP. Feature will be inactive.\n");
#endif
        if (borderless)
            flags |= SDL_WINDOW_BORDERLESS;
        else
            flags |= SDL_WINDOW_RESIZABLE;
        window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, default_width, default_height, flags);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		renderer = 0;
        if (window) {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!renderer) {
                av_log(NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
                renderer = SDL_CreateRenderer(window, -1, 0);
            }
            if (renderer) {
                if (!SDL_GetRendererInfo(renderer, &renderer_info))
                    av_log(NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", renderer_info.name);
            }
        }
		
        if (!window || !renderer || !renderer_info.num_texture_formats) {
            av_log(NULL, AV_LOG_FATAL, "Failed to create window or renderer: %s", SDL_GetError());
            do_exit(NULL);
        }
    }

	input_filename = "";
    is = StreamHandler::stream_open(input_filename, file_iformat, pCtx);
    if (!is) {
        av_log(NULL, AV_LOG_FATAL, "Failed to initialize VideoState!\n");
        do_exit(NULL);
    }

    Player::event_loop(is);	
	
	// Free resources
	avformat_close_input(&pCtx);  // AVFormatContext is released by avformat_close_input
	av_free(pIOCtx);             // AVIOContext is released by av_free
	delete[] pBuffer; 
	
}
 
 
// --- QUIT ---
void Ffplay::quit() {
	// Stop player.
	Player::quit();
}

