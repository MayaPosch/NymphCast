/*
	NymphCastServer.cpp - Server that accepts NymphCast client sessions to play back audio.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
		
	2019/01/25, Maya Posch
*/


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
#include <condition_variable>

namespace fs = std::filesystem;

#include "ffplay/ffplay.h"
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

using namespace Poco;

#include "INIReader.h"

#include "nyansd.h"

#include "nc_apps.h"
#include "gui.h"


// Global objects.
Condition gCon;
Mutex gMutex;
// ---


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
bool gui_enable;
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
std::atomic<uint32_t> audio_volume = 100;


#ifdef main
#undef main
#endif


// --- Globals ---
FileMetaInfo file_meta;
std::atomic<bool> playerStarted = false;
std::atomic<bool> castingUrl = false;		// We're either casting data or streaming when playing.
std::string castUrl;
Poco::Thread avThread;
Ffplay ffplay;
const uint32_t nymph_seek_event = SDL_RegisterEvents(1);
std::string appsFolder;
std::atomic<bool> running = true;
std::condition_variable dataRequestCv;
std::mutex dataRequestMtx;

NCApps nc_apps;
// ---


// --- DATA REQUEST FUNCTION ---
// This function can be signalled with the condition variable to request data from the client.
void dataRequestFunction() {
	// Create and share condition variable with DataBuffer class.
	DataBuffer::setDataRequestCondition(&dataRequestCv);
	
	while (running) {
		// Wait for the condition to be signalled.
		std::unique_lock<std::mutex> lk(dataRequestMtx);
		dataRequestCv.wait(lk);
		
		if (!running) { 
			std::cout << "Shutting down data request function..." << std::endl;
			break;
		}
		
		// Request more data.
		// TODO: Initial buffer size is 2 MB. Make this dynamically scale.
		std::vector<NymphType*> values;
		std::string result;
		NymphBoolean* resVal = 0;
		if (!NymphRemoteClient::callCallback(DataBuffer::getSessionHandle(), "MediaReadCallback", values, result)) {
			std::cerr << "Calling callback failed: " << result << std::endl;
			return;
		}
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
NymphStruct* getPlaybackStatus() {
	// Set the playback status.
	// We're sending back whether we are playing something currently. If so, also includes:
	// * duration of media in seconds.
	// * position in the media, in seconds with remainder.
	// * title of the media, if available.
	// * artist of the media, if available.
	NymphStruct* response = new NymphStruct;
	if (playerStarted) {
		// TODO: distinguish between playing and paused for the player.
		response->addPair("status", new NymphUint32(NYMPH_PLAYBACK_STATUS_PLAYING));
		response->addPair("playing", new NymphBoolean(true));
		response->addPair("duration", new NymphUint64(file_meta.duration));
		response->addPair("position", new NymphDouble(file_meta.position));
		response->addPair("title", new NymphString());
		response->addPair("artist", new NymphString());
		response->addPair("volume", new NymphUint8(ffplay.getVolume()));
	}
	else {
		response->addPair("status", new NymphUint32(NYMPH_PLAYBACK_STATUS_STOPPED));
		response->addPair("playing", new NymphBoolean(false));
		response->addPair("duration", new NymphUint64(0));
		response->addPair("position", new NymphDouble(0));
		response->addPair("title", new NymphString());
		response->addPair("artist", new NymphString());
		response->addPair("volume", new NymphUint8(0));
	}
	
	return response;
}


// --- SEEKING HANDLER ---
void seekingHandler(uint32_t session, int64_t offset) {
	if (DataBuffer::seeking()) {
		// Send message to client indicating that we're seeking in the file.
		std::vector<NymphType*> values;
		values.push_back(new NymphUint64(offset));
		std::string result;
		NymphBoolean* resVal = 0;
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
	NymphBoolean* resVal = 0;
	if (!NymphRemoteClient::callCallback(handle, "MediaStopCallback", values, result)) {
		std::cerr << "Calling media stop callback failed: " << result << std::endl;
		return;
	}
	
	// Call the status update callback to indicate to the client that playback stopped.
	values.clear();
	values.push_back(getPlaybackStatus());
	resVal = 0;
	if (!NymphRemoteClient::callCallback(handle, "MediaStatusCallback", values, result)) {
		std::cerr << "Calling media status callback failed: " << result << std::endl;
		return;
	}
	
	// Start the Screensaver here for now.
	if (!display_disable) {
		if (gui_enable) {
			// Return to GUI.
			Gui::active = true;
			Gui::resumeCv.notify_one();
		}
		else {
			// Start screensaver.
			ScreenSaver::start(15);
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
	
	return true;
}


void signal_handler(int signal) {
	gCon.signal();
}


// Data structure.
struct SessionParams {
	int max_buffer;
};


struct CastClient {
	std::string name;
	bool sessionActive;
	uint32_t filesize;
};


std::map<int, CastClient> clients;


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
	
	std::string clientStr = ((NymphString*) msg->parameters()[0])->getValue();
	std::cout << "Client string: " << clientStr << "\n";
	
	// TODO: check whether we're not operating in slave or master mode already.
	std::cout << "Switching to stand-alone server mode." << std::endl;
	serverMode = NCS_MODE_STANDALONE;
	
	// Register this client with its ID. Return error if the client ID already exists.
	NymphMessage* returnMsg = msg->getReplyMessage();
	std::map<int, CastClient>::iterator it;
	it = clients.find(session);
	NymphBoolean* retVal = 0;
	if (it == clients.end()) {
		// Client ID already exists, abort.
		retVal = new NymphBoolean(false);
	}
	else {
		CastClient c;
		c.name = clientStr;
		c.sessionActive = false;
		c.filesize = 0;
		clients.insert(std::pair<int, CastClient>(session, c));
		retVal = new NymphBoolean(true);
	}
	
	// Send the client the current playback status.
	std::vector<NymphType*> values;
	values.push_back(getPlaybackStatus());
	std::string result;
	NymphBoolean* resVal = 0;
	if (!NymphRemoteClient::callCallback(session, "MediaStatusCallback", values, result)) {
		std::cerr << "Calling media status callback failed: " << result << std::endl;
	}
	
	returnMsg->setResultValue(retVal);
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
		returnMsg->setResultValue(new NymphSint64(0));
	}
	else {
		// FIXME: for now we just return the current time.
		std::cout << "Switching to slave server mode." << std::endl;
		serverMode = NCS_MODE_SLAVE;
		//DataBuffer::setFileSize(it->second.filesize);
		DataBuffer::setSessionHandle(session);
		
		Poco::Timestamp ts;
		int64_t now = (int64_t) ts.epochMicroseconds();
		returnMsg->setResultValue(new NymphSint64(now));
	}
	
	// TODO: Obtain timestamp, compare with current time.
	//time_t then = ((NymphSint64*) msg->parameters()[0])->getValue();
	
	// TODO: Send delay request to master.
	
	// TODO: Determine final latency and share with master.
	
	return returnMsg;
}


// --- RECEIVE DATA MASTER ---
// Receives data chunks for playback.
// uint8 receiveDataMaster(blob data)
NymphMessage* receiveDataMaster(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Extract data blob and add it to the buffer.
	std::string mediaData = ((NymphBlob*) msg->parameters()[0])->getValue();
	bool done = ((NymphBoolean*) msg->parameters()[1])->getValue();
	int64_t when = ((NymphSint64*) msg->parameters()[2])->getValue();
	
	// Write string into buffer.
	DataBuffer::write(mediaData);
	
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
	returnMsg->setResultValue(new NymphBoolean(true));
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
	if (it != clients.end()) {
		returnMsg->setResultValue(new NymphUint8(1));
		return returnMsg;
	}
	
	// Obtain the filesize from the client, which we use with the buffer management.
	NymphStruct* fileInfo = ((NymphStruct*) msg->parameters()[0]);
	NymphType* num = 0;
	if (!fileInfo->getValue("filesize", num)) {
		returnMsg->setResultValue(new NymphUint8(1));
		return returnMsg;
	}
	
	it->second.filesize = ((NymphUint32*) num)->getValue();
	
	std::cout << "Starting new session for file with size: " << it->second.filesize << std::endl;
	
	DataBuffer::setFileSize(it->second.filesize);
	DataBuffer::setSessionHandle(session);
	
	// Start calling the client's read callback method to obtain data. Once the data buffer
	// has been filled sufficiently, start the playback.
	// TODO: Initial buffer size is 1 MB. Make this dynamically scale.
	DataBuffer::start();
	it->second.sessionActive = true;
	
	// Stop screensaver.
	if (!video_disable) {
		if (gui_enable) {
			// TODO: stop the GUI.
			
		}
		else {
			ScreenSaver::stop();
		}
	}
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// Client sends meta data for the track.
// Returns: OK (0), ERROR (1).
// int session_meta(string artist, string album, int track, string name)
NymphMessage* session_meta(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// X unused function.
	
	return returnMsg;
}


// --- SESSION ADD SLAVE ---
// Client sends list of slave server which this server instance should control.
// Returns: OK (0), ERROR (1).
// int session_add_slave(array servers);
NymphMessage* session_add_slave(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Extract the array.
	std::vector<NymphType*> remotes = ((NymphArray*) msg->parameters()[0])->getValues();
	for (int i = 0; i < remotes.size(); ++i) {
		NymphStruct* ns = (NymphStruct*) remotes[i];
		NymphCastSlaveRemote remote;
		NymphType* value = 0;
		((NymphStruct*) remotes[i])->getValue("name", value);
		remote.name = ((NymphString*) value)->getValue();
		((NymphStruct*) remotes[i])->getValue("ipv4", value);
		remote.ipv4 = ((NymphString*) value)->getValue();
		((NymphStruct*) remotes[i])->getValue("ipv6", value);
		remote.ipv6 = ((NymphString*) value)->getValue();
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
			
			returnMsg->setResultValue(new NymphUint8(1));
			return returnMsg;
		}
		
		// Attempt to start slave mode on the remote.
		// Send the current timestamp to the slave remote as part of the latency determination.
		Poco::Timestamp ts;
		int64_t now = (int64_t) ts.epochMicroseconds();
		std::vector<NymphType*> values;
		values.push_back(new NymphSint64(now));
		NymphType* returnValue = 0;
		if (!NymphRemoteServer::callMethod(rm.handle, "connectMaster", values, returnValue, result)) {
			std::cerr << "Slave connect master failed: " << result << std::endl;
			// TODO: disconnect from slave remotes.
			returnMsg->setResultValue(new NymphUint8(1));
			return returnMsg;
		}
		
		// Get new time. This should be roughly twice the latency to the slave remote.
		ts.update();
		int64_t pong = ts.epochMicroseconds();
		
		// Check return value.
		if (returnValue->type() != NYMPH_SINT64) {
			std::cout << "Return value wasn't a sint64. Type: " << returnValue->type() << std::endl;
			// TODO: disconnect from slave remotes.
			returnMsg->setResultValue(new NymphUint8(1));
			return returnMsg;
		}
		
		if (((NymphSint64*) returnValue)->getValue() == 0) {
			std::cerr << "Configuring remote as slave failed." << std::endl;
			// TODO: disconnect from slave remotes.
			returnMsg->setResultValue(new NymphUint8(1));
			return returnMsg;
		}
		
		// Use returned time stamp to calculate the delay.
		// FIXME: using stopwatch-style local time to determine latency for now.
		time_t theirs = ((NymphSint64*) returnValue)->getValue();
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
	
	std::cout << "Switching to master server mode." << std::endl;
	serverMode = NCS_MODE_MASTER;
	
	returnMsg->setResultValue(new NymphUint8(0));
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
	if (it != clients.end()) {
		returnMsg->setResultValue(new NymphUint8(1));
		return returnMsg;
	}
	
	// Safely write the data for this session to the buffer.
	std::string mediaData = ((NymphBlob*) msg->parameters()[0])->getValue();
	bool done = ((NymphBoolean*) msg->parameters()[1])->getValue();
	
	// Write string into buffer.
	DataBuffer::write(mediaData);
	
	// If passing the message through to slave remotes, add the timestamp to the message.
	// This timestamp is the current time plus the largest master-slave latency times 2.
	int64_t then = 0;
	if (serverMode == NCS_MODE_MASTER) {
		Poco::Timestamp ts;
		int64_t now = (int64_t) ts.epochMicroseconds();
		//then = now + (slaveLatencyMax * 2);
		
		NymphBlob* mediaBlob = new NymphBlob(mediaData);
		NymphBoolean* doneBool = new NymphBoolean(done);
		
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			then = slaveLatencyMax - rm.delay;
		
			// Prepare data vector.
			std::vector<NymphType*> values;
			values.push_back(mediaBlob);
			values.push_back(doneBool);
			values.push_back(new NymphSint64(then));
			
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "receiveDataMaster", values, returnValue, result)) {
				// TODO:
			}
		}
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
		
		// Signal the client that we're playing now.
		// TODO: is it okay to call this right after starting the player thread?
		std::vector<NymphType*> values;
		values.push_back(getPlaybackStatus());
		std::string result;
		NymphBoolean* resVal = 0;
		if (!NymphRemoteClient::callCallback(DataBuffer::getSessionHandle(), "MediaStatusCallback", values, result)) {
			std::cerr << "Calling media status callback failed: " << result << std::endl;
		}
	}
	
	// if 'done' is true, the client has sent the last bytes. Signal session end in this case.
	if (done) {
		DataBuffer::setEof(done);
	}
	
	returnMsg->setResultValue(new NymphUint8(0));
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
		returnMsg->setResultValue(new NymphUint8(1));
		return returnMsg;
	}
	
	it->second.sessionActive = false;
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- VOLUME SET ---
// uint8 volume_set(uint8 volume)
NymphMessage* volume_set(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	uint8_t volume = ((NymphUint8*) msg->parameters()[0])->getValue();
	
	std::vector<NymphType*> values;
	values.push_back(new NymphUint8(volume));
	if (serverMode == NCS_MODE_MASTER) {
		for (int i = 0; i < slave_remotes.size(); ++i) {
			NymphCastSlaveRemote& rm = slave_remotes[i];
			std::string result;
			NymphType* returnValue = 0;
			if (!NymphRemoteServer::callMethod(rm.handle, "volume_set", values, returnValue, result)) {
				// TODO:
			}
		}
	}
	
	audio_volume = volume;
	ffplay.setVolume(volume);
	
	returnMsg->setResultValue(new NymphUint8(0));
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
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_0;
	SDL_PushEvent(&event);
	
	// TODO: update global audio_volume variable.
	
	returnMsg->setResultValue(new NymphUint8(0));
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
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_9;
	SDL_PushEvent(&event);
	
	// TODO: update global audio_volume variable.
	
	returnMsg->setResultValue(new NymphUint8(0));
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
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_m;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
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
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_SPACE;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
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
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_ESCAPE;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
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
		}
	}
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_p;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK REWIND ---
// uint8 playback_rewind()
// TODO:
NymphMessage* playback_rewind(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK FORWARD ---
// uint8 playback_forward()
// TODO:
NymphMessage* playback_forward(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	returnMsg->setResultValue(new NymphUint8(0));
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
	
	uint8_t type = ((NymphUint8*) msg->parameters()[0])->getValue();
	if (type == NYMPH_SEEK_TYPE_PERCENTAGE) {
		uint8_t percentage = ((NymphUint8*) msg->parameters()[1])->getValue();
		
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
		returnMsg->setResultValue(new NymphUint8(1));
		return returnMsg;
	}
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK URL ---
// uint8 playback_url(string)
NymphMessage* playback_url(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	std::string url = ((NymphString*) msg->parameters()[0])->getValue();
	bool ret = streamTrack(url);
	
	NymphUint8* retval = new NymphUint8(1);
	if (ret) {
		std::vector<NymphType*> values;
		values.push_back(new NymphString(url));
		if (serverMode == NCS_MODE_MASTER) {
			for (int i = 0; i < slave_remotes.size(); ++i) {
				NymphCastSlaveRemote& rm = slave_remotes[i];
				std::string result;
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(rm.handle, "playback_url", values, returnValue, result)) {
					// TODO:
				}
			}
		}
		
		retval->setValue(0);
	}
	
	returnMsg->setResultValue(retval);
	return returnMsg;
}
	


// --- PLAYBACK STATUS ---
// struct playback_status()
NymphMessage* playback_status(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
		
	returnMsg->setResultValue(getPlaybackStatus());
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
	std::string names;
	std::vector<std::string>::const_iterator it = appNames.cbegin();
	while (it != appNames.cend()) {
		names.append(*it);
		names.append("\n");
		it++;
	}
	
	returnMsg->setResultValue(new NymphString(names));
	return returnMsg;
}


// --- APP SEND ---
// string app_send(string appId, string data)
NymphMessage* app_send(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Validate the application ID, try to find running instance, else launch new app instance.
	std::string appId = ((NymphString*) msg->parameters()[0])->getValue();
	std::string message = ((NymphString*) msg->parameters()[1])->getValue();
	
	// Get the desired output format.
	// 0 - CLI	- Text with tab (\t) separators and \n terminator.
	// 1 - HTML	- HTML format.
	//uint8_t format = ((NymphUint8*) msg->parameters()[1])->getValue();
	
	// Find the application details.
	std::string result = "";
	NymphCastApp app = nc_apps.findApp(appId);
	if (app.id.empty()) {
		std::cerr << "Failed to find a matching application for '" << appId << "'." << std::endl;
		returnMsg->setResultValue(new NymphString(result));
		return returnMsg;
	}
	
	std::cout << "Found " << appId << " app." << std::endl;
	
	if (!nc_apps.runApp(appId, message, result)) {
		std::cerr << "Error running app: " << result << std::endl;
		
		// TODO: report back error to client.
	}
	
	returnMsg->setResultValue(new NymphString(result));
	return returnMsg;
}


// --- APP LOAD RESOURCE ---
NymphMessage* app_loadResource(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Validate the application ID, try to find running instance, else launch new app instance.
	std::string appId = ((NymphString*) msg->parameters()[0])->getValue();
	std::string name = ((NymphString*) msg->parameters()[1])->getValue();
	
	// Find the application details.
	std::string result;
	
	if (appId.empty()) {
		// Use root folder.
		// First check that the name doesn't contain a '/' or '\' as this might be used to create
		// a relative path that breaks security (hierarchy travel).
		if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
			std::cerr << "File name contained illegal directory separator character." << std::endl;
			returnMsg->setResultValue(new NymphBlob(result));
			return returnMsg;
		}
		
		fs::path f = appsFolder + name;
		if (!fs::exists(f)) {
			std::cerr << "Failed to find requested file '" << f.string() << "'." << std::endl;
			returnMsg->setResultValue(new NymphBlob(result));
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
		result.swap(buffer);
	}
	else {
		// Use App folder.
		// First check that the app really exists, as a safety feature. This should prevent
		// relative path that lead up the hierarchy.
		NymphCastApp app = nc_apps.findApp(appId);
		if (app.id.empty()) {
			std::cerr << "Failed to find a matching application for '" << appId << "'." << std::endl;
			returnMsg->setResultValue(new NymphBlob(result));
			return returnMsg;
		}
		
		// Next check that the name doesn't contain a '/' or '\' as this might be used to create
		// a relative path that breaks security (hierarchy travel).
		if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
			std::cerr << "File name contained illegal directory separator character." << std::endl;
			returnMsg->setResultValue(new NymphBlob(result));
			return returnMsg;
		}
		
		fs::path f = appsFolder + appId + "/" + name;
		if (!fs::exists(f)) {
			std::cerr << "Failed to find requested file '" << f.string() << "'." << std::endl;
			returnMsg->setResultValue(new NymphBlob(result));
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
		result.swap(buffer);
	}
	
	returnMsg->setResultValue(new NymphBlob(result));
	return returnMsg;
}


// --- LOG FUNCTION ---
void logFunction(int level, std::string logStr);/* {
	std::cout << level << " - " << logStr << std::endl;
}*/


int main(int argc, char** argv) {
	// Parse the command line arguments.
	Sarge sarge;
	sarge.setArgument("h", "help", "Get this help message.", false);
	sarge.setArgument("a", "apps", "Custom NymphCast apps location.", true);
	sarge.setArgument("w", "wallpaper", "Custom NymphCast wallpaper location.", true);
	sarge.setArgument("c", "configuration", "Path to configuration file.", true);
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
	
	// Read in the configuration.
	IniParser config;
	if (!config.load(config_file)) {
		std::cerr << "Unable to load configuration file: " << config_file << std::endl;
		return 1;
	}
	
	is_full_screen = config.getValue<bool>("fullscreen", false);
	display_disable = config.getValue<bool>("disable_video", false);
	
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
	
	// Initialise the server.
	std::cout << "Initialising server...\n";
	long timeout = 5000; // 5 seconds.
	//NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_INFO, timeout);
	
	// Initialise the client component (RemoteServer) for use with slave remotes.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_INFO, timeout);
	
	
	// Define all of the RPC methods we want to export for clients.
	std::cout << "Registering methods...\n";
	std::vector<NymphTypes> parameters;
	
	// Client connects to server.
	// bool connect(string client_id)
	parameters.push_back(NYMPH_STRING);
	NymphMethod connectFunction("connect", parameters, NYMPH_BOOL);
	connectFunction.setCallback(connectClient);
	NymphRemoteClient::registerMethod("connect", connectFunction);
	
	// Master server calls this to turn this server instance into a slave.
	// uint8 connectMaster(sint64)
	parameters.clear();
	parameters.push_back(NYMPH_SINT64);
	NymphMethod connectMasterFunction("connectMaster", parameters, NYMPH_SINT64);
	connectMasterFunction.setCallback(connectMaster);
	NymphRemoteClient::registerMethod("connectMaster", connectMasterFunction);
	
	// Receives data chunks for playback.
	// uint8 receiveDataMaster(blob data, bool done, sint64)
	parameters.clear();
	parameters.push_back(NYMPH_BLOB);
	parameters.push_back(NYMPH_BOOL);
	parameters.push_back(NYMPH_SINT64);
	NymphMethod receivedataMasterFunction("receiveDataMaster", parameters, NYMPH_UINT8);
	receivedataMasterFunction.setCallback(receiveDataMaster);
	NymphRemoteClient::registerMethod("receiveDataMaster", receivedataMasterFunction);
	
	// Client disconnects from server.
	// bool disconnect()
	parameters.clear();
	NymphMethod disconnectFunction("disconnect", parameters, NYMPH_BOOL);
	disconnectFunction.setCallback(disconnect);
	NymphRemoteClient::registerMethod("disconnect", disconnectFunction);
	
	// Client starts a session.
	// Return value: OK (0), ERROR (1).
	// int session_start()
	parameters.clear();
	parameters.push_back(NYMPH_STRUCT);
	NymphMethod sessionStartFunction("session_start", parameters, NYMPH_UINT8);
	sessionStartFunction.setCallback(session_start);
	NymphRemoteClient::registerMethod("session_start", sessionStartFunction);
	
	// Client sends meta data for the track.
	// Returns: OK (0), ERROR (1).
	// int session_meta(string artist, string album, int track, string name)
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_UINT32);
	parameters.push_back(NYMPH_STRING);
	NymphMethod sessionMetaFunction("session_meta", parameters, NYMPH_UINT8);
	sessionMetaFunction.setCallback(session_meta);
	NymphRemoteClient::registerMethod("session_meta", sessionMetaFunction);
	
	// Client adds slave NymphCast servers to the session.
	// Any slaves will follow the master server exactly when it comes to playback.
	// Returns: OK (0), ERROR (1).
	// int session_add_slave(array servers);
	parameters.clear();
	parameters.push_back(NYMPH_ARRAY);
	NymphMethod sessionAddSlaveFunction("session_add_slave", parameters, NYMPH_UINT8);
	sessionAddSlaveFunction.setCallback(session_add_slave);
	NymphRemoteClient::registerMethod("session_add_slave", sessionAddSlaveFunction);
	
	// Client sends a chunk of track data.
	// Returns: OK (0), ERROR (1).
	// int session_data(string buffer)
	parameters.clear();
	parameters.push_back(NYMPH_BLOB);
	parameters.push_back(NYMPH_BOOL);
	NymphMethod sessionDataFunction("session_data", parameters, NYMPH_UINT8);
	sessionDataFunction.setCallback(session_data);
	NymphRemoteClient::registerMethod("session_data", sessionDataFunction);
	
	// Client ends the session.
	// Returns: OK (0), ERROR (1).
	// int session_end()
	parameters.clear();
	NymphMethod sessionEndFunction("session_end", parameters, NYMPH_UINT8);
	sessionEndFunction.setCallback(session_end);
	NymphRemoteClient::registerMethod("session_end", sessionEndFunction);
	
	// Playback control methods.
	//
	// VolumeSet.
	// uint8 volume_set(uint8 volume)
	// Set volume to between 0 - 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	parameters.push_back(NYMPH_UINT8);
	NymphMethod volumeSetFunction("volume_set", parameters, NYMPH_UINT8);
	volumeSetFunction.setCallback(volume_set);
	NymphRemoteClient::registerMethod("volume_set", volumeSetFunction);
	
	// VolumeUp.
	// uint8 volume_up()
	// Increase volume by 10 up to 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	NymphMethod volumeUpFunction("volume_up", parameters, NYMPH_UINT8);
	volumeUpFunction.setCallback(volume_up);
	NymphRemoteClient::registerMethod("volume_up", volumeUpFunction);
		
	// VolumeDown.
	// uint8 volume_down()
	// Decrease volume by 10 up to 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	NymphMethod volumeDownFunction("volume_down", parameters, NYMPH_UINT8);
	volumeDownFunction.setCallback(volume_down);
	NymphRemoteClient::registerMethod("volume_down", volumeDownFunction);
	
	// PlaybackStart.
	// uint8 playback_start()
	// Start playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackStartFunction("playback_start", parameters, NYMPH_UINT8);
	playbackStartFunction.setCallback(playback_start);
	NymphRemoteClient::registerMethod("playback_start", playbackStartFunction);
	
	// PlaybackStop.
	// uint8 playback_stop()
	// Stop playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackStopFunction("playback_stop", parameters, NYMPH_UINT8);
	playbackStopFunction.setCallback(playback_stop);
	NymphRemoteClient::registerMethod("playback_stop", playbackStopFunction);
	
	// PlaybackPause.
	// uint8 playback_pause()
	// Pause playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackPauseFunction("playback_pause", parameters, NYMPH_UINT8);
	playbackPauseFunction.setCallback(playback_pause);
	NymphRemoteClient::registerMethod("playback_pause", playbackPauseFunction);
	
	// PlaybackRewind.
	// uint8 playback_rewind()
	// Rewind the current file to the beginning.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackRewindFunction("playback_rewind", parameters, NYMPH_UINT8);
	playbackRewindFunction.setCallback(playback_rewind);
	NymphRemoteClient::registerMethod("playback_rewind", playbackRewindFunction);
	
	// PlaybackForward
	// uint8 playback_forward()
	// Forward the current file to the end.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackForwardFunction("playback_forward", parameters, NYMPH_UINT8);
	playbackForwardFunction.setCallback(playback_forward);
	NymphRemoteClient::registerMethod("playback_forward", playbackForwardFunction);
	
	// PlaybackSeek
	// uint8 playback_seek(uint64)
	// Seek to the indicated position.
	// Returns success or error number.
	parameters.clear();
	parameters.push_back(NYMPH_ARRAY);
	NymphMethod playbackSeekFunction("playback_seek", parameters, NYMPH_UINT8);
	playbackSeekFunction.setCallback(playback_seek);
	NymphRemoteClient::registerMethod("playback_seek", playbackSeekFunction);
	
	// PlaybackUrl.
	// uint8 playback_url(string)
	// Try to the play the media file indicated by the provided URL.
	// Returns success or error number.
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	NymphMethod playbackUrlFunction("playback_url", parameters, NYMPH_UINT8);
	playbackUrlFunction.setCallback(playback_url);
	NymphRemoteClient::registerMethod("playback_url", playbackUrlFunction);
	
	// PlaybackStatus
	// struct playback_status()
	// The current state of the NymphCast server.
	// Return struct with information:
	// ["playing"] => boolean (true/false)
	// 
	parameters.clear();
	NymphMethod playbackStatusFunction("playback_status", parameters, NYMPH_STRUCT);
	playbackStatusFunction.setCallback(playback_status);
	NymphRemoteClient::registerMethod("playback_status", playbackStatusFunction);
	
	
	// ReceiverStatus.
	// 
	
	
	// AppList
	// string app_list()
	// Returns a list of installed applications.
	parameters.clear();
	NymphMethod appListFunction("app_list", parameters, NYMPH_STRING);
	appListFunction.setCallback(app_list);
	NymphRemoteClient::registerMethod("app_list", appListFunction);	
	
	// AppSend
	// string app_send(uint32 appId, string data)
	// Allows a client to send data to a NymphCast application.
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	NymphMethod appSendFunction("app_send", parameters, NYMPH_STRING);
	appSendFunction.setCallback(app_send);
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
	NymphMethod appLoadResourceFunction("app_loadResource", parameters, NYMPH_BLOB);
	appLoadResourceFunction.setCallback(app_loadResource);
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
	
	// Install signal handler to terminate the server.
	// Note: SDL will install its own signal handler. It's paramount that our signal handler is
	// 			therefore installed after its, to override it.
	signal(SIGINT, signal_handler);
	
	// Initialise SDL.
	if (!SdlRenderer::init()) {
		std::cerr << "Failed to init SDL. Aborting..." << std::endl;
		return 0;
	}
	
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
			// Start the GUI with the default document.
			if (!Gui::init("index.rml")) {
				// Handle error.
				std::cerr << "Failed to start the GUI. Aborting..." << std::endl;
				init_success = false;
			}
		}
		else {
			ScreenSaver::setDataPath(wallpapersFolder);
			ScreenSaver::start(15);
		}
	}
	
	
	// Wait for the condition to be signalled.
	if (init_success && !gui_enable) {
		gMutex.lock();
		gCon.wait(gMutex);
	}
	else if (gui_enable) {
		// Blocking function, returns when exiting the GUI.
		Gui::start();
	}
	
	std::cout << "Shutting down..." << std::endl;
	
	// Stop screensaver if it's running.
	if (!display_disable) {
		if (gui_enable) {
			Gui::stop();
			Gui::quit();
		}
		else {
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
	
	NyanSD::stopListener();
	NymphRemoteClient::shutdown();
	
	// Wait before exiting, giving threads time to exit.
	Thread::sleep(2000); // 2 seconds.
	
	return 0;
}
