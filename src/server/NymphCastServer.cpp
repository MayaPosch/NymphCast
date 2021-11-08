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


#ifdef main
#undef main
#endif

struct CastClient {
	std::string name;
	int handle;
	bool sessionActive;
	uint32_t filesize;
};

// --- Globals ---
FileMetaInfo file_meta;
std::atomic<bool> playerStarted = { false };
std::atomic<bool> playerPaused = { false };
std::atomic<bool> castingUrl = { false };		// We're either casting data or streaming when playing.
std::string castUrl;
Poco::Thread avThread;

#ifdef PROFILING
FfplayDummy ffplay;
#else
Ffplay ffplay;
#endif

const uint32_t nymph_seek_event = SDL_RegisterEvents(1);
std::string appsFolder;
std::atomic<bool> running = { true };
std::condition_variable dataRequestCv;
std::mutex dataRequestMtx;
std::string loggerName = "NymphCastServer";

NCApps nc_apps;
std::map<int, CastClient> clients;
// ---


// --- DATA REQUEST FUNCTION ---
// This function can be signalled with the condition variable to request data from the client.
void dataRequestFunction() {
	// Create and share condition variable with DataBuffer class.
	DataBuffer::setDataRequestCondition(&dataRequestCv);
	
	while (running) {
		// Wait for the condition to be signalled.
		std::unique_lock<std::mutex> lk(dataRequestMtx);
		using namespace std::chrono_literals;
		dataRequestCv.wait(lk);
		
		if (!running) {
			NYMPH_LOG_INFORMATION("Shutting down data request function...");
			break;
		}
		else if (!DataBuffer::dataRequestPending) { continue; } // Spurious wake-up.
		
		NYMPH_LOG_INFORMATION("Asking for data...");
	
		// Request more data.
		std::vector<NymphType*> values;
		std::string result;
		if (!NymphRemoteClient::callCallback(DataBuffer::getSessionHandle(), "MediaReadCallback", values, result)) {
			std::cerr << "Calling callback failed: " << result << std::endl;
			return;
		}
		
		// We're now playing, so make sure the data buffer stays fed. Check every 100 ms whether
		// there's a pending request.
		/* while (1) {
			dataRequestCv.wait_for(lk, 500ms);
			if (!playerStarted) { 
				NYMPH_LOG_INFORMATION("DataRequestFunction: returning to waiting...");
				break; 
			}
			else if (!DataBuffer::dataRequestPending) { continue; }
		
			NYMPH_LOG_INFORMATION("Asking for data...");
		
			// Request more data.
			std::vector<NymphType*> values;
			std::string result;
			if (!NymphRemoteClient::callCallback(DataBuffer::getSessionHandle(), "MediaReadCallback", values, result)) {
				std::cerr << "Calling callback failed: " << result << std::endl;
				return;
			}
		} */
	}
}


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
	if (playerStarted) {
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
		
		key = new std::string("duration");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType(file_meta.duration);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("position");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType(file_meta.position);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("title");
		pair.key = new NymphType(key, true);
		std::string* val = new std::string(file_meta.getTitle());
		pair.value = new NymphType(val, true);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("artist");
		pair.key = new NymphType(key, true);
		val = new std::string(file_meta.getArtist());
		pair.value = new NymphType(val, true);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
		
		key = new std::string("volume");
		pair.key = new NymphType(key, true);
		pair.value = new NymphType(audio_volume);
		pairs->insert(std::pair<std::string, NymphPair>(*key, pair));
	}
	else {
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
		pair.value = new NymphType(audio_volume);
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
		std::cerr << "Calling media status callback failed: " << result << std::endl;
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
	
	// Delete the map.
	// TODO: persist the data somehow.
	delete pairs;
}


// --- SEEKING HANDLER ---
void seekingHandler(uint32_t session, int64_t offset) {
	if (DataBuffer::seeking()) {
		// Send message to client indicating that we're seeking in the file.
		std::vector<NymphType*> values;
		values.push_back(new NymphType((uint64_t) offset));
		std::string result;
		NymphType* resVal = 0;
		if (!NymphRemoteClient::callCallback(session, "MediaSeekCallback", values, result)) {
			std::cerr << "Calling media seek callback failed: " << result << std::endl;
			return;
		}
				
		return; 
	}
}


// --- FINISH PLAYBACK ---
// Called at the end of playback of a stream or file.
// If a stream is queued, play it, otherwise end playback.
void finishPlayback() {
	// Check whether we have any queued URLs to stream next.
	if (DataBuffer::hasStreamTrack()) {
		playerStarted = true;
		castUrl = DataBuffer::getStreamTrack();
		castingUrl = true;
		
		avThread.start(ffplay);
		
		return;
	}
	
	castingUrl = false;
	playerStarted = false;
	
	// Send message to client indicating that we're done.
	uint32_t handle = DataBuffer::getSessionHandle();
	std::vector<NymphType*> values;
	std::string result;
	if (!NymphRemoteClient::callCallback(handle, "MediaStopCallback", values, result)) {
		std::cerr << "Calling media stop callback failed: " << result << std::endl;
		return;
	}
	
	// Call the status update callback to indicate to the clients that playback stopped.
	sendGlobalStatusUpdate();
	
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
	if (playerStarted) {
		// Add to queue.
		DataBuffer::addStreamTrack(url);
		
		return true;
	}
	
	castUrl = url;
	castingUrl = true;
	
	if (!playerStarted) {
		playerStarted = true;
		avThread.start(ffplay);
	}
	
	// Send status update to client.
	sendStatusUpdate(DataBuffer::getSessionHandle());
	
	return true;
}


void signal_handler(int signal) {
	NYMPH_LOG_INFORMATION("SIGINT handler called. Shutting down...");
	SdlRenderer::stop_event_loop();
}


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


enum NcsMode {
	NCS_MODE_STANDALONE = 0,
	NCS_MODE_MASTER,
	NCS_MODE_SLAVE
};

NcsMode serverMode = NCS_MODE_STANDALONE;
std::vector<NymphCastSlaveRemote> slave_remotes;
uint32_t slaveLatencyMax = 0;	// Max latency to slave remote in milliseconds.


// Callback for the connect function.
NymphMessage* connectClient(int session, NymphMessage* msg, void* data) {
	std::cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << "\n";
	
	std::string clientStr = msg->parameters()[0]->getString();
	std::cout << "Client string: " << clientStr << "\n";
	
	// TODO: check whether we're not operating in slave or master mode already.
	std::cout << "Switching to stand-alone server mode." << std::endl;
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
		std::cerr << "Calling media status callback failed: " << result << std::endl;
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
	std::cout << "Received master connect request, slave mode initiation requested." << std::endl;
	
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Switch to slave mode, if possible.
	// Return error if we're currently playing content in stand-alone mode.
	if (playerStarted) {
		returnMsg->setResultValue(new NymphType((int64_t) 0));
	}
	else {
		// FIXME: for now we just return the current time.
		std::cout << "Switching to slave server mode." << std::endl;
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
// Receives data chunks for playback.
// uint8 receiveDataMaster(blob data)
NymphMessage* receiveDataMaster(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Extract data blob and add it to the buffer.
	NymphType* mediaData = msg->parameters()[0];
	bool done = msg->parameters()[1]->getBool();
	int64_t when = msg->parameters()[2]->getInt64();
	
	// Write string into buffer.
	DataBuffer::write(mediaData->getChar(), mediaData->string_length());
	
	if (!playerStarted) {
		// Start the player when the delay in 'when' has been reached.
		std::condition_variable cv;
		std::mutex cv_m;
		std::unique_lock<std::mutex> lk(cv_m);
		//std::chrono::system_clock::time_point then = std::chrono::system_clock::from_time_t(when);
		std::chrono::microseconds dur(when);
		std::chrono::time_point<std::chrono::system_clock> then(dur);
		//while (cv.wait_until(lk, then) != std::cv_status::timeout) { }
		while (cv.wait_for(lk, dur) != std::cv_status::timeout) { }
		
		// Start player.
		playerStarted = true;
		avThread.start(ffplay);
	}
	
	if (done) {
		DataBuffer::setEof(done);
	}
	
	msg->discard();
	
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
	
	std::cout << "Current server mode: " << serverMode << std::endl;
	
	// Disconnect any slave remotes if we're connected.
	if (serverMode == NCS_MODE_MASTER) {
		std::cout << "# of slave remotes: " << slave_remotes.size() << std::endl;
		for (int i = 0; i < slave_remotes.size(); ++i) {
			// Disconnect from slave remote.
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::cout << "Disconnecting slave: " << rm.name << std::endl;
			std::string result;
			if (!NymphRemoteServer::disconnect(rm.handle, result)) {
				// Failed to connect, error out. Disconnect from any already connected slaves.
				std::cerr << "Slave disconnect error: " << result << std::endl;
			}
		}
		
		slave_remotes.clear();
	}
	
	std::cout << "Switching to stand-alone server mode." << std::endl;
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
		std::cerr << "Didn't find entry 'filesize'. Aborting..." << std::endl;
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		msg->discard();
		
		return returnMsg;
	}
	
	it->second.filesize = num->getUint32();
	
	std::cout << "Starting new session for file with size: " << it->second.filesize << std::endl;
	
	DataBuffer::setFileSize(it->second.filesize);
	DataBuffer::setSessionHandle(session);
	
	// Start calling the client's read callback method to obtain data. Once the data buffer
	// has been filled sufficiently, start the playback.
	if (!DataBuffer::start()) {
		std::cerr << "Failed to start buffering. Abort." << std::endl;
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
			std::cerr << "Slave connection error: " << result << std::endl;
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
			std::cerr << "Slave connect master failed: " << result << std::endl;
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
			std::cerr << "Configuring remote as slave failed." << std::endl;
			// TODO: disconnect from slave remotes.
			returnMsg->setResultValue(new NymphType((uint8_t) 1));
			msg->discard();
			
			return returnMsg;
		}
		
		// Use returned time stamp to calculate the delay.
		// FIXME: using stopwatch-style local time to determine latency for now.
		
		//rm.delay = theirs - now;
		rm.delay = pong - now;
		std::cout << "Slave delay: " << rm.delay << " microseconds." << std::endl;
		std::cout << "Current max slave delay: " << slaveLatencyMax << std::endl;
		if (rm.delay > slaveLatencyMax) { 
			slaveLatencyMax = rm.delay;
			std::cout << "Max slave latency increased to: " << slaveLatencyMax << " microseconds." 
						<< std::endl;
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
	//std::string mediaData = ((NymphBlob*) msg->parameters()[0])->getValue();
	NymphType* mediaData = msg->parameters()[0];
	bool done = msg->parameters()[1]->getBool();
	
	// Write string into buffer.
	DataBuffer::write(mediaData->getChar(), mediaData->string_length());
	
	// If passing the message through to slave remotes, add the timestamp to the message.
	// This timestamp is the current time plus the largest master-slave latency times 2.
	int64_t then = 0;
	if (serverMode == NCS_MODE_MASTER) {
		Poco::Timestamp ts;
		int64_t now = (int64_t) ts.epochMicroseconds();
		//then = now + (slaveLatencyMax * 2);
		
		// Timing: 	Multiply the max slave latency by the number of slaves. After sending this delay
		// 			to the first slave (minus half its recorded latency), 
		// 			subtract the time it took to send to this slave from the
		//			first delay, then send this new delay to the second slave, and so on.
		int64_t countdown = slaveLatencyMax * slave_remotes.size();
		
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			//then = slaveLatencyMax - rm.delay;
			then = countdown - (rm.delay / 2);
			
			int64_t send = (int64_t) ts.epochMicroseconds();
		
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
			
			int64_t receive = (int64_t) ts.epochMicroseconds();
			
			countdown -= (receive - send);
		}
		
		// Wait out the countdown.
		std::condition_variable cv;
		std::mutex cv_m;
		std::unique_lock<std::mutex> lk(cv_m);
		std::chrono::microseconds dur(countdown);
		while (cv.wait_for(lk, dur) != std::cv_status::timeout) { }
	}
	
	// Start the player if it hasn't yet. This ensures we have a buffer ready.
	// TODO: take into account delay of slave remotes before starting local playback.
	if (!playerStarted) {
		// if we're in master mode, only start after the slaves are starting as well.
		// In slave mode, we execute time-critical commands like playback start when
		/* if (serverMode == NCS_MODE_MASTER) {
			// We use the calculated latency to the slave to determine when to send the play
			// command to the slave.
			// TODO:
		} */
		
		if (serverMode == NCS_MODE_MASTER) {
			// Start the player when the delay in 'then' has been reached.
			std::condition_variable cv;
			std::mutex cv_m;
			std::unique_lock<std::mutex> lk(cv_m);
			std::chrono::microseconds dur(slaveLatencyMax);
			//std::chrono::time_point<std::chrono::system_clock> when(dur);
			while (cv.wait_for(lk, dur) != std::cv_status::timeout) { }
		}
		
		// Start playback locally.
		playerStarted = true;
		avThread.start(ffplay);
		
		// Signal the clients that we're playing now.
		sendGlobalStatusUpdate();
	}
	else {
		// Send status update to clients.
		sendGlobalStatusUpdate();
	}
	
	// if 'done' is true, the client has sent the last bytes. Signal session end in this case.
	if (done) {
		DataBuffer::setEof(done);
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


// --- VOLUME SET ---
// uint8 volume_set(uint8 volume)
NymphMessage* volume_set(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	uint8_t volume = msg->parameters()[0]->getUint8();
	
	std::vector<NymphType*> values;
	values.push_back(new NymphType(volume));
	if (serverMode == NCS_MODE_MASTER) {
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
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_m;
	SDL_PushEvent(&event);
	
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
	
	playerPaused = ~playerPaused;
	
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


enum {
	NYMPH_SEEK_TYPE_BYTES = 1,
	NYMPH_SEEK_TYPE_PERCENTAGE = 2
};


// --- PLAYBACK SEEK ---
// uint8 playback_seek(uint64)
NymphMessage* playback_seek(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	uint8_t type = msg->parameters()[0]->getUint8();
	if (type == NYMPH_SEEK_TYPE_PERCENTAGE) {
		uint8_t percentage = msg->parameters()[1]->getUint8();
		
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
		// TODO: implement.
	}
	else {
		returnMsg->setResultValue(new NymphType((uint8_t) 1));
		return returnMsg;
	}
	
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
// string app_send(string appId, string data)
NymphMessage* app_send(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Validate the application ID, try to find running instance, else launch new app instance.
	std::string appId = msg->parameters()[0]->getString();
	std::string message = msg->parameters()[1]->getString();
	
	// Get the desired output format.
	// 0 - CLI	- Text with tab (\t) separators and \n terminator.
	// 1 - HTML	- HTML format.
	//uint8_t format = ((NymphUint8*) msg->parameters()[1])->getValue();
	
	// Find the application details.
	std::string* result = new std::string();
	NymphCastApp app = nc_apps.findApp(appId);
	if (app.id.empty()) {
		std::cerr << "Failed to find a matching application for '" << appId << "'." << std::endl;
		returnMsg->setResultValue(new NymphType(result, true));
		msg->discard();
			
		return returnMsg;
	}
	
	std::cout << "Found " << appId << " app." << std::endl;
	
	if (!nc_apps.runApp(appId, message, *result)) {
		std::cerr << "Error running app: " << result << std::endl;
		
		// TODO: report back error to client.
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
			std::cerr << "File name contained illegal directory separator character." << std::endl;
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		fs::path f = appsFolder + name;
		if (!fs::exists(f)) {
			std::cerr << "Failed to find requested file '" << f.string() << "'." << std::endl;
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		// Read in file data.
		std::cout << "Reading file: " << f.string() << std::endl;
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
			std::cerr << "Failed to find a matching application for '" << appId << "'." << std::endl;
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		// Next check that the name doesn't contain a '/' or '\' as this might be used to create
		// a relative path that breaks security (hierarchy travel).
		if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
			std::cerr << "File name contained illegal directory separator character." << std::endl;
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		fs::path f = appsFolder + appId + "/" + name;
		if (!fs::exists(f)) {
			std::cerr << "Failed to find requested file '" << f.string() << "'." << std::endl;
			returnMsg->setResultValue(new NymphType(result, true));
			msg->discard();
			
			return returnMsg;
		}
		
		// Read in file data.
		std::cout << "Reading file: " << f.string() << std::endl;
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
void logFunction(int level, std::string logStr);/* {
	std::cout << level << " - " << logStr << std::endl;
}*/


int main(int argc, char** argv) {
	// Do locale initialisation here to appease Valgrind (prevent data-race reporting).
	std::ostringstream dummy;
	dummy << 0;
	
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
	
	// Check for 'gui_enable' boolean value. If on, use the RmlUi-based GUI instead of the
	// screensaver mode.
	gui_enable = config.getValue<bool>("gui_enable", false);
	
	// Open the 'apps.ini' file and parse it.
	nc_apps.setAppsFolder(appsFolder);
	if (!nc_apps.readAppList(appsFolder + "apps.ini")) {
		std::cerr << "Failed to read in app list." << std::endl;
		return 1;
	}
	
	// Initialise Poco.
	Poco::Data::SQLite::Connector::registerConnector();
	
	// Initialise the client component (RemoteServer) for use with slave remotes.
	std::cout << "Initialising server...\n";
	long timeout = 5000; // 5 seconds.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_INFO, timeout);
	
	// Initialise the server.
	//NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	//NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_INFO, timeout);
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_WARNING, timeout);
	
	
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
	// string app_send(uint32 appId, string data)
	// Allows a client to send data to a NymphCast application.
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
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
	//parameters.push_back(NYMPH_STRING);
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
	uint32_t buffer_size = config.getValue<uint32_t>("buffer_size", 20971520); // Default 20 MB.
	DataBuffer::init(buffer_size);
	DataBuffer::setSeekRequestCallback(seekingHandler);
	
	std::cout << "Set up new buffer with size: " << buffer_size << " bytes." << std::endl;
	
	playerStarted = false;
	
	// Set further global variables.
	// FIXME: refactor.
	if (display_disable) {
		video_disable = 1;
	}
	
	// Initialise SDL.
	if (!SdlRenderer::init()) {
		std::cerr << "Failed to init SDL. Aborting..." << std::endl;
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
	
	std::cout << "Starting NyanSD on port 4004 UDP..." << std::endl;
	NyanSD::startListener(4004);
	
	// Start the data request handler in its own thread.
	std::thread drq(dataRequestFunction);
	
	// Start idle wallpaper & clock display.
	// Transition time is 15 seconds.
	bool init_success = true;
	if (!display_disable) {
		if (gui_enable) {
			// Start the GUI with the specified resource folder.
			if (!Gui::init(resourceFolder)) {
				// Handle error.
				std::cerr << "Failed to initialize the GUI. Aborting..." << std::endl;
				init_success = false;
			}
			
			if (!Gui::start()) {
				std::cerr << "Failed to start the GUI. Aborting..." << std::endl;
				init_success = false;
			}
			
			SdlRenderer::hideWindow();
			//SdlRenderer::showWindow();
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
	
	// Start SDL event loop, wait for it to exit.
	if (init_success) {
		SdlRenderer::run_event_loop();
	}
	
	std::cout << "Main thread: Shutting down..." << std::endl;
	
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
	dataRequestCv.notify_one();
	drq.join();
 
	// Close window and clean up libSDL.
	ffplay.quit();
	avThread.join();
	SdlRenderer::quit();
	
	std::cout << "Stopping SDL loop..." << std::endl;
	
	NyanSD::stopListener();
	NymphRemoteClient::shutdown();
	
	// Wait before exiting, giving threads time to exit.
	Thread::sleep(2000); // 2 seconds.
	
	return 0;
}
