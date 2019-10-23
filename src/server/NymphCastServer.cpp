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
#include <atomic>

#include "ffplay.h"
#include "screensaver.h"

#include <nymph/nymph.h>

#include <Poco/Condition.h>
#include <Poco/Thread.h>

using namespace Poco;


// Global objects.
Condition gCon;
Mutex gMutex;
// ---


#ifdef main
#undef main
#endif


struct FileMetaInfo {
	uint32_t filesize;	// bytes.
	uint32_t duration;	// milliseconds
	uint32_t width;		// pixels
	uint32_t height;	// pixels
	uint32_t video_rate;	// kilobits per second
	uint32_t audio_rate;	// kilobits per second
	uint32_t framrate;
	uint8_t audio_channels;
	std::string title;
	std::string artist;
	std::string album;
};


// --- Globals ---
std::atomic<bool> playerStarted;
Poco::Thread avThread;
Ffplay ffplay;
// ---


// --- DATA REQUEST FUNCTION ---
// This function can be signalled with the condition variable to request data from the client.
void dataRequestFunction() {
	while (1) {
		// Wait for the condition to be signalled.
		media_buffer.requestMutex.lock();
		media_buffer.requestCondition.wait(media_buffer.requestMutex);
		
		if (media_buffer.requestInFlight) { continue; }
		
		// Request more data.
		// TODO: Initial buffer size is 2 MB. Make this dynamically scale.
		std::vector<NymphType*> values;
		std::string result;
		NymphBoolean* resVal = 0;
		if (!NymphRemoteClient::callCallback(media_buffer.activeSession, "MediaReadCallback", values, result)) {
			std::cerr << "Calling callback failed: " << result << std::endl;
			return;
		}
		
		media_buffer.requestInFlight = true;
	}
}


// --- PLAYER DONE CALLBACK ---
// Called when the player has finished with the media track and has shut down.


void resetDataBuffer() {
	media_buffer.currentIndex = 0;		// The current index into the vector element.
	media_buffer.currentSlot = 0;		// The current vector slot we're using.
	media_buffer.numSlots = 50;			// Total number of slots in the data vector.
	media_buffer.nextSlot = 0;			// Next slot to fill in the buffer vector.
	media_buffer.buffIndexLow = 0;		// File index at the buffer front.
	media_buffer.buffIndexHigh = 0;	
	media_buffer.freeSlots = 50;
	media_buffer.eof = false;
	media_buffer.requestInFlight = false;
	
	playerStarted = false;
	
	// Send message to client indicating that we're done.
	std::vector<NymphType*> values;
	std::string result;
	NymphBoolean* resVal = 0;
	if (!NymphRemoteClient::callCallback(media_buffer.activeSession, "MediaStopCallback", values, result)) {
		std::cerr << "Calling media stop callback failed: " << result << std::endl;
		return;
	}
	
	// Start the Screensaver here for now.
	ScreenSaver::start(5);
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


// Callback for the connect function.
NymphMessage* connect(int session, NymphMessage* msg, void* data) {
	std::cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << "\n";
	
	std::string clientStr = ((NymphString*) msg->parameters()[0])->getValue();
	std::cout << "Client string: " << clientStr << "\n";
	
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
	
	returnMsg->setResultValue(retVal);
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
	
    media_buffer.size = it->second.filesize; // Set to stream size.
	media_buffer.activeSession = session;
	
	// Start calling the client's read callback method to obtain data. Once the data buffer
	// has been filled sufficiently, start the playback.
	// TODO: Initial buffer size is 1 MB. Make this dynamically scale.
	media_buffer.requestInFlight = false;
	media_buffer.requestCondition.signal();
	it->second.sessionActive = true;
	
	// Stop screensaver.
	ScreenSaver::stop();
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// Client sends meta data for the track.
// Returns: OK (0), ERROR (1).
// int session_meta(string artist, string album, int track, string name)
NymphMessage* session_meta(int session, NymphMessage* msg, void* data) {
	// X unused function.
}


// Client sends a chunk of track data.
// Returns: OK (0), ERROR (1).
// int session_data(string buffer, boolean done)
NymphMessage* session_data(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// TODO: if this boolean is false already, dismiss message?
	media_buffer.requestInFlight = false;
	
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
	
	// Copy pointer into free slot of vector, delete data if not empty.
	// Reset the next slot value if the end of the vector has been reached.
	// 
	// TODO: optimise.
	// TODO: prevent accidental overwriting.
	// TODO: update front/back index counters.
	if (media_buffer.freeSlots > 0) {
		std::cout << "Writing into buffer slot: " << media_buffer.nextSlot << std::endl;
		media_buffer.mutex.lock();
		media_buffer.data[media_buffer.nextSlot] = mediaData;
		media_buffer.mutex.unlock();
		if (media_buffer.nextSlot == media_buffer.currentSlot) {
			media_buffer.slotSize = mediaData.length();
			media_buffer.slotBytesLeft = mediaData.length();
		}
		
		media_buffer.nextSlot++;
		if (!(media_buffer.nextSlot < media_buffer.numSlots)) { media_buffer.nextSlot = 0; }
		
		std::cout << "Next buffer slot: " << media_buffer.nextSlot << std::endl;
		
		media_buffer.freeSlots--;
		media_buffer.buffBytesLeft += mediaData.length();
	}
	
	// Signal the condition variable in the VLC read callback in case we're waiting there.
	media_buffer.bufferDelayCondition.signal();
	
	// Start the player if it hasn't yet. This ensures we have a buffer ready.
	if (!playerStarted && done) {
		playerStarted = true;
		//ffplay.setBuffer(&media_buffer);
		avThread.start(ffplay);
	}
	
	// if 'done' is true, the client has sent the last bytes. Signal session end in this case.
	if (done) {
		media_buffer.eof = true;
	}
	else {
		// If there are free slots in the buffer, request more data from the client.
		if (!media_buffer.requestInFlight && !(media_buffer.eof) && media_buffer.freeSlots > 0) {
			media_buffer.requestCondition.signal();
		}
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


// --- LOG FUNCTION ---
void logFunction(int level, std::string logStr) {
	std::cout << level << " - " << logStr << std::endl;
}


int main() {
	// Initialise the server.
	std::cout << "Initialising server...\n";
	long timeout = 5000; // 5 seconds.
	//NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_INFO, timeout);
	
	
	// Define all of the RPC methods we want to export for clients.
	std::cout << "Registering methods...\n";
	std::vector<NymphTypes> parameters;
	
	// Client connects to server.
	// bool connect(string client_id)
	parameters.push_back(NYMPH_STRING);
	NymphMethod connectFunction("connect", parameters, NYMPH_BOOL);
	connectFunction.setCallback(connect);
	NymphRemoteClient::registerMethod("connect", connectFunction);
	
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
	// End client callback registration.
	
	// Create empty buffer with N entries, initialised as empty strings.
	media_buffer.mutex.lock();
	media_buffer.data.assign(50, std::string());
	media_buffer.size = media_buffer.data.size();
	media_buffer.mutex.unlock();
	media_buffer.currentIndex = 0;		// The current index into the vector element.
	media_buffer.currentSlot = 0;		// The current vector slot we're using.
	media_buffer.numSlots = 50;			// Total number of slots in the data vector.
	media_buffer.nextSlot = 0;			// Next slot to fill in the buffer vector.
	media_buffer.buffIndexLow = 0;		// File index at the buffer front.
	media_buffer.buffIndexHigh = 0;	
	media_buffer.freeSlots = 50;
	media_buffer.eof = false;
	media_buffer.requestInFlight = false;
	
	playerStarted = false;
	
	// Install signal handler to terminate the server.
	signal(SIGINT, signal_handler);
	
	// Start server on port 4004.
	NymphRemoteClient::start(4004);
	
	// Start the data request handler in its own thread.
	std::thread drq(dataRequestFunction);
	
	// Start idle wallpaper & clock display.
	// Transition time is 5 seconds.
	ScreenSaver::start(5);
	
	// Advertise presence via mDNS.
	
	
	// Wait for the condition to be signalled.
	gMutex.lock();
	gCon.wait(gMutex);
	
	// Clean-up
 
	// Close window and clean up libSDL.
	ffplay.quit();
	avThread.join();
	
	NymphRemoteClient::shutdown();
	
	// Wait before exiting, giving threads time to exit.
	Thread::sleep(2000); // 2 seconds.
	
	return 0;
}
