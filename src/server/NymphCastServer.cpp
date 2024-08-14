/*
	NymphCastServer.cpp - Server that accepts NymphCast client sessions to play back audio.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
		
	2019/01/25, Maya Posch
*/


// Uncomment PROFILING to enable profiling mode.
// This disables the FFMPEG-based ffplay class and uses the ffplay-dummy driver instead.
// This driver dummy simulates an active ffplay player, with regular reads from the DataBuffer.
//#define PROFILING 1
// -----------------


#include <iostream>
#include <vector>
#include <queue>
#include <csignal>
#include <string>
#include <iterator>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <filesystem> 		// C++17
#include <set>
#include <fstream>
#include <ostream>
#include <condition_variable>

namespace fs = std::filesystem;

#ifdef PROFILING
#include "ffplaydummy.h"
#else
#include "ffplay/ffplay.h"
#endif

#include "ffplay/types.h"
#include "sdl_renderer.h"

#include "databuffer.h"
#include "screensaver.h"

#include <nymph/nymph.h>

#include "config_parser.h"
#include "sarge.h"

#include <Poco/Condition.h>
#include <Poco/Thread.h>
#include <Poco/URI.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/SQLite/SQLiteException.h>
#include <Poco/Timestamp.h>
#include <Poco/NumberFormatter.h>

using namespace Poco;

#include "INIReader.h"

#include "nyansd.h"

#include "nc_apps.h"
#include "gui.h"

#ifdef __ANDROID__
//#include <android_native_app_glue.h>

// Set up native logging by redirecting stdout to Android logging.
static int pfd[2];
static pthread_t thr;
static const char* tag = "NymphCastServer";

#include <android/log.h>

static void* thread_func(void*) {
    ssize_t rdsz;
    char buf[128];
    while ((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        //if (buf[rdsz - 1] == '\n') --rdsz; // Remove newline if it exists.
        buf[rdsz] = 0;  // add null-terminator
        __android_log_write(ANDROID_LOG_DEBUG, tag, buf);
    }
    
    return 0;
}


int start_logger(const char* app_name) {
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if (pthread_create(&thr, 0, thread_func, 0) == -1) { return -1; }
    pthread_detach(thr);
    return 0;
}
#endif


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
bool subtitle_disable = true;
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
std::atomic<bool> muted = { false };
std::atomic<uint32_t> muted_volume;

#ifndef __ANDROID__
#ifdef main
#undef main
#endif
#endif

struct CastClient {
	std::string name;
	int handle;
	bool sessionActive;
	uint32_t filesize;
};

// --- Globals ---
std::atomic<bool> playerPaused = { false };
std::atomic<bool> playerStopped = { false };	// Playback was stopped by the user.
Poco::Thread avThread;
Poco::Condition slavePlayCon;
Poco::Mutex slavePlayMutex;

//#ifndef _MSC_VER
// LCDProc client.
#include "lcdapi/include/LCDHeaders.h"
//#endif
std::atomic<bool> lcdapi_active = { false };
bool lcdproc_enabled = false;
// ---

#ifdef PROFILING
FfplayDummy ffplay;
#else
Ffplay ffplay;
#endif

const uint32_t nymph_seek_event = SDL_RegisterEvents(1);
std::string appsFolder;
std::atomic<bool> running = { true };
std::string loggerName = "NymphCastServer";

NCApps nc_apps;
std::map<int, CastClient> clients;

// -- BLOCK SIZE ---
// This defines the size of the data blocks requested from the client with a read request.
// Defined in kilobytes.
uint32_t readBlockSize = 200;
// ---


// Data structure.
struct SessionParams {
	int max_buffer;
};

struct NymphCastSlaveRemote {
	std::string name;
	std::string ipv4;
	std::string ipv6;
	uint16_t port;
	uint32_t handle;
	int64_t delay;
};

NcsMode serverMode = NCS_MODE_STANDALONE;
std::vector<NymphCastSlaveRemote> slave_remotes;
uint32_t slaveLatencyMax = 0;	// Max latency to slave remote in milliseconds.
// ---



// --- MEDIA READ CALLBACK ---
// Called by slave remotes during a read request.
void MediaReadCallback(uint32_t session, NymphMessage* msg, void* data) {
	// Handled by the usual client & master routines.
}


// --- MEDIA SEEK CALLBACK ---
// Called by a slave remote during a seek request.
void MediaSeekCallback(uint32_t session, NymphMessage* msg, void* data) {
	// Handled by the usual client & master routines.
}


// --- MEDIA STOP CALLBACK ---
// Called by a slave remote as a stop notification.
void MediaStopCallback(uint32_t session, NymphMessage* msg, void* data) {
	// 
}


// --- MEDIA STATUS CALLBACK ---
// Called by a slave remote during a status update.
void MediaStatusCallback(uint32_t session, NymphMessage* msg, void* data) {
	// Handled by the usual client & master routines.
}


// --- GET PLAYBACK STATUS ---
std::map<std::string, NymphPair>* getPlaybackStatus() {
	// Set the playback status.
	// We're sending back whether we are playing something currently. If so, also includes:
	// * duration of media in seconds.
	// * position in the media, in seconds with remainder.
	// * title of the media, if available.
	// * artist of the media, if available.
	std::map<std::string, NymphPair>* pairs = new std::map<std::string, NymphPair>();
	NymphPair pair;
	std::string* key;
	if (ffplay.playbackActive()) {
		// Distinguish between playing and paused for the player.
		if (playerPaused) {
			key = new std::string("status");
			pair.key = new NymphType(key, true);
			pair.value = new NymphType((uint32_t) NYMPH_PLAYBACK_STATUS_PAUSED);
			pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		}
		else {
			key = new std::string("status");
			pair.key = new NymphType(key, true);
			pair.value = new NymphType((uint32_t) NYMPH_PLAYBACK_STATUS_PLAYING);
			pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		}
		
		key = new std::string("playing");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType(true);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("stopped");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType(false);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("duration");
		pair.key = new NymphType(key, true);
		//pair.value = new NymphType(FileMetaInfo::getDuration());
		pair.value = new NymphType(file_meta.getDuration());
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("position");
		pair.key = new NymphType(key, true);
		//pair.value = new NymphType(FileMetaInfo::getPosition());
		//pair.value = new NymphType(file_meta.getPosition());
		pair.value = new NymphType(file_meta.position);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("title");
		pair.key = new NymphType(key, true);
		//std::string* val = new std::string(FileMetaInfo::getTitle());
		std::string* val = new std::string(file_meta.getTitle());
		pair.value = new NymphType(val, true);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("artist");
		pair.key = new NymphType(key, true);
		//val = new std::string(FileMetaInfo::getArtist());
		val = new std::string(file_meta.getArtist());
		pair.value = new NymphType(val, true);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("volume");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType((uint8_t) audio_volume.load());
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("subtitle_disable");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType(subtitle_disable);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
	}
	else {
		if (playerStopped) {
			// Stopped by user.
			key = new std::string("stopped");
			pair.key = new NymphType(key, true);
			pair.value = new NymphType(true);
			pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		}
		else {
			key = new std::string("stopped");
			pair.key = new NymphType(key, true);
			pair.value = new NymphType(false);
			pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		}
		
		key = new std::string("status");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType((uint32_t) NYMPH_PLAYBACK_STATUS_STOPPED);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("playing");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType(false);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("duration");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType((uint64_t) 0);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("position");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType((double) 0.0);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("title");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType((char*) 0, 0);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("artist");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType((char*) 0, 0);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("volume");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType((uint8_t) audio_volume.load());
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("subtitle_disable");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType(subtitle_disable);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
	}
	
	return pairs;
}


// --- SEND STATUS UPDATE ---
void sendStatusUpdate(uint32_t handle) {
	// Call the status update callback with the current playback status.
	std::vector<NymphType*> values;
	std::map<std::string, NymphPair>* status = getPlaybackStatus();
	values.push_back(new NymphType(status, true));
	NymphType* resVal = 0;
	std::string result;
	if (!NymphRemoteClient::callCallback(handle, "MediaStatusCallback", values, result)) {
		NYMPH_LOG_ERROR("Calling media status callback failed: " + result);
		return;
	}
}


// --- SEND GLOBAL STATUS UPDATE ---
// Send playback status update to all connected clients.
void sendGlobalStatusUpdate() {
	NYMPH_LOG_INFORMATION("Sending status update to all " + Poco::NumberFormatter::format(clients.size()) 
					+ " clients.");
					
	std::map<std::string, NymphPair>* pairs = getPlaybackStatus();
	std::map<int, CastClient>::const_iterator it;
	for (it = clients.begin(); it != clients.end();/**/) {
		NYMPH_LOG_DEBUG("Client ID: " + Poco::NumberFormatter::format(it->first) + "/" + it->second.name);
		
		// Call the status update callback with the current playback status.
		NymphType* resVal = 0;
		std::string result;
		std::vector<NymphType*> values;
		values.push_back(new NymphType(pairs));
		if (!NymphRemoteClient::callCallback(it->first, "MediaStatusCallback", values, result)) {
			NYMPH_LOG_ERROR("Calling media status callback failed: " + result);
			
			// An error here very likely means that the client no long exists. Remove it.
			it = clients.erase(it);
		}
		else {
			++it;
		}
	}
	
	// Delete the map and its values.
	// TODO: persist the data somehow.
	std::map<std::string, NymphPair>::iterator sit;
	for (sit = pairs->begin(); sit != pairs->end(); ++sit) {
		delete sit->second.key;
		delete sit->second.value;
	}
	
	delete pairs;
}


// --- START SLAVE PLAYBACK ---
// [Master] Signal slaves that they can begin playback.
bool startSlavePlayback() {
	// Start slaves according to the average connection latency.
	Poco::Timestamp ts;
	int64_t now = (int64_t) ts.epochMicroseconds();
	//then = now + (slaveLatencyMax * 2);
		
	// Timing: 	Multiply the max slave latency by the number of slaves. After sending this delay
	// 			to the first slave (minus half its recorded latency), 
	// 			subtract the time it took to send to this slave from the
	//			first delay, then send this new delay to the second slave, and so on.
	int64_t countdown = slaveLatencyMax * slave_remotes.size();
	
	int64_t then = 0;
	for (int i = 0; i < slave_remotes.size(); ++i) {
		NymphCastSlaveRemote& rm = slave_remotes[i];
		//then = slaveLatencyMax - rm.delay;
		then = countdown - (rm.delay / 2);
			
		int64_t send = (int64_t) ts.epochMicroseconds();
		
		// Prepare data vector.
		std::vector<NymphType*> values;
		values.push_back(new NymphType(then));
			
		std::string result;
		NymphType* returnValue = 0;
		if (!NymphRemoteServer::callMethod(rm.handle, "slave_start", values, returnValue, result)) {
			NYMPH_LOG_ERROR("Calling slave_start failed: " + result);
			return false;
		}
			
		delete returnValue;
			
		int64_t receive = (int64_t) ts.epochMicroseconds();
			
		countdown -= (receive - send);
	}
		
	// Wait out the countdown before returning.
	std::condition_variable cv;
	std::mutex cv_m;
	std::unique_lock<std::mutex> lk(cv_m);
	std::chrono::microseconds dur(countdown);
	while (cv.wait_for(lk, dur) != std::cv_status::timeout) { }
	
	/* for (uint32_t i = 0; i < slave_remotes.size(); ++i) {
		//
		NymphType* resVal = 0;
		std::string result;
		std::vector<NymphType*> values;
		if (!NymphRemoteClient::callCallback(slave_remotes[i].handle, "slave_start", values, result)) {
			NYMPH_LOG_ERROR("Calling slave_start failed: " + result);
			return false;
		}
	} */
	
	return true;
}


// --- DATA REQUEST HANDLER ---
// Allows the DataBuffer to request more file data from a client.
bool dataRequestHandler(uint32_t session) {
	if (DataBuffer::seeking()) {
		NYMPH_LOG_ERROR("Cannot request data while seeking. Abort.");
		return false;
	}
	
	DataBuffer::dataRequestPending = true;
	
	NYMPH_LOG_INFORMATION("Asking for data...");

	// Request more data.
	std::vector<NymphType*> values;
	values.push_back(new NymphType(readBlockSize));
	std::string result;
	if (!NymphRemoteClient::callCallback(DataBuffer::getSessionHandle(), "MediaReadCallback", values, result)) {
		NYMPH_LOG_ERROR("Calling callback failed: " + result);
		return false;
	}
	
	return true;
}


// --- SEEKING HANDLER ---
void seekingHandler(uint32_t session, int64_t offset) {
	if (DataBuffer::seeking()) {
		if (serverMode == NCS_MODE_MASTER) {
			// Send data buffer reset notification. This ensures that those are all reset as well.
			for (int i = 0; i < slave_remotes.size(); ++i) {
				NymphCastSlaveRemote& rm = slave_remotes[i];
				std::vector<NymphType*> values;
				std::string result;
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(rm.handle, "slave_buffer_reset", values, returnValue, result)) {
					// TODO: Handle error. Check return value.
					NYMPH_LOG_ERROR("Calling slave_buffer_reset failed.");
				}
				
				delete returnValue;
			}	
		}
		
		// Send message to client indicating that we're seeking in the file.
		std::vector<NymphType*> values;
		values.push_back(new NymphType((uint64_t) offset));
		values.push_back(new NymphType(readBlockSize));
		std::string result;
		NymphType* resVal = 0;
		if (!NymphRemoteClient::callCallback(session, "MediaSeekCallback", values, result)) {
			NYMPH_LOG_ERROR("Calling media seek callback failed: " + result);
			return;
		}
				
		return; 
	}
}


// --- FINISH PLAYBACK ---
// Called at the end of playback of a stream or file.
// If a stream is queued, play it, otherwise end playback.
void finishPlayback() {
	// Send message to client indicating that we're done.
	uint32_t handle = DataBuffer::getSessionHandle();
	std::vector<NymphType*> values;
	std::string result;
	if (!NymphRemoteClient::callCallback(handle, "MediaStopCallback", values, result)) {
		NYMPH_LOG_ERROR("Calling media stop callback failed: " + result);
		return;
	}
	
	// Update the LCDProc daemon if enabled.
	if (lcdproc_enabled) {
		// TODO: Clear the screen?
	}
	
	// Start the Screensaver here for now.
	if (!display_disable) {
		if (gui_enable) {
			// Return to GUI. Hide window.
			SDL_Event event;
			event.type = SDL_KEYDOWN;
			event.key.keysym.sym = SDLK_UNDERSCORE;
			SDL_PushEvent(&event);
			
			Gui::active = true;
			Gui::resumeCv.notify_one();
		}
		else if (screensaver_enable) {
			// Start screensaver.
			ScreenSaver::start(15);
		}
		else {
			// Hide window.
			SDL_Event event;
			event.type = SDL_KEYDOWN;
			event.key.keysym.sym = SDLK_UNDERSCORE;
			SDL_PushEvent(&event);
		}
	}
}


// --- STREAM TRACK ---
// Attempt to stream from the indicated URL.
bool streamTrack(std::string url) {
	// TODO: Check that we're not still streaming, otherwise queue the URL.
	// TODO: allow to cancel any currently playing track/empty queue?
	if (ffplay.playbackActive()) {
		// Add to queue.
		DataBuffer::addStreamTrack(url);
		
		return true;
	}
	
	// Schedule next track URL.
	ffplay.streamTrack(url);
	
	// Send status update to client.
	sendStatusUpdate(DataBuffer::getSessionHandle());
	
	return true;
}


void signal_handler(int signal) {
	NYMPH_LOG_INFORMATION("SIGINT handler called. Shutting down...");
	SdlRenderer::stop_event_loop();
}


// Callback for the connect function.
NymphMessage* connectClient(int session, NymphMessage* msg, void* data) {
	NYMPH_LOG_INFORMATION("Received message for session: " + Poco::NumberFormatter::format(session)
							+ ", msg ID: " + Poco::NumberFormatter::format(msg->getMessageId()));
	
	std::string clientStr = msg->parameters()[0]->getString();
	NYMPH_LOG_INFORMATION("Client string: " + clientStr);
	
	// TODO: check whether we're not operating in slave or master mode already.
	NYMPH_LOG_INFORMATION("Switching to stand-alone server mode.");
	serverMode = NCS_MODE_STANDALONE;
	
	// Register this client with its ID. Return error if the client ID already exists.
	NymphMessage* returnMsg = msg->getReplyMessage();
	std::map<int, CastClient>::iterator it;
	it = clients.find(session);
	NymphType* retVal = 0;
	if (it != clients.end()) {
		// Client ID already exists, abort.
		retVal = new NymphType(false);
	}
	else {
		CastClient c;
		c.name = clientStr;
		c.handle = session;
		c.sessionActive = false;
		c.filesize = 0;
		clients.insert(std::pair<int, CastClient>(session, c));
		retVal = new NymphType(true);
	}
	
	// Send the client the current playback status.
	std::vector<NymphType*> values;
	std::map<std::string, NymphPair>* status = getPlaybackStatus();
	values.push_back(new NymphType(status, true));
	std::string result;
	if (!NymphRemoteClient::callCallback(session, "MediaStatusCallback", values, result)) {
		NYMPH_LOG_ERROR("Calling media status callback failed: " + result);
	}
	
	returnMsg->setResultValue(retVal);
	msg->discard();
	
	return returnMsg;
}


// --- CONNECT MASTER ---
// Master server calls this to turn this server instance into a slave.
// This disables the regular client connection functionality for the duration of the master/slave
// session.
// Returns the timestamp when the message was received.
// sint64 connectMaster(sint64)
NymphMessage* connectMaster(int session, NymphMessage* msg, void* data) {
	NYMPH_LOG_INFORMATION("Received master connect request, slave mode initiation requested.");
	
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Switch to slave mode, if possible.
	// Return error if we're currently playing content in stand-alone mode.
	if (ffplay.playbackActive()) {
		returnMsg->setResultValue(new NymphType((int64_t) 0));
	}
	else {
		// FIXME: for now we just return the current time.
		NYMPH_LOG_INFORMATION("Switching to slave server mode.");
		serverMode = NCS_MODE_SLAVE;
		//DataBuffer::setFileSize(it->second.filesize);
		DataBuffer::setSessionHandle(session);
		
		Poco::Timestamp ts;
		int64_t now = (int64_t) ts.epochMicroseconds();
		returnMsg->setResultValue(new NymphType(now));
	}
	
	// TODO: Obtain timestamp, compare with current time.
	//time_t then = ((NymphSint64*) msg->parameters()[0])->getValue();
	
	// TODO: Send delay request to master.
	
	// TODO: Determine final latency and share with master.
	
	msg->discard();
	
	return returnMsg;
}


// --- RECEIVE DATA MASTER ---
// Receives data chunks for playback from a master receiver. (Slave-only)
// uint8 receiveDataMaster(blob data, bool done, sint64 when)
NymphMessage* receiveDataMaster(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Extract data blob and add it to the buffer.
	NymphType* mediaData = msg->parameters()[0];
	bool done = msg->parameters()[1]->getBool();
	
	// Write string into buffer.
	DataBuffer::write(mediaData->getChar(), mediaData->string_length());
	
	// Playback is started in its own function, which is called by the master when it's ready.
	int64_t then = 0;
	if (!ffplay.playbackActive()) {
		int64_t when = msg->parameters()[2]->getInt64();
		
		// Start the player when the delay in 'when' has been reached.
		/* std::condition_variable cv;
		std::mutex cv_m;
		std::unique_lock<std::mutex> lk(cv_m);
		//std::chrono::system_clock::time_point then = std::chrono::system_clock::from_time_t(when);
		std::chrono::microseconds dur(when);
		std::chrono::time_point<std::chrono::system_clock> then(dur);
		//while (cv.wait_until(lk, then) != std::cv_status::timeout) { }
		while (cv.wait_for(lk, dur) != std::cv_status::timeout) { } */
		
		// Start player.
		ffplay.playTrack(when);
	}
	
	if (done) {
		DataBuffer::setEof(done);
	}
	
	msg->discard();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	return returnMsg;
}


// --- SLAVE START ---
// Receives data chunks for playback from a master receiver. (Slave-only)
// uint8 slave_start(int64 when)
NymphMessage* slave_start(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Extract data blob and add it to the buffer.
	//NymphType* mediaData = msg->parameters()[0];
	//bool done = msg->parameters()[1]->getBool();
	int64_t when = msg->parameters()[0]->getInt64();
	
	if (!ffplay.playbackActive()) {
		// Start the player when the delay in 'when' has been reached.
		std::condition_variable cv;
		std::mutex cv_m;
		std::unique_lock<std::mutex> lk(cv_m);
		//std::chrono::system_clock::time_point then = std::chrono::system_clock::from_time_t(when);
		std::chrono::microseconds dur(when);
		std::chrono::time_point<std::chrono::system_clock> then(dur);
		//while (cv.wait_until(lk, then) != std::cv_status::timeout) { }
		while (cv.wait_for(lk, dur) != std::cv_status::timeout) { }
	}
	
	// Trigger the playback start condition variable that will resume the read_thread of this slave
	// receiver's ffplay module.
	if (serverMode == NCS_MODE_SLAVE) {
		slavePlayCon.signal();
	}
	
	msg->discard();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	return returnMsg;
}


// Client disconnects from server.
// bool disconnect()
NymphMessage* disconnect(int session, NymphMessage* msg, void* data) {
	
	// Remove the client ID from the list.
	std::map<int, CastClient>::iterator it;
	it = clients.find(session);
	if (it != clients.end()) {
		clients.erase(it);
	}
	
	NYMPH_LOG_INFORMATION("Current server mode: " + Poco::NumberFormatter::format(serverMode));
	
	// Disconnect any slave remotes if we're connected.
	if (serverMode == NCS_MODE_MASTER) {
		NYMPH_LOG_DEBUG("# of slave remotes: " + 
								Poco::NumberFormatter::format(slave_remotes.size()));
		for (int i = 0; i < slave_remotes.size(); ++i) {
			// Disconnect from slave remote.
			NymphCastSlaveRemote& rm = slave_remotes[i];
			NYMPH_LOG_DEBUG("Disconnecting slave: " + rm.name);
			std::string result;
			if (!NymphRemoteServer::disconnect(rm.handle, result)) {
				// Failed to connect, error out. Disconnect from any already connected slaves.
				NYMPH_LOG_ERROR("Slave disconnect error: " + result);
			}
		}
		
		slave_remotes.clear();
	}
	
	NYMPH_LOG_INFORMATION("Switching to stand-alone server mode.");
	serverMode = NCS_MODE_STANDALONE;
	
	NymphMessage* returnMsg = msg->getReplyMessage();
	returnMsg->setResultValue(new NymphType(true));
	msg->discard();
	
	return returnMsg;
}


// Client starts a session.
// Return value: OK (0), ERROR (1).
// int session_start(struct fileInfo)
NymphMessage* session_start(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Set up a new session instance for the client.
	std::map<int, CastClient>::iterator it;
	it = clients.find(session);
	if (it == clients.end()) {
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg;
	}
	
	// Obtain the filesize from the client, which we use with the buffer management.
	NymphType* fileInfo = msg->parameters()[0];
	NymphType* num = 0;
	if (!fileInfo->getStructValue("filesize", num)) {
		NYMPH_LOG_FATAL("Didn't find entry 'filesize'. Aborting...");
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg;
	}
	
	it->second.filesize = num->getUint32();
	
	// Check whether we're already playing or not. If we continue here, this will forcefully 
	// end current playback.
	//	FIXME:	=> this likely happens due to a status update glitch. Fix by sending back status update
	// 			along with error?
	if (ffplay.playbackActive()) {
		NYMPH_LOG_ERROR("Trying to start a new session with session already active. Abort.");
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg;
	}
	
	// If the AV thread is currently running, we wait until it's quit.
	//if (avThread.joinable()) { avThread.join(); } // FIXME: C++11 version
	/* if (avThread.isRunning()) {
		std::cout << "AV thread active: waiting for join..." << std::endl;
		bool ret = avThread.tryJoin(100);
		if (!ret) {
			// Player thread is still running, meaning we cannot proceed. Error out.
			std::cerr << "Joining failed: aborting new session..." << std::endl;
			returnMsg->setResultValue(new NymphType((uint8_t) 1));
			msg->discard();

			return returnMsg;
		}
	} */
	
	NYMPH_LOG_INFORMATION("Starting new session for file with size: " +
							Poco::NumberFormatter::format(it->second.filesize));
	
	DataBuffer::setFileSize(it->second.filesize);
	DataBuffer::setSessionHandle(session);
	
	// Start calling the client's read callback method to obtain data. Once the data buffer
	// has been filled sufficiently, start the playback.
	if (!DataBuffer::start()) {
		NYMPH_LOG_ERROR("Failed to start buffering. Abort.");
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg;
	}
		
	it->second.sessionActive = true;
	
	// Stop screensaver.
	if (!video_disable) {
		if (gui_enable) {
			// Show window.
			SDL_Event event;
			event.type = SDL_KEYDOWN;
			event.key.keysym.sym = SDLK_MINUS;
			SDL_PushEvent(&event);
		}
		else if (screensaver_enable) {
			ScreenSaver::stop();
		}
		else {
			// Show window.
			SDL_Event event;
			event.type = SDL_KEYDOWN;
			event.key.keysym.sym = SDLK_MINUS;
			SDL_PushEvent(&event);
		}
	}
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// Client sends meta data for the track.
// Returns: OK (0), ERROR (1).
// int session_meta(string artist, string album, int track, string name)
NymphMessage* session_meta(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// X unused function.
	
	msg->discard();
	
	return returnMsg;
}


// --- SESSION ADD SLAVE ---
// Client sends list of slave server which this server instance should control.
// Returns: OK (0), ERROR (1).
// int session_add_slave(array servers);
NymphMessage* session_add_slave(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Disconnect slaves and clear array.
	// TODO: Maybe merge this with proper session management.
	for (uint32_t i = 0; i < slave_remotes.size(); ++i) {
		NymphCastSlaveRemote& rm = slave_remotes[i];
		std::string result;
		if (!NymphRemoteServer::disconnect(rm.handle, result)) {
			// Failed to connect, error out. Disconnect from any already connected slaves.
			NYMPH_LOG_ERROR("Slave disconnection error: " + result);
			
			returnMsg->setResultValue(new NymphType((uint8_t) 1));
			msg->discard();
			
			return returnMsg;
		}
	}
	
	slave_remotes.clear();
	slaveLatencyMax = 0;
	
	// Extract the array.
	std::vector<NymphType*>* remotes = msg->parameters()[0]->getArray();
	for (int i = 0; i < (*remotes).size(); ++i) {
		std::map<std::string, NymphPair>* pairs = (*remotes)[i]->getStruct();
		NymphCastSlaveRemote remote;
		NymphType* value = 0;
		(*remotes)[i]->getStructValue("name", value);
		remote.name = std::string(value->getChar(), value->string_length());
		
		(*remotes)[i]->getStructValue("ipv4", value);
		remote.ipv4 = std::string(value->getChar(), value->string_length());
		
		(*remotes)[i]->getStructValue("ipv6", value);
		remote.ipv6 = std::string(value->getChar(), value->string_length());
		
		(*remotes)[i]->getStructValue("port", value);
		remote.port = value->getUint16();
		remote.handle = 0;
		remote.delay = 0;
	
		slave_remotes.push_back(remote);
	}
	
	// Validate that each slave remote is accessible and determine latency.
	for (int i = 0; i < slave_remotes.size(); ++i) {
		// Establish RPC connection to remote. Starts the PTP-like handshake.
		NymphCastSlaveRemote& rm = slave_remotes[i];
		std::string result;
		if (!NymphRemoteServer::connect(rm.ipv4, 4004, rm.handle, 0, result)) {
			// Failed to connect, error out. Disconnect from any already connected slaves.
			NYMPH_LOG_ERROR("Slave connection error: " + result);
			for (; i >= 0; --i) {
				NymphCastSlaveRemote& drm = slave_remotes[i];
				NymphRemoteServer::disconnect(drm.handle, result);
			}
			
			returnMsg->setResultValue(new NymphType((uint8_t) 1));
			msg->discard();
			
			return returnMsg;
		}
		
		// Attempt to start slave mode on the remote.
		// Send the current timestamp to the slave remote as part of the latency determination.
		Poco::Timestamp ts;
		int64_t now = (int64_t) ts.epochMicroseconds();
		std::vector<NymphType*> values;
		values.push_back(new NymphType(now));
		NymphType* returnValue = 0;
		if (!NymphRemoteServer::callMethod(rm.handle, "connectMaster", values, returnValue, result)) {
			NYMPH_LOG_ERROR("Slave connect master failed: " + result);
			// TODO: disconnect from slave remotes.
			returnMsg->setResultValue(new NymphType((uint8_t) 1));
			msg->discard();
			
			return returnMsg;
		}
		
		// Get new time. This should be roughly twice the latency to the slave remote.
		ts.update();
		int64_t pong = ts.epochMicroseconds();
		time_t theirs = returnValue->getInt64();
		delete returnValue;
		if (theirs == 0) {
			NYMPH_LOG_ERROR("Configuring remote as slave failed.");
			// TODO: disconnect from slave remotes.
			returnMsg->setResultValue(new NymphType((uint8_t) 1));
			msg->discard();
			
			return returnMsg;
		}
		
		// Use returned time stamp to calculate the delay.
		// FIXME: using stopwatch-style local time to determine latency for now.
		
		//rm.delay = theirs - now;
		rm.delay = pong - now;
		NYMPH_LOG_DEBUG("Slave delay: " + Poco::NumberFormatter::format(rm.delay) + 
							" microseconds.");
		NYMPH_LOG_DEBUG("Current max slave delay: " + 
							Poco::NumberFormatter::format(slaveLatencyMax));
		if (rm.delay > slaveLatencyMax) { 
			slaveLatencyMax = rm.delay;
			NYMPH_LOG_DEBUG("Max slave latency increased to: " + 
								Poco::NumberFormatter::format(slaveLatencyMax) + " microseconds.");
		}
	}
	
	NYMPH_LOG_INFORMATION("Switching to master server mode.");
	serverMode = NCS_MODE_MASTER;
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// Client sends a chunk of track data.
// Returns: OK (0), ERROR (1).
// int session_data(string buffer, boolean done)
NymphMessage* session_data(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Get iterator to the session instance for the client.
	std::map<int, CastClient>::iterator it;
	it = clients.find(session);
	if (it == clients.end()) {
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		return returnMsg;
	}
	
	// Safely write the data for this session to the buffer.
	NymphType* mediaData = msg->parameters()[0];
	bool done = msg->parameters()[1]->getBool();
	
	// Update EOF status.
	DataBuffer::setEof(done);
	
	// Write string into buffer.
	DataBuffer::write(mediaData->getChar(), mediaData->string_length());
	
	// If passing the message through to slave remotes, add the timestamp to the message.
	// This timestamp is the current time plus the largest master-slave latency times 2.
	// Timing: 	Multiply the max slave latency by the number of slaves. After sending this delay
	// 			to the first slave (minus half its recorded latency), 
	// 			subtract the time it took to send to this slave from the
	//			first delay, then send this new delay to the second slave, and so on.
	int64_t then = 0;
	if (serverMode == NCS_MODE_MASTER) {
		Poco::Timestamp ts;
		int64_t now = 0;
		int64_t countdown = 0;
		if (!ffplay.playbackActive()) {
			now = (int64_t) ts.epochMicroseconds();
			//then = now + (slaveLatencyMax * 2);
			countdown = slaveLatencyMax * slave_remotes.size();
		}
		
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			
			int64_t send = 0;
			if (!ffplay.playbackActive()) {
				//then = slaveLatencyMax - rm.delay;
				then = countdown - (rm.delay / 2);
				ts.update();
				send = (int64_t) ts.epochMicroseconds();
			}
		
			// Prepare data vector.
			NymphType* media = new NymphType((char*) mediaData->getChar(), mediaData->string_length());
			NymphType* doneBool = new NymphType(done);
			std::vector<NymphType*> values;
			values.push_back(media);
			values.push_back(doneBool);
			values.push_back(new NymphType(then));
			
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "receiveDataMaster", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
			
			if (!ffplay.playbackActive()) {
				ts.update();
				int64_t receive = (int64_t) ts.epochMicroseconds();
			
				countdown -= (receive - send);
			}
		}
		
		if (!ffplay.playbackActive()) {
			// Wait out the countdown.
			std::condition_variable cv;
			std::mutex cv_m;
			std::unique_lock<std::mutex> lk(cv_m);
			std::chrono::microseconds dur(countdown);
			while (cv.wait_for(lk, dur) != std::cv_status::timeout) { }
		}
	}
	
	// Start the player if it hasn't yet. This ensures we have a buffer ready.
	// TODO: take into account delay of slave remotes before starting local playback.
	if (!ffplay.playbackActive()) {
		// if we're in master mode, only start after the slaves are starting as well.
		// In slave mode, we execute time-critical commands like playback start when
		/* if (serverMode == NCS_MODE_MASTER) {
			// We use the calculated latency to the slave to determine when to send the play
			// command to the slave.
			// TODO:
		} */
		
		/* if (serverMode == NCS_MODE_MASTER) {
			// Start the player when the delay in 'then' has been reached.
			std::condition_variable cv;
			std::mutex cv_m;
			std::unique_lock<std::mutex> lk(cv_m);
			std::chrono::microseconds dur(slaveLatencyMax);
			//std::chrono::time_point<std::chrono::system_clock> when(dur);
			while (cv.wait_for(lk, dur) != std::cv_status::timeout) { }
		} */
		
		// Start playback locally.
		ffplay.playTrack();
		
		playerStopped = false;
	}
	else {
		// Send status update to clients.
		sendGlobalStatusUpdate();
	}
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// Client ends the session.
// Returns: OK (0), ERROR (1).
// int session_end()
NymphMessage* session_end(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Mark session as inactive.
	std::map<int, CastClient>::iterator it;
	it = clients.find(session);
	if (it == clients.end()) {
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg;
	}
	
	it->second.sessionActive = false;
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- SLAVE BUFFER RESET ---
// Called to reset the slave's local data buffer.
// Returns: OK (0), ERROR (1).
// int slave_buffer_reset()
NymphMessage* slave_buffer_reset(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Call DataBuffer's reset function.
	if (!DataBuffer::reset()) {
		NYMPH_LOG_ERROR("Resetting data buffer failed.");
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg;
	}
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- VOLUME SET ---
// uint8 volume_set(uint8 volume)
NymphMessage* volume_set(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	uint8_t volume = msg->parameters()[0]->getUint8();
	
	if (serverMode == NCS_MODE_MASTER) {
		std::vector<NymphType*> values;
		values.push_back(new NymphType(volume));
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "volume_set", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	audio_volume = volume;
	ffplay.setVolume(volume);
	
	// Inform all clients of this update.
	sendGlobalStatusUpdate();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- VOLUME UP ---
// uint8 volume_up()
NymphMessage* volume_up(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "volume_up", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_0;
	SDL_PushEvent(&event);
	
	// TODO: update global audio_volume variable.
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- VOLUME DOWN ---
// uint8 volume_down()
NymphMessage* volume_down(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "volume_down", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_9;
	SDL_PushEvent(&event);
	
	// TODO: update global audio_volume variable.
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- VOLUME MUTE ---
// uint8 volume_mute()
NymphMessage* volume_mute(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "volume_mute", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	// Update muted audio state.
	muted = !muted;
	if (muted) {
		muted_volume = audio_volume.load();
		audio_volume = 0;
	}
	else {
		audio_volume = muted_volume.load();
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_m;
	SDL_PushEvent(&event);
	
	sendGlobalStatusUpdate();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- PLAYBACK START ---
// uint8 playback_start()
NymphMessage* playback_start(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "playback_start", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_SPACE;
	SDL_PushEvent(&event);
	
	playerPaused = false;
	
	// Send status update to clients.
	sendGlobalStatusUpdate();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- PLAYBACK STOP ---
// uint8 playback_stop()
NymphMessage* playback_stop(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "playback_stop", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_ESCAPE;
	SDL_PushEvent(&event);
	
	playerPaused = false;
	playerStopped = true;
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- PLAYBACK PAUSE ---
// uint8 playback_pause()
NymphMessage* playback_pause(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "playback_pause", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_p;
	SDL_PushEvent(&event);
	
	playerPaused = !playerPaused;
	
	// Send status update to clients.
	sendGlobalStatusUpdate();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- PLAYBACK REWIND ---
// uint8 playback_rewind()
// TODO:
NymphMessage* playback_rewind(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- PLAYBACK FORWARD ---
// uint8 playback_forward()
// TODO:
NymphMessage* playback_forward(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- CYCLE AUDIO ---
// uint8 cycle_audio()
NymphMessage* cycle_audio(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "cycle_audio", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_a;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- CYCLE VIDEO ---
// uint8 cycle_video()
NymphMessage* cycle_video(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "cycle_video", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_v;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- CYCLE SUBTITLE ---
// uint8 cycle_subtitle()
NymphMessage* cycle_subtitle(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::vector<NymphType*> values;
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "cycle_subtitle", values, returnValue, result)) {
				// TODO:
			}
			
			delete returnValue;
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_t;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- PLAYBACK SEEK ---
// uint8 playback_seek(uint64)
NymphMessage* playback_seek(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	std::vector<NymphType*>* valArray = msg->parameters()[0]->getArray();
	NymphSeekType type = static_cast<NymphSeekType>((*valArray)[0]->getUint8());
	if (type == NYMPH_SEEK_TYPE_PERCENTAGE) {
		uint8_t percentage = (*valArray)[1]->getUint8();
		
		// Sanity check.
		// We accept a value from 0 - 100.
		if (percentage > 100) { percentage = 100; }
	
		// Create mouse event structure.
		SDL_Event event;
		event.type = nymph_seek_event;
		event.user.code = NYMPH_SEEK_EVENT;
		event.user.code = percentage;
		SDL_PushEvent(&event);
	}
	else if (type == NYMPH_SEEK_TYPE_BYTES) {
		NYMPH_LOG_FATAL("Error: Seeking by byte offset is not implemented.");
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		return returnMsg;
	}
	else {
		NYMPH_LOG_FATAL("Error: Unknown seeking type requested: " + 
										Poco::NumberFormatter::format(type));
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		return returnMsg;
	}
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- SUBTITLES SET --
// uint8 subtitles_set()
NymphMessage* subtitles_set(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Set global subtitle_disable variable.
	bool stat = msg->parameters()[0]->getBool();
	subtitle_disable = !stat; // Invert to match intent.
	
	// Send status update to clients.
	sendGlobalStatusUpdate();
	
	returnMsg->setResultValue(new NymphType((uint8_t) 0));
	msg->discard();
	
	return returnMsg;
}


// --- PLAYBACK URL ---
// uint8 playback_url(string)
NymphMessage* playback_url(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	std::string url = msg->parameters()[0]->getString();
	bool ret = streamTrack(url);
	
	NymphType* retval = new NymphType((uint8_t) 1);
	if (ret) {
		if (serverMode == NCS_MODE_MASTER) {
			std::vector<NymphType*> values;
			std::string* tUrl = new std::string(url);
			values.push_back(new NymphType(tUrl, true));
			for (int i = 0; i < slave_remotes.size(); ++i) {
				NymphCastSlaveRemote& rm = slave_remotes[i];
				std::string result;
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(rm.handle, "playback_url", values, returnValue, result)) {
					// TODO:
				}
				
				delete returnValue;
			}
		}
		
		retval->setValue((uint8_t) 0);
	}
	
	// Send status update to client.
	sendStatusUpdate(DataBuffer::getSessionHandle());
	
	returnMsg->setResultValue(retval);
	msg->discard();
	
	return returnMsg;
}
	


// --- PLAYBACK STATUS ---
// struct playback_status()
NymphMessage* playback_status(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
		
	returnMsg->setResultValue(new NymphType(getPlaybackStatus(), true));
	msg->discard();
	
	return returnMsg;
}


// --- APP LIST ---
// string app_list()
// Returns a list of registered apps, separated by a newline and ending with a newline.
NymphMessage* app_list(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Obtain and serialise the app names list.
	//std::vector<std::string> appNames = NymphCastApps::appNames();
	std::vector<std::string> appNames = nc_apps.appNames();
	std::string* names = new std::string();
	std::vector<std::string>::const_iterator it = appNames.cbegin();
	while (it != appNames.cend()) {
		names->append(*it);
		names->append("\n");
		it++;
	}
	
	returnMsg->setResultValue(new NymphType(names, true));
	msg->discard();
	
	return returnMsg;
}


// --- APP SEND ---
// string app_send(string appId, string data, uint8 format)
NymphMessage* app_send(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Validate the application ID, try to find running instance, else launch new app instance.
	std::string appId = msg->parameters()[0]->getString();
	std::string message = msg->parameters()[1]->getString();
	
	// Get the desired output format.
	// 0 - CLI	- Text with tab (\t) separators and \n terminator.
	// 1 - HTML	- HTML format.
	uint8_t format = msg->parameters()[2]->getUint8();
	
	// Find the application details.
	std::string* result = new std::string();
	NymphCastApp app = nc_apps.findApp(appId);
	if (app.id.empty()) {
		NYMPH_LOG_FATAL("Failed to find a matching application for '" + appId + "'.");
		returnMsg->setResultValue(new NymphType(result, true));
		msg->discard();
			
		return returnMsg;
	}
	
	NYMPH_LOG_INFORMATION("Found " + appId + " app.");
	
	if (!nc_apps.runApp(appId, message, format, *result)) {
		NYMPH_LOG_ERROR("Error running app: " + *result);
		
		// Report back error to client.
		NYMPH_LOG_FATAL("Failed to run application for '" + appId + "'.");
		returnMsg->setResultValue(new NymphType(result, true));
		msg->discard();
			
		return returnMsg;
	}
	
	returnMsg->setResultValue(new NymphType(result, true));
	msg->discard();
	
	return returnMsg;
}


// --- APP LOAD RESOURCE ---
NymphMessage* app_loadResource(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Validate the application ID, try to find running instance, else launch new app instance.
	std::string appId = msg->parameters()[0]->getString();
	std::string name = msg->parameters()[1]->getString();
	
	// Find the application details.
	std::string* result = new std::string();
	
	if (appId.empty()) {
		// Use root folder.
		// First check that the name doesn't contain a '/' or '\' as this might be used to create
		// a relative path that breaks security (hierarchy travel).
		if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
			NYMPH_LOG_ERROR("File name contained illegal directory separator character.");
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		fs::path f = appsFolder + name;
		if (!fs::exists(f)) {
			NYMPH_LOG_ERROR("Failed to find requested file '" + f.string() + "'.");
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		// Read in file data.
		NYMPH_LOG_INFORMATION("Reading file: " + f.string());
		std::ifstream fstr(f.string());
		fstr.seekg(0, std::ios::end);
		size_t size = fstr.tellg();
		std::string buffer(size, ' ');
		fstr.seekg(0);
		fstr.read(&buffer[0], size);
		result->swap(buffer);
	}
	else {
		// Use App folder.
		// First check that the app really exists, as a safety feature. This should prevent
		// relative path that lead up the hierarchy.
		NymphCastApp app = nc_apps.findApp(appId);
		if (app.id.empty()) {
			NYMPH_LOG_ERROR("Failed to find a matching application for '" + appId + "'.");
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		// Next check that the name doesn't contain a '/' or '\' as this might be used to create
		// a relative path that breaks security (hierarchy travel).
		if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
			NYMPH_LOG_ERROR("File name contained illegal directory separator character.");
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		fs::path f = appsFolder + appId + "/" + name;
		if (!fs::exists(f)) {
			NYMPH_LOG_ERROR("Failed to find requested file '" + f.string() + "'.");
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		// Read in file data.
		NYMPH_LOG_INFORMATION("Reading file: " + f.string());
		std::ifstream fstr(f.string());
		fstr.seekg(0, std::ios::end);
		size_t size = fstr.tellg();
		std::string buffer(size, ' ');
		fstr.seekg(0);
		fstr.read(&buffer[0], size);
		result->swap(buffer);
	}
	
	returnMsg->setResultValue(new NymphType(result));
	msg->discard();
	
	return returnMsg;
}


// --- LOG FUNCTION ---
// Forward declaration. Real function is in the client libnymphcast library.
void logFunction(int level, std::string logStr);


int main(int argc, char** argv) {
	// Do locale initialisation here to appease Valgrind (prevent data-race reporting).
	std::ostringstream dummy;
	dummy << 0;
	
#ifdef __ANDROID__
	std::cout << "In main()..." << std::endl;
	
    // We need to redirect stdout/stderr. This requires starting a new thread here.
    start_logger(tag);

	// Set default parameters.
	// TODO: make configurable.
	// TODO: enable wallpaper storage & access.
	// TODO: enable app storage & access.
	appsFolder = "apps/";
	std::string wallpapersFolder = "wallpapers/";
	std::string resourceFolder = "";
	
	// Settings.
	is_full_screen = true;
	display_disable = false;
	screensaver_enable = false;
	audio_disable = false;
	
	// Check for 'enable_gui' boolean value. If 'true', use the GUI interface.
	gui_enable = false;
	
	// Check whether the LCDProc client should be enabled.
	lcdproc_enabled = false;
	
	// Set target LCDProc host.
	std::string lcdproc_host = "localhost";
#else
	// Parse the command line arguments.
	Sarge sarge;
	sarge.setArgument("h", "help", "Get this help message.", false);
	sarge.setArgument("a", "apps", "Custom NymphCast apps location.", true);
	sarge.setArgument("w", "wallpaper", "Custom NymphCast wallpaper location.", true);
	sarge.setArgument("c", "configuration", "Path to configuration file.", true);
	sarge.setArgument("r", "resources", "Path to GUI resource folder.", true);
	sarge.setArgument("v", "version", "Output the NymphCast version and exit.", false);
	sarge.setDescription("NymphCast receiver application. For use with NymphCast clients. More details: http://nyanko.ws/nymphcast.php.");
	sarge.setUsage("nymphcast_server <options>");
	
	sarge.parseArguments(argc, argv);
	
	if (sarge.exists("help")) {
		sarge.printHelp();
		return 0;
	}
	
	if (sarge.exists("version")) {
		std::cout << "NymphCast version: " << __VERSION << std::endl;
		return 0;
	}
	
	std::string config_file;
	if (!sarge.getFlag("configuration", config_file)) {
		std::cerr << "No configuration file provided in command line arguments." << std::endl;
		return 1;
	}
	
	// Set apps folder. Ensure the path ends with a slash.
	appsFolder = "apps/";
	if (!sarge.getFlag("apps", appsFolder)) {
		std::cout << "Setting app folder to default location." << std::endl;
	}
	
	if (appsFolder.back() != '/') {
		appsFolder.append("/");
	}
	
	// Set wallpapers folder. Ensure the path ends with a flash.
	std::string wallpapersFolder = "wallpapers/";
	if (!sarge.getFlag("wallpaper", wallpapersFolder)) {
		std::cout << "Setting wallpapers folder to default location." << std::endl;
	}

	if (wallpapersFolder.back() != '/') {
		wallpapersFolder.append("/");
	}
	
	std::string resourceFolder = "";
	if (!sarge.getFlag("resources", resourceFolder)) {
		std::cout << "Setting resource folder to default location." << std::endl;
	}
	
	// Read in the configuration.
	IniParser config;
	if (!config.load(config_file)) {
		std::cerr << "Unable to load configuration file: " << config_file << std::endl;
		return 1;
	}
	
	is_full_screen = config.getValue<bool>("fullscreen", false);
	display_disable = config.getValue<bool>("disable_video", false);
	screensaver_enable = config.getValue<bool>("enable_screensaver", false);
	
	// Check for 'enable_gui' boolean value. If 'true', use the GUI interface.
	gui_enable = config.getValue<bool>("enable_gui", false);
	
	// Check whether the LCDProc client should be enabled.
	lcdproc_enabled = config.getValue<bool>("enable_lcdproc", false);
	
	// Set target LCDProc host.
	std::string lcdproc_host = config.getValue<std::string>("lcdproc_host", "localhost");
	
	// Open the 'apps.ini' file and parse it.
	nc_apps.setAppsFolder(appsFolder);
	if (!nc_apps.readAppList(appsFolder + "apps.ini")) {
		std::cerr << "Failed to read in app list." << std::endl;
		return 1;
	}
#endif // if not Android
	
	// Initialise Poco.
	Poco::Data::SQLite::Connector::registerConnector();
	
	// Initialise the client component (RemoteServer) for use with slave remotes.
	std::cout << "Initialising server...\n";
	long timeout = 5000; // 5 seconds.
	//NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_INFO, timeout);
	
	// Initialise the server.
	//NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_INFO, timeout);
	//NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_WARNING, timeout);
	
	
	// Define all of the RPC methods we want to export for clients.
	std::cout << "Registering methods...\n";
	std::vector<NymphTypes> parameters;
	
	// Client connects to server.
	// bool connect(string client_id)
	parameters.push_back(NYMPH_STRING);
	NymphMethod connectFunction("connect", parameters, NYMPH_BOOL, connectClient);
	NymphRemoteClient::registerMethod("connect", connectFunction);
	
	// Master server calls this to turn this server instance into a slave.
	// uint8 connectMaster(sint64)
	parameters.clear();
	parameters.push_back(NYMPH_SINT64);
	NymphMethod connectMasterFunction("connectMaster", parameters, NYMPH_SINT64, connectMaster);
	NymphRemoteClient::registerMethod("connectMaster", connectMasterFunction);
	
	// Receives data chunks for playback.
	// uint8 receiveDataMaster(blob data, bool done, sint64)
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_BOOL);
	parameters.push_back(NYMPH_SINT64);
	NymphMethod receivedataMasterFunction("receiveDataMaster", parameters, NYMPH_UINT8, receiveDataMaster);
	NymphRemoteClient::registerMethod("receiveDataMaster", receivedataMasterFunction);
	
	// Receives data chunks for playback.
	// uint8 slave_start(sint64 when)
	parameters.clear();
	parameters.push_back(NYMPH_SINT64);
	NymphMethod slaveStartFunction("slave_start", parameters, NYMPH_UINT8, slave_start);
	NymphRemoteClient::registerMethod("slave_start", slaveStartFunction);
	
	// Client disconnects from server.
	// bool disconnect()
	parameters.clear();
	NymphMethod disconnectFunction("disconnect", parameters, NYMPH_BOOL, disconnect);
	NymphRemoteClient::registerMethod("disconnect", disconnectFunction);
	
	// Client starts a session.
	// Return value: OK (0), ERROR (1).
	// int session_start()
	parameters.clear();
	parameters.push_back(NYMPH_STRUCT);
	NymphMethod sessionStartFunction("session_start", parameters, NYMPH_UINT8, session_start);
	NymphRemoteClient::registerMethod("session_start", sessionStartFunction);
	
	// Client sends meta data for the track.
	// Returns: OK (0), ERROR (1).
	// int session_meta(string artist, string album, int track, string name)
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_UINT32);
	parameters.push_back(NYMPH_STRING);
	NymphMethod sessionMetaFunction("session_meta", parameters, NYMPH_UINT8, session_meta);
	NymphRemoteClient::registerMethod("session_meta", sessionMetaFunction);
	
	// Client adds slave NymphCast servers to the session.
	// Any slaves will follow the master server exactly when it comes to playback.
	// Returns: OK (0), ERROR (1).
	// int session_add_slave(array servers);
	parameters.clear();
	parameters.push_back(NYMPH_ARRAY);
	NymphMethod sessionAddSlaveFunction("session_add_slave", parameters, NYMPH_UINT8, session_add_slave);
	NymphRemoteClient::registerMethod("session_add_slave", sessionAddSlaveFunction);
	
	// Client sends a chunk of track data.
	// Returns: OK (0), ERROR (1).
	// int session_data(string buffer)
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_BOOL);
	NymphMethod sessionDataFunction("session_data", parameters, NYMPH_UINT8, session_data);
	NymphRemoteClient::registerMethod("session_data", sessionDataFunction);
	
	// Client ends the session.
	// Returns: OK (0), ERROR (1).
	// int session_end()
	parameters.clear();
	NymphMethod sessionEndFunction("session_end", parameters, NYMPH_UINT8, session_end);
	NymphRemoteClient::registerMethod("session_end", sessionEndFunction);
	
	// Reset slave data buffer.
	// Returns: OK (0), ERROR (1).
	// int slave_buffer_reset()
	parameters.clear();
	NymphMethod slaveBufferResetFunction("slave_buffer_reset", parameters, NYMPH_UINT8, slave_buffer_reset);
	NymphRemoteClient::registerMethod("slave_buffer_reset", slaveBufferResetFunction);
	
	// Playback control methods.
	//
	// VolumeSet.
	// uint8 volume_set(uint8 volume)
	// Set volume to between 0 - 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	parameters.push_back(NYMPH_UINT8);
	NymphMethod volumeSetFunction("volume_set", parameters, NYMPH_UINT8, volume_set);
	NymphRemoteClient::registerMethod("volume_set", volumeSetFunction);
	
	// VolumeUp.
	// uint8 volume_up()
	// Increase volume by 10 up to 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	NymphMethod volumeUpFunction("volume_up", parameters, NYMPH_UINT8, volume_up);
	NymphRemoteClient::registerMethod("volume_up", volumeUpFunction);
		
	// VolumeDown.
	// uint8 volume_down()
	// Decrease volume by 10 up to 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	NymphMethod volumeDownFunction("volume_down", parameters, NYMPH_UINT8, volume_down);
	NymphRemoteClient::registerMethod("volume_down", volumeDownFunction);
		
	// VolumeMute.
	// uint8 volume_mute()
	// Toggle muting audio volume.
	// Returns 0 if succeeded.
	parameters.clear();
	NymphMethod volumeMuteFunction("volume_mute", parameters, NYMPH_UINT8, volume_mute);
	NymphRemoteClient::registerMethod("volume_mute", volumeMuteFunction);
	
	// PlaybackStart.
	// uint8 playback_start()
	// Start playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackStartFunction("playback_start", parameters, NYMPH_UINT8, playback_start);
	NymphRemoteClient::registerMethod("playback_start", playbackStartFunction);
	
	// PlaybackStop.
	// uint8 playback_stop()
	// Stop playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackStopFunction("playback_stop", parameters, NYMPH_UINT8, playback_stop);
	NymphRemoteClient::registerMethod("playback_stop", playbackStopFunction);
	
	// PlaybackPause.
	// uint8 playback_pause()
	// Pause playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackPauseFunction("playback_pause", parameters, NYMPH_UINT8, playback_pause);
	NymphRemoteClient::registerMethod("playback_pause", playbackPauseFunction);
	
	// PlaybackRewind.
	// uint8 playback_rewind()
	// Rewind the current file to the beginning.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackRewindFunction("playback_rewind", parameters, NYMPH_UINT8, playback_rewind);
	NymphRemoteClient::registerMethod("playback_rewind", playbackRewindFunction);
	
	// PlaybackForward
	// uint8 playback_forward()
	// Forward the current file to the end.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackForwardFunction("playback_forward", parameters, NYMPH_UINT8, playback_forward);
	NymphRemoteClient::registerMethod("playback_forward", playbackForwardFunction);
	
	// uint8 cycle_audio()
	// Cycle audio channel.
	// Returns success or error number.
	parameters.clear();
	NymphMethod cycleAudioFunction("cycle_audio", parameters, NYMPH_UINT8, cycle_audio);
	NymphRemoteClient::registerMethod("cycle_audio", cycleAudioFunction);
	
	// uint8 cycle_video()
	// Cycle video channel.
	// Returns success or error number.
	parameters.clear();
	NymphMethod cycleVideoFunction("cycle_video", parameters, NYMPH_UINT8, cycle_video);
	NymphRemoteClient::registerMethod("cycle_video", cycleVideoFunction);
	
	// uint8 cycle_subtitle()
	// Cycle audio channel.
	// Returns success or error number.
	parameters.clear();
	NymphMethod cycleSubtitleFunction("cycle_subtitle", parameters, NYMPH_UINT8, cycle_subtitle);
	NymphRemoteClient::registerMethod("cycle_subtitle", cycleSubtitleFunction);
	
	// PlaybackSeek
	// uint8 playback_seek(uint64)
	// Seek to the indicated position.
	// Returns success or error number.
	parameters.clear();
	parameters.push_back(NYMPH_ARRAY);
	NymphMethod playbackSeekFunction("playback_seek", parameters, NYMPH_UINT8, playback_seek);
	NymphRemoteClient::registerMethod("playback_seek", playbackSeekFunction);
	
	// SubtitlesSet
	// uint8 subtitles_set()
	// Turn subtitles on or off.
	// Returns success or error number.
	parameters.clear();
	parameters.push_back(NYMPH_BOOL);
	NymphMethod subtitlesSetFunction("subtitles_set", parameters, NYMPH_UINT8, subtitles_set);
	NymphRemoteClient::registerMethod("subtitles_set", subtitlesSetFunction);
	
	// PlaybackUrl.
	// uint8 playback_url(string)
	// Try to the play the media file indicated by the provided URL.
	// Returns success or error number.
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	NymphMethod playbackUrlFunction("playback_url", parameters, NYMPH_UINT8, playback_url);
	NymphRemoteClient::registerMethod("playback_url", playbackUrlFunction);
	
	// PlaybackStatus
	// struct playback_status()
	// The current state of the NymphCast server.
	// Return struct with information:
	// ["playing"] => boolean (true/false)
	// 
	parameters.clear();
	NymphMethod playbackStatusFunction("playback_status", parameters, NYMPH_STRUCT, playback_status);
	NymphRemoteClient::registerMethod("playback_status", playbackStatusFunction);
		
	// AppList
	// string app_list()
	// Returns a list of installed applications.
	parameters.clear();
	NymphMethod appListFunction("app_list", parameters, NYMPH_STRING, app_list);
	NymphRemoteClient::registerMethod("app_list", appListFunction);	
	
	// AppSend
	// string app_send(uint32 appId, string data, uint8 format)
	// Allows a client to send data to a NymphCast application.
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_UINT8);
	NymphMethod appSendFunction("app_send", parameters, NYMPH_STRING, app_send);
	NymphRemoteClient::registerMethod("app_send", appSendFunction);	
	
	// AppLoadResource
	// string app_loadResource(string appId, string resource)
	// appID	: ID of the app, or blank for the root folder.
	// resource	: Name of the resource file.
	// Returns the contents of the requested file, either from an App folder, or (if AppId is empty)
	// from the Apps root folder.
	// Return string is empty if the requested resource could not be found.
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	NymphMethod appLoadResourceFunction("app_loadResource", parameters, NYMPH_STRING, app_loadResource);
	NymphRemoteClient::registerMethod("app_loadResource", appLoadResourceFunction);
	
	
	// Register client callbacks
	//
	// MediaReadCallback
	parameters.clear();
	parameters.push_back(NYMPH_UINT32);
	NymphMethod mediaReadCallback("MediaReadCallback", parameters, NYMPH_NULL);
	mediaReadCallback.enableCallback();
	NymphRemoteClient::registerCallback("MediaReadCallback", mediaReadCallback);
	
	// MediaStopCallback
	parameters.clear();
	//parameters.push_back(NYMPH_STRING);
	NymphMethod mediaStopCallback("MediaStopCallback", parameters, NYMPH_NULL);
	mediaStopCallback.enableCallback();
	NymphRemoteClient::registerCallback("MediaStopCallback", mediaStopCallback);
	
	// MediaSeekCallback
	// Sends the desired byte position in the open file to seek to.
	// void MediaSeekCallback(uint64)
	parameters.clear();
	parameters.push_back(NYMPH_UINT64);
	parameters.push_back(NYMPH_UINT32);
	NymphMethod mediaSeekCallback("MediaSeekCallback", parameters, NYMPH_NULL);
	mediaSeekCallback.enableCallback();
	NymphRemoteClient::registerCallback("MediaSeekCallback", mediaSeekCallback);
	
	// MediaStatusCallback
	// Sends a struct with this remote's playback status to the client.
	// void MediaStatusCallback(struct)
	parameters.clear();
	parameters.push_back(NYMPH_STRUCT);
	NymphMethod mediaStatusCallback("MediaStatusCallback", parameters, NYMPH_NULL);
	mediaStatusCallback.enableCallback();
	NymphRemoteClient::registerCallback("MediaStatusCallback", mediaStatusCallback);
	
	// ReceiveFromAppCallback
	// Sends message from a NymphCast app to the client.
	// void ReceiveFromAppCallback(string appId, string message)
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	NymphMethod receiveFromAppCallback("ReceiveFromAppCallback", parameters, NYMPH_NULL);
	receiveFromAppCallback.enableCallback();
	NymphRemoteClient::registerCallback("ReceiveFromAppCallback", receiveFromAppCallback);
	
	// End client callback registration.
	
	// Master-Slave registrations.
	using namespace std::placeholders; 
	NymphRemoteServer::registerCallback("MediaReadCallback", MediaReadCallback, 0);
	NymphRemoteServer::registerCallback("MediaSeekCallback", MediaSeekCallback, 0);
	NymphRemoteServer::registerCallback("MediaStopCallback", MediaStopCallback, 0);
	NymphRemoteServer::registerCallback("MediaStatusCallback", MediaStatusCallback, 0);
	
	// Initialise buffer of the desired size.
#ifdef __ANDROID__
	uint32_t buffer_size = 20971520; // Default 20 MB.
#else
	uint32_t buffer_size = config.getValue<uint32_t>("buffer_size", 20971520); // Default 20 MB.
#endif
	DataBuffer::init(buffer_size);
	DataBuffer::setSeekRequestCallback(seekingHandler);
	DataBuffer::setDataRequestCallback(dataRequestHandler);
	
	NYMPH_LOG_INFORMATION("Set up new buffer with size: " + 
							Poco::NumberFormatter::format(buffer_size) + " bytes.");
	
	// Set further global variables.
	// FIXME: refactor.
	if (display_disable) {
		video_disable = 1;
	}
	
	// Initialise SDL.
	if (!SdlRenderer::init()) {
		NYMPH_LOG_FATAL("Failed to init SDL. Aborting...");
		return 0;
	}
	
	// Install signal handler to terminate the server.
	// Note: SDL will install its own signal handler. It's paramount that our signal handler is
	// 			therefore installed after its, to override it.
	signal(SIGINT, signal_handler);
	
	// Start server on port 4004.
	NymphRemoteClient::start(4004);
	
	// Start NyanSD announcement server.
	NYSD_service sv;
	sv.port = 4004;
	sv.protocol = NYSD_PROTOCOL_TCP;
	sv.service = "nymphcast";
	NyanSD::addService(sv);
	
	NYMPH_LOG_INFORMATION("Starting NyanSD on port 4004 UDP...");
	NyanSD::startListener(4004);
	
//#ifndef _MSC_VER
#ifndef __ANDROID__
	if (lcdproc_enabled) {
		// Try to connect to the local LCDProc daemon if it's running.
		try {
			NYMPH_LOG_INFORMATION("Connecting to LCDProc server at " + lcdproc_host + ":13666");
			static lcdapi::LCDClient lcdclient(lcdproc_host, 13666);
			
			lcdapi_active = true;
			
			// Set screen.
			// TODO: get properties of remote screen. For now assume 16x2 HD44780.
			static lcdapi::LCDScreen screen1(&lcdclient);
			screen1.setDuration(32);
			
			static lcdapi::LCDTitle title(&screen1);
			screen1.add(&title);
			title.set("NymphCast");
			
			static lcdapi::LCDScroller scroll(&screen1);
			scroll.setWidth(20);
			scroll.setSpeed(3);
			scroll.move(1, 3);
			
			static lcdapi::LCDNymphCastSensor ncsensor("No title");
			ncsensor.addOnChangeWidget(&scroll);
			
			screen1.setBackLight(lcdapi::LCD_BACKLIGHT_ON);
		}
		catch (lcdapi::LCDException e)  {
			NYMPH_LOG_ERROR(e.what());
			NYMPH_LOG_ERROR("Skipping LCDProc client activation due to exception.");
			
			lcdapi_active = false;
		}
	}
#endif
	
	// Start idle wallpaper & clock display.
	// Transition time is 15 seconds.
	bool init_success = true;
	if (!display_disable) {
		if (gui_enable) {
			// Start the GUI with the specified resource folder.
			if (!Gui::init(resourceFolder)) {
				// Handle error.
				NYMPH_LOG_ERROR("Failed to initialize the GUI. Aborting...");
				init_success = false;
			}
			
			if (!Gui::start()) {
				NYMPH_LOG_ERROR("Failed to start the GUI. Aborting...");
				init_success = false;
			}
			
			SdlRenderer::hideWindow();
		}
		else if (screensaver_enable) {
			ScreenSaver::setDataPath(wallpapersFolder);
			SdlRenderer::showWindow();
			ScreenSaver::start(15);
		}
		else {
			// 
			SdlRenderer::hideWindow();
		}
		
		// Set full-screen mode.
		SdlRenderer::set_fullscreen(is_full_screen);
	}
	
	// Start AV thread.
	avThread.start(ffplay);
	
	// Start SDL event loop, wait for it to exit.
	if (init_success) {
		SdlRenderer::run_event_loop();
	}
	
	NYMPH_LOG_INFORMATION("Main thread: Shutting down...");
	
	// Stop screensaver if it's running.
	if (!display_disable) {
		if (gui_enable) {
			Gui::stop();
			Gui::quit();
		}
		else if (screensaver_enable) {
			ScreenSaver::stop();
		}
	}
	
	// Clean-up
	DataBuffer::cleanup();
	running = false;
 
	// Close window and clean up libSDL.
	ffplay.quit();
	avThread.join();
	SdlRenderer::quit();
	
	NYMPH_LOG_INFORMATION("Stopped SDL loop. Shutting down server threads.");
	
	NyanSD::stopListener();
	NymphRemoteClient::shutdown();
	
	// Wait before exiting, giving threads time to exit.
	Thread::sleep(2000); // 2 seconds.
	
	return 0;
}


//#ifdef __ANDROID__
//void* start_func(void* /*val*/) {
/* int main() {
	__android_log_print(ANDROID_LOG_INFO, "Entered start_func()", "n");
	std::cout << "In start_func()..." << std::endl;
	main_func();
	
	//void* retval = 0;
	int retval = 0;
	return retval;
} */


/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
/* void android_main(struct android_app* state) {
	// native_app_glue spawns a new thread, calling android_main() when the
	// activity onStart() or onRestart() methods are called.

	// Call main() here with the appropriate command line arguments defined in argv.
	//int argc = 2;
	//char* argv[] = { "NymphCastServer", "-c", "nymphcast_screensaver_config.ini" };
	
	//struct engine engine; 
	
	// Suppress link-time optimization that removes unreferenced code // to make sure glue isn't stripped. 
	// FIXME: Obsolete, can be removed. https://github.com/android-ndk/ndk/issues/381.
	//app_dummy(); 
	
	//LOGI("Starting NymphCast...");
	__android_log_print(ANDROID_LOG_INFO, "NymphCast starting...", "n");
	std::cout << "Starting NymphCast..." << std::endl;
	
	// TODO: Restore state?
	
	// Start application in its own thread.
	pthread_t main_thr;
	if (pthread_create(&main_thr, 0, start_func, 0) == -1) { 
		std::cerr << "Couldn't create thread." << std::endl;
		return; 
	}
	
	//main(argc, argv);
	
	while (true) {
		// Read all pending events
		int ident;
		int events;
		struct android_poll_source* source;
		
		// If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
		//if ((ident = ALooper_pollAll(-1, NULL, &events, (void**) &source)) >= 0) {
		while ((ident = ALooper_pollAll(-1, nullptr, &events, (void**) &source)) >= 0) {
            // Process this event.
            if (source != nullptr) {
                source->process(state, source);
			}
			
			// Check if we are exiting.
            if (state->destroyRequested != 0) {
				// TODO: Ensure application has terminated.
				//engine_term_display(&engine);
				return;
            }
		}
	}
} */
//#endif
