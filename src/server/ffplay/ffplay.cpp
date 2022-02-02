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
FileMetaInfo file_meta;

unsigned sws_flags = SWS_BICUBIC;

AVPacket flush_pkt;

#include <Poco/NumberFormatter.h>
#include <nymph/nymph_logger.h>

#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <string>
#include <cstring>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "types.h"
#include "../databuffer.h"

#include "player.h"
#include "stream_handler.h"
#include "sdl_renderer.h"


// Global objects.
Poco::Condition playerCon;
Poco::Mutex playerMutex;
std::condition_variable playbackCv;
// ---

// Static definitions.
std::string Ffplay::loggerName = "Ffplay";


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
				(ShowMode) (int) parse_number_or_die(opt, arg, OPT_INT, 0, SHOW_MODE_NB-1);
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
	//std::cout << "Read " << bytesRead << " bytes." << std::endl;
	NYMPH_LOG_INFORMATION("Read " + Poco::NumberFormatter::format(bytesRead) + " bytes.");
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
	NYMPH_LOG_INFORMATION("media_seek: offset " + Poco::NumberFormatter::format(offset) + 
							", whence " + Poco::NumberFormatter::format(whence));
	
	int64_t new_offset = AVERROR(EIO);
	switch (whence) {
		case SEEK_SET:	// Seek from the beginning of the file.
			NYMPH_LOG_INFORMATION("media_seek: SEEK_SET");
			new_offset = DataBuffer::seek(DB_SEEK_START, offset);
			break;
		case SEEK_CUR:	// Seek from the current position.
			NYMPH_LOG_INFORMATION("media_seek: SEEK_CUR");
			new_offset = DataBuffer::seek(DB_SEEK_CURRENT, offset);
			break;
		case SEEK_END:	// Seek from the end of the file.
			NYMPH_LOG_INFORMATION("media_seek: SEEK_END");
			new_offset = DataBuffer::seek(DB_SEEK_END, offset);
			break;
		case AVSEEK_SIZE:
			NYMPH_LOG_INFORMATION("media_seek: received AVSEEK_SIZE, returning file size.");
			return DataBuffer::getFileSize();
			break;
		default:
			NYMPH_LOG_ERROR("media_seek: default. The universe broke.");
			/* new_offset = -1;
			return new_offset; */
	}
	
	if (new_offset < 0) {
		// Some error occurred.
		NYMPH_LOG_ERROR("Error during seeking.");
		new_offset = AVERROR(EIO);
	}
	
	NYMPH_LOG_INFORMATION("New offset: " + Poco::NumberFormatter::format(new_offset));
	
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


// --- STREAM TRACK ---
bool Ffplay::streamTrack(std::string url) {
	if (!playerStarted) {
		castUrl = url;
		castingUrl = true;
		
		playbackCv.notify_one();
		
		// TODO: wait N ms for playback to start. Try again if not started, else fail.
	}
	else {
		av_log(NULL, AV_LOG_ERROR, "Playback already active. Aborting playback of %s.\n", url.c_str());
		return false;
	}
	
	return true;
}


// --- PLAY TRACK ---
bool Ffplay::playTrack() {
	if (!playerStarted) {
		playingTrack = true;
		
		playbackCv.notify_one();
	}
	else {
		av_log(NULL, AV_LOG_ERROR, "Playback already active. Aborting track playback.\n");
		return false;
	}
	
	return true;
}


// --- RUN ---
void Ffplay::run() {
	init_dynload();

	avdevice_register_all();
	avformat_network_init();

	init_opts();

	//parse_options(NULL, argc, argv.data(), options, opt_input_file);
	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	//av_log_set_level(AV_LOG_TRACE);
	av_log_set_level(AV_LOG_INFO);
	
	// Start main loop.
	// This loop waits until it is triggered, at which point there should either be a URL to
	// stream from, or a file to stream via the DataBuffer.
	running = true;
	playerStarted = false;
	std::mutex playbackMtx;
	while (running) {
		// Wait in condition variable until triggered. Ensure an event is waiting to deal with
		// spurious wake-ups.
		std::unique_lock<std::mutex> lk(playbackMtx);
		using namespace std::chrono_literals;
		playbackCv.wait(lk);
		
		if (!running) {
			av_log(NULL, AV_LOG_INFO, "Terminating AV thread...\n");
			break;
		}
		
		// Intercept spurious wake-ups.
		if (!playingTrack && !castingUrl) { continue; }
		
		// Start playback.
		playerStarted = true;
		
		// --- AVIOContext section ---
		AVFormatContext* formatContext = 0;
		AVIOContext* ioContext = 0;
		if (!castingUrl) {
			input_filename = "";
		
			// Create internal buffer for FFmpeg.
			size_t iBufSize = 32 * 1024; // 32 kB
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
		/* if (!castingUrl) {
			file_meta.duration = is->ic->duration / AV_TIME_BASE; // Convert to seconds.
		} */

		
		Player::setVideoState(is);
		SdlRenderer::playerEvents(true);
		
		// Wait here until playback has finished.
		// The read thread in StreamHandler will signal this condition variable.
		playerMutex.lock();
		playerCon.wait(playerMutex);
		playerMutex.unlock();
		
		// Ensure we disable player events since we're no longer processing them.
		// The StreamHandler::read_thread should have disabled them already.
		SdlRenderer::playerEvents(false);
		
		// Clear file meta info.
		//FileMetaInfo::setPosition(0.0);
		file_meta.setPosition(0.0);
		//FileMetaInfo::setDuration(0);
		file_meta.setDuration(0);
		
		if (ioContext) {
			av_freep(&ioContext->buffer);
			av_freep(&ioContext);
		}
		
		if (is) {
			StreamHandler::stream_close(is);
			is = 0;
		}
		
		SDL_Delay(500); // wait 500 ms.
		
		av_log(NULL, AV_LOG_INFO, "Terminating player...\n");
		
		DataBuffer::reset();	// Clears the data buffer (file data buffer).
		finishPlayback();		// Calls handler for post-playback steps.
		playerStarted = false;
		playingTrack = false;
		castingUrl = false;
		
		// Update clients with status update.
		sendGlobalStatusUpdate();
	}
}
 
 
// --- QUIT ---
void Ffplay::quit() {
	// Stop player.
	Player::quit();
	
	// End loop.
	running = false;
	playbackCv.notify_one();
}

