/*
	test_ffplay_local_file.cpp - FFPlay test using local file data.
	
*/


#include <Poco/Condition.h>
#include <Poco/Thread.h>
#include <Poco/Path.h>
#include <Poco/File.h>

#include <iostream>
#include <string>
#include <fstream>

#include "types.h"
#include "player.h"
#include "stream_handler.h"
#include "sdl_renderer.h"


struct FileMetaInfo {
	//std::atomic<uint32_t> filesize;		// bytes.
	std::atomic<uint64_t> duration;		// seconds
	std::atomic<double> position;		// seconds with remainder.
	std::atomic<uint32_t> width;		// pixels
	std::atomic<uint32_t> height;		// pixels
	std::atomic<uint32_t> video_rate;	// kilobits per second
	std::atomic<uint32_t> audio_rate;	// kilobits per second
	std::atomic<uint32_t> framrate;
	std::atomic<uint8_t> audio_channels;
	std::string title;
	std::string artist;
	std::string album;
	Poco::Mutex mutex;	// Use only with non-atomic entries.
	
	void setTitle(std::string t) { mutex.lock(); title = t; mutex.unlock(); }
	std::string getTitle() { mutex.lock(); std::string t = title; mutex.unlock(); return t; }
	
	void setArtist(std::string a) { mutex.lock(); artist = a; mutex.unlock(); }
	std::string getArtist() { mutex.lock(); std::string a = artist; mutex.unlock(); return a; }
	
	void setAlbum(std::string a) { mutex.lock(); album = a; mutex.unlock(); }
	std::string getAlbum() { mutex.lock(); std::string a = album; mutex.unlock(); return a; }
};


// Global objects.
FileMetaInfo file_meta;
Poco::Condition gCon;
Poco::Condition playerCon;
Poco::Mutex playerMutex;
std::ifstream source;
bool eof = false;
uint64_t filePosition = 0;
uint32_t filesize;

const uint32_t nymph_seek_event = SDL_RegisterEvents(1);



/* current context */
int is_full_screen;
int64_t audio_callback_time;

unsigned sws_flags = SWS_BICUBIC;

AVPacket flush_pkt;

/* options specified by the user */
AVInputFormat *file_iformat;
const char *input_filename;
const char *window_title;
int default_width  = 640;
int default_height = 480;
std::atomic<int> screen_width  = { 0 };
std::atomic<int> screen_height = { 0 };
int screen_left = SDL_WINDOWPOS_CENTERED;
int screen_top = SDL_WINDOWPOS_CENTERED;
int audio_disable;
int video_disable;
int subtitle_disable;
const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
int seek_by_bytes = -1;
float seek_interval = 10;
int display_disable;
bool gui_enable;
bool screensaver_enable;
int borderless;
int alwaysontop;
int startup_volume = 100;
int show_status = 1;
int av_sync_type = AV_SYNC_AUDIO_MASTER;
//int av_sync_type = AV_SYNC_EXTERNAL_CLOCK;
int64_t start_time = AV_NOPTS_VALUE;
int64_t duration = AV_NOPTS_VALUE;
int fast = 0;
int genpts = 0;
int lowres = 0;
int decoder_reorder_pts = -1;
int autoexit = 1;
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
std::atomic<uint32_t> audio_volume = { 100 };
// ---

const char program_name[] = "ffplay";
const int program_birth_year = 2003;
void show_help_default(const char *opt, const char *arg) { }


/**
 * Reads from a buffer into FFmpeg.
 *
 * @param ptr	   A pointer to the user-defined IO data structure.
 * @param buf	   A buffer to read into.
 * @param buf_size  The size of the buffer buff.
 *
 * @return The number of bytes read into the buffer.
 */
int media_read(void* opaque, uint8_t* buf, int buf_size) {
	// Read the requested bytes.
	source.read((char*) buf, buf_size);
	
	// Check characters read.
	uint32_t count = source.gcount();
	std::cout << "Read " << count << " bytes." << std::endl;
	if (count == 0) {
		std::cout << "EOF is " << eof << std::endl;
		if (eof) { return AVERROR_EOF; }
		else { return AVERROR(EIO); }
	}
	
	filePosition += count;
	
	return count;
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
int64_t media_seek(void* opaque, int64_t offset, int whence) {
	std::cout << "media_seek: offset " << offset << ", whence " << whence << std::endl;
	
	int64_t new_offset = AVERROR(EIO);
	switch (whence) {
		case SEEK_SET:	// Seek from the beginning of the file.
			std::cout << "media_seek: SEEK_SET" << std::endl;
			//new_offset = DataBuffer::seek(DB_SEEK_START, offset);
			new_offset = offset;
			break;
		case SEEK_CUR:	// Seek from the current position.
			std::cout << "media_seek: SEEK_CUR" << std::endl;
			//new_offset = DataBuffer::seek(DB_SEEK_CURRENT, offset);
			new_offset = filePosition + offset;
			break;
		case SEEK_END:	// Seek from the end of the file.
			std::cout << "media_seek: SEEK_END" << std::endl;
			//new_offset = DataBuffer::seek(DB_SEEK_END, offset);
			new_offset = filesize - offset - 1;
			break;
		case AVSEEK_SIZE:
			std::cout << "media_seek: received AVSEEK_SIZE, returning file size." << std::endl;
			//return DataBuffer::getFileSize();
			return filesize;
			break;
		default:
			std::cerr << "media_seek: default. The universe broke." << std::endl;
	}
	
	if (new_offset < 0) {
		// Some error occurred.
		std::cerr << "Error during seeking." << std::endl;
		new_offset = AVERROR(EIO);
	}
	
	std::cout << "New offset: " << new_offset << std::endl;
	
	// Seek to the indicated position in the file.
	uint64_t position = (uint64_t) new_offset;
	std::cout << "Seeking to position: " << position << std::endl;
	if (source.eof()) {
		std::cout << "Clearing EOF flag..." << std::endl;
		source.clear();
	}
	
	// Seek from the beginning of the file.
	std::cout << "Seeking from file beginning..." << std::endl;
	source.seekg(0);
	source.seekg(position);
	
	filePosition = position;
	
	return new_offset;
}


/* void signal_handler(int signal) {
	std::cout << "SIGINT handler called. Shutting down..." << std::endl;
	playerCon.signal();
} */


void shutdownFunction() {
	// Signal Player.
}


int main() {
	// Open file, get file size.
	std::string filename = "Big_Buck_Bunny_1080_10s_30MB.mp4";
	
	if (filename.length() == 0 ) {
		std::cerr << "Filename is empty" << std::endl;
		return false;
	}

	/* fs::path filePath(filename);
	if (!fs::exists(filePath)) { */
	Poco::File file(filename);
	if (!file.exists()) {
		std::cerr << "File '" << filename << "' doesn't exist." << std::endl;
		return false;
	}
	
	std::cout << "Opening file '" << filename << "'" << std::endl;
	
	if (source.is_open()) {
		source.close();
	}
	
	source.open(filename, std::ios::binary);
	if (!source.good()) {
		std::cerr << "Failed to read input file '" << filename << "'" << std::endl;
		return false;
	}
	
	filesize = file.getSize();
	eof = false;
	
	// Init SDL.
	if (!SdlRenderer::init()) {
		std::cerr << "Failed to init SDL. Aborting..." << std::endl;
		return 0;
	}
	
	SdlRenderer::showWindow();
	
	// Set up ffplay AVIO
	AVFormatContext* formatContext = 0;
	AVIOContext* ioContext = 0;

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
	
	// Start player.
	VideoState* is = 0;
	std::string input_filename = "";
	is = StreamHandler::stream_open(input_filename.c_str(), file_iformat, formatContext);
	if (!is) {
		av_log(NULL, AV_LOG_FATAL, "Failed to initialize VideoState!\n");
		return 1;
	}
	
	// Wait here until playback has finished.
	// The read thread in StreamHandler will signal this condition variable.
	/* playerMutex.lock();
	playerCon.wait(playerMutex);
	playerMutex.unlock(); */
	
	Player::event_loop(is);
	
	if (ioContext) {
		av_freep(&ioContext->buffer);
		av_freep(&ioContext);
	}
	
	if (is) {
		StreamHandler::stream_close(is);
		is = 0;
	}
	
	av_log(NULL, AV_LOG_INFO, "Terminating player...\n");
	
	return 0;
}
