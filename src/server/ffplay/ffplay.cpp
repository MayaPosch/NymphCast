/*
	ffplay.cpp - Implementation file for the static AV_IO class.
	
	Revision 0
	
	Notes:
			- 
		
	2019/10/15 - Maya Posch
*/



#include "ffplay.h"


//#define DEBUG 1


/* current context */
int is_full_screen;
int64_t audio_callback_time;

unsigned sws_flags = SWS_BICUBIC;

AVPacket flush_pkt;


// Global objects.
Poco::Condition playerCon;
Poco::Mutex playerMutex;
// ---


#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <string>
#include <cstring>
#include <vector>

#include "types.h"
#include "../databuffer.h"

#include "player.h"
#include "stream_handler.h"
#include "sdl_renderer.h"


static void do_exit(VideoState *is) {
	if (is) {
		StreamHandler::stream_close(is);
	}
	
	uninit_opts();
#if CONFIG_AVFILTER
	av_freep(&vfilters_list);
#endif
	avformat_network_deinit();
	if (show_status)
		printf("\n");
	SdlRenderer::quit();
	av_log(NULL, AV_LOG_QUIET, "%s", "");
	exit(0);
}

/* static void sigterm_handler(int sig) {
	exit(123);
} */


static int opt_frame_size(void *optctx, const char *opt, const char *arg) {
	av_log(NULL, AV_LOG_WARNING, "Option -s is deprecated, use -video_size.\n");
	return opt_default(NULL, "video_size", arg);
}

static int opt_width(void *optctx, const char *opt, const char *arg) {
	screen_width = parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX);
	return 0;
}

static int opt_height(void *optctx, const char *opt, const char *arg) {
	screen_height = parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX);
	return 0;
}

static int opt_format(void *optctx, const char *opt, const char *arg) {
	file_iformat = av_find_input_format(arg);
	if (!file_iformat) {
		av_log(NULL, AV_LOG_FATAL, "Unknown input format: %s\n", arg);
		return AVERROR(EINVAL);
	}
	return 0;
}

static int opt_frame_pix_fmt(void *optctx, const char *opt, const char *arg) {
	av_log(NULL, AV_LOG_WARNING, "Option -pix_fmt is deprecated, use -pixel_format.\n");
	return opt_default(NULL, "pixel_format", arg);
}

static int opt_sync(void *optctx, const char *opt, const char *arg) {
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

static int opt_seek(void *optctx, const char *opt, const char *arg) {
	start_time = parse_time_or_die(opt, arg, 1);
	return 0;
}

static int opt_duration(void *optctx, const char *opt, const char *arg) {
	duration = parse_time_or_die(opt, arg, 1);
	return 0;
}

static int opt_show_mode(void *optctx, const char *opt, const char *arg) {
	show_mode = (ShowMode) !strcmp(arg, "video") ? SHOW_MODE_VIDEO :
				!strcmp(arg, "waves") ? SHOW_MODE_WAVES :
				!strcmp(arg, "rdft" ) ? SHOW_MODE_RDFT  :
				(ShowMode) parse_number_or_die(opt, arg, OPT_INT, 0, SHOW_MODE_NB-1);
	return 0;
}

static void opt_input_file(void *optctx, const char *filename) {
	if (input_filename) {
		av_log(NULL, AV_LOG_FATAL,
			   "Argument '%s' provided as input filename, but '%s' was already specified.\n",
				filename, input_filename);
		exit(1);
	}
	
	if (!strcmp(filename, "-")) { filename = "pipe:"; }
	input_filename = filename;
}

static int opt_codec(void *optctx, const char *opt, const char *arg) {
   const char *spec = strchr(opt, ':');
   if (!spec) {
	   av_log(NULL, AV_LOG_ERROR,
			  "No media specifier was specified in '%s' in option '%s'\n",
			   arg, opt);
	   return AVERROR(EINVAL);
   }
   spec++;
   switch (spec[0]) {
   case 'a' :	audio_codec_name = arg; break;
   case 's' : subtitle_codec_name = arg; break;
   case 'v' :	video_codec_name = arg; break;
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
	//{ "an", OPT_BOOL, { &audio_disable }, "disable audio" },
	//{ "vn", OPT_BOOL, { &video_disable }, "disable video" },
	//{ "sn", OPT_BOOL, { &subtitle_disable }, "disable subtitling" },
	{ "ast", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_AUDIO] }, "select desired audio stream", "stream_specifier" },
	{ "vst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_VIDEO] }, "select desired video stream", "stream_specifier" },
	{ "sst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE] }, "select desired subtitle stream", "stream_specifier" },
	{ "ss", HAS_ARG, { .func_arg = opt_seek }, "seek to a given position in seconds", "pos" },
	{ "t", HAS_ARG, { .func_arg = opt_duration }, "play  \"duration\" seconds of audio/video", "duration" },
	{ "bytes", OPT_INT | HAS_ARG, { &seek_by_bytes }, "seek by bytes 0=off 1=on -1=auto", "val" },
	{ "seek_interval", OPT_FLOAT | HAS_ARG, { &seek_interval }, "set seek interval for left/right keys, in seconds", "seconds" },
	//{ "nodisp", OPT_BOOL, { &display_disable }, "disable graphical display" },
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
	{ "acodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {	&audio_codec_name }, "force audio decoder",	"decoder_name" },
	{ "scodec", HAS_ARG | OPT_STRING | OPT_EXPERT, { &subtitle_codec_name }, "force subtitle decoder", "decoder_name" },
	{ "vcodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {	&video_codec_name }, "force video decoder",	"decoder_name" },
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
		   "q, ESC			  quit\n"
		   "f				   toggle full screen\n"
		   "p, SPC			  pause\n"
		   "m				   toggle mute\n"
		   "9, 0				decrease and increase volume respectively\n"
		   "/, *				decrease and increase volume respectively\n"
		   "a				   cycle audio channel in the current program\n"
		   "v				   cycle video channel\n"
		   "t				   cycle subtitle channel in the current program\n"
		   "c				   cycle program\n"
		   "w				   cycle video filters or show modes\n"
		   "s				   activate frame-step mode\n"
		   "left/right		  seek backward/forward 10 seconds or to custom interval if -seek_interval is set\n"
		   "down/up			 seek backward/forward 1 minute\n"
		   "page down/page up   seek backward/forward 10 minutes\n"
		   "right mouse click   seek to percentage in file corresponding to fraction of width\n"
		   "left double-click   toggle full screen\n"
		   );
}


/**
 * Reads from a buffer into FFmpeg.
 *
 * @param ptr	   A pointer to the user-defined IO data structure.
 * @param buf	   A buffer to read into.
 * @param buf_size  The size of the buffer buff.
 *
 * @return The number of bytes read into the buffer.
 */
int Ffplay::media_read(void* opaque, uint8_t* buf, int buf_size) {
	uint32_t bytesRead = DataBuffer::read(buf_size, buf);
	std::cout << "Read " << bytesRead << " bytes." << std::endl;
	if (bytesRead == 0) {
		std::cout << "EOF is " << DataBuffer::isEof() << std::endl;
		if (DataBuffer::isEof()) { return AVERROR_EOF; }
		else { return AVERROR(EIO); }
	}
	
	return bytesRead;
}


/**
 * Seeks to a given position in the currently open file.
 * 
 * @param opaque  A pointer to the user-defined IO data structure.
 * @param offset  The position to seek to.
 * @param whence  SEEK_SET, SEEK_CUR, SEEK_END (like fseek) and AVSEEK_SIZE.
 *
 * @return  The new byte position in the file or -1 in case of failure.
 */
int64_t Ffplay::media_seek(void* opaque, int64_t offset, int whence) {
	std::cout << "media_seek: offset " << offset << ", whence " << whence << std::endl;
	
	int64_t new_offset = AVERROR(EIO);
	switch (whence) {
		case SEEK_SET:	// Seek from the beginning of the file.
			std::cout << "media_seek: SEEK_SET" << std::endl;
			new_offset = DataBuffer::seek(DB_SEEK_START, offset);
			break;
		case SEEK_CUR:	// Seek from the current position.
			std::cout << "media_seek: SEEK_CUR" << std::endl;
			new_offset = DataBuffer::seek(DB_SEEK_CURRENT, offset);
			break;
		case SEEK_END:	// Seek from the end of the file.
			std::cout << "media_seek: SEEK_END" << std::endl;
			new_offset = DataBuffer::seek(DB_SEEK_END, offset);
			break;
		case AVSEEK_SIZE:
			std::cout << "media_seek: received AVSEEK_SIZE, returning file size." << std::endl;
			return DataBuffer::getFileSize();
			break;
		default:
			std::cout << "media_seek: default. The universe broke." << std::endl;
			/* new_offset = -1;
			return new_offset; */
	}
	
	if (new_offset < 0) {
		// Some error occurred.
		std::cerr << "Error during seeking." << std::endl;
		new_offset = AVERROR(EIO);
	}
	
	std::cout << "New offset: " << new_offset << std::endl;
	
	return new_offset;
}


// --- GET VOLUME ---
uint8_t Ffplay::getVolume() {
	//if (!is) { return 0; }
	
	return audio_volume;
}


// --- SET VOLUME ---
void Ffplay::setVolume(uint8_t volume) {
	audio_volume = volume;
	if (!is) { return; }
	
	if (volume > SDL_MIX_MAXVOLUME) {
		is->audio_volume = SDL_MIX_MAXVOLUME;
	}
	else {
		is->audio_volume = volume;
	}
}


#ifdef _WIN32
void avLogCallback(void *ptr, int level, const char *fmt, va_list vargs) {
    //if (level > av_log_get_level()) { return; }

	//std::cout << level << " - " << fmt << std::endl;

    //printf("%s ", "va_log:");
    vprintf(fmt, vargs);
} 
#endif


// --- RUN ---
void Ffplay::run() {
	init_dynload();
	
	// Fake command line arguments.
	std::vector<std::string> arguments = {"nymphcast", "-autoexit", "-loglevel", "info"};
	std::vector<char*> argv;
	for (int i = 0; i < arguments.size(); i++) {
		const std::string& arg = arguments[i];
		argv.push_back((char*) arg.data());
	}
	
	int argc = argv.size() - 1;

	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	parse_loglevel(argc, argv.data(), options);
	
#ifdef _WIN32
	av_log_set_callback(avLogCallback);
#endif

	/* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
	avdevice_register_all();
#endif
	avformat_network_init();

	init_opts();
	
	//signal(SIGINT , sigterm_handler); /* Interrupt (ANSI).	*/
	//signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

	show_banner(argc, argv.data(), options);
	parse_options(NULL, argc, argv.data(), options, opt_input_file);
		
		
	// --- AVIOContext section ---
	AVFormatContext* formatContext = 0;
	AVIOContext* ioContext = 0;
	if (!castingUrl) {
		input_filename = "";
	
		// Create internal buffer for FFmpeg.
		size_t iBufSize = 1024 * 1024; // 1 MB
		uint8_t* pBuffer = (uint8_t*) av_malloc(iBufSize);
		 
		// Allocate the AVIOContext:
		// The fourth parameter (pStream) is a user parameter which will be passed to our callback functions
		ioContext = avio_alloc_context(pBuffer, iBufSize,  // internal Buffer and its size
												 0,				  // bWriteable (1=true,0=false) 
												 0, //&media_buffer,	  // user data
												 media_read, 
												 0,				  // Write callback function. 
												 media_seek);
		 
		// Allocate the AVFormatContext. This holds information about the container format.
		formatContext = avformat_alloc_context();
		formatContext->pb = ioContext;	// Set the IOContext.
		//formatContext->flags = AVFMT_FLAG_CUSTOM_IO;
		
		// Determine the input format.
		// Create the ProbeData structure for av_probe_input_format.
		/* AVProbeData probeData;
		media_buffer.dataMutex.lock();
		probeData.buf = (unsigned char*) media_buffer.data[0].data();
		//probeData.buf_size = media_buffer.data[0].size();
		probeData.buf_size = 4096;
		probeData.filename = "";
		
		pCtx->iformat = av_probe_input_format(&probeData, 1);
		media_buffer.dataMutex.unlock(); */
		
		
	// --- End AVIOContext section ---
	}
	else {
		input_filename = castUrl.c_str();
	}
	
	// Start player.
	is = StreamHandler::stream_open(input_filename, file_iformat, formatContext);
	if (!is) {
		av_log(NULL, AV_LOG_FATAL, "Failed to initialize VideoState!\n");
		do_exit(NULL);
	}
	
	// Extract meta data from VideoState instance and copy to FileMetaInfo instance.
	//av_dict_get(ic->metadata, "title", NULL, 0);
	if (!castingUrl) {
		file_meta.duration = is->ic->duration / AV_TIME_BASE; // Convert to seconds.
	}

	Player::setVideoState(is);
	SdlRenderer::playerEvents(true);
	
	// Wait here until playback has finished.
	// The read thread in StreamHandler will signal this condition variable.
	playerMutex.lock();
	playerCon.wait(playerMutex);
	
	SDL_Delay(500); // wait 500 ms.
	
	if (ioContext) {
		av_freep(&ioContext->buffer);
		av_freep(&ioContext);
	}
	
	if (is) {
		StreamHandler::stream_close(is);
		is = 0;
	}
	
	av_log(NULL, AV_LOG_INFO, "Terminating player...\n");
	
	SdlRenderer::playerEvents(false);
	DataBuffer::reset();	// Clears the data buffer (file data buffer).
	finishPlayback();		// Calls handler for post-playback steps.
}
 
 
// --- QUIT ---
void Ffplay::quit() {
	// Stop player.
	Player::quit();
}

