


#include "nymphcast_client.h"

#include <iostream>
#include <vector>
#include <filesystem> 		// C++17

namespace fs = std::filesystem;


void logFunction(int level, std::string logStr) {
	std::cout << level << " - " << logStr << std::endl;
}


// Callback to register with the server. 
// This callback will be called once by the server and then discarded. This is
// useful for one-off events, but can also be used for callbacks during the 
// life-time of the client.
void NymphCastClient::MediaReadCallback(uint32_t session, NymphMessage* msg, void* data) {
	std::cout << "Media Read callback function called.\n";
	
	// Call the 'session_data' remote function with new data buffer.
	// Read N bytes from the file.
	// TODO: receive desired block size here from remote?
	
	// FIXME: we're using 2M blocks for now. This should be made adjustable by the remote.
	uint32_t bufLen = 2048 * 1024;
	char* buffer = new char[bufLen];
	source.read(buffer, bufLen);
	
	// Check characters read.
	NymphBoolean* fileEof = new NymphBoolean(false);
	if (source.gcount() < bufLen) { fileEof->setValue(true); }
	
	std::string block(buffer, source.gcount());
	
	// Debug
	std::cout << "Read block with size " << block.length() << " bytes." << std::endl;
	
	std::vector<NymphType*> values;
	values.push_back(new NymphBlob(block));
	values.push_back(fileEof);
	NymphType* returnValue = 0;
	std::string result;
	if (!NymphRemoteServer::callMethod(session, "session_data", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(session, result);
		//NymphRemoteServer::shutdown();
		exit(1);
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't an int. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(session, result);
		//NymphRemoteServer::shutdown();
		exit(1);
	}
}


void NymphCastClient::MediaStopCallback(uint32_t session, NymphMessage* msg, void* data) {
	std::cout << "Client callback function called.\n";
	
	// Remove the callbacks.
	NymphRemoteServer::removeCallback("MediaReadCallback");
	NymphRemoteServer::removeCallback("MediaStopCallback");
	NymphRemoteServer::removeCallback("MediaSeekCallback");
	
	// End NymphCast session and disconnect from server.
	std::vector<NymphType*> values;
	NymphType* returnValue = 0;
	std::string result;
	if (!NymphRemoteServer::callMethod(session, "session_end", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(session, result);
		//NymphRemoteServer::shutdown();
		exit(1);
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't an int. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(session, result);
		//NymphRemoteServer::shutdown();
		exit(1);
	}
	
	returnValue = 0;
	if (!NymphRemoteServer::callMethod(session, "disconnect", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(session, result);
		//NymphRemoteServer::shutdown();
		exit(1);
	}
	
	if (returnValue->type() != NYMPH_BOOL) {
		std::cout << "Return value wasn't a boolean. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(session, result);
		//NymphRemoteServer::shutdown();
		exit(1);
	}
	
	// Signal the condition variable to terminate the application.
	//cnd.signal();
	
	// TODO: signal the application that playback was ended?
}


void NymphCastClient::MediaSeekCallback(uint32_t session, NymphMessage* msg, void* data) {
	// TODO: implement.
}


// --- CONSTRUCTOR ---
NymphCastClient::NymphCastClient() {
	//
}


// --- DESTRUCTOR ---
NymphCastClient::~NymphCastClient() {
	//
	NymphRemoteServer::shutdown();
}


// --- FIND SERVERS ---
void NymphCastClient::findServers() {
	//
}


// --- CONNECT SERVER ---
bool NymphCastClient::connectServer(uint32_t &handle) {
	// TODO: accept servers.
	std::string serverip = "127.0.0.1";
	
	// TODO: don't shutdown entire remote server on an error.
	
	// Initialise the remote client instance.
	long timeout = 60000; // 60 seconds.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	// Connect to the remote server.
	std::string result;
	if (!NymphRemoteServer::connect(serverip, 4004, handle, 0, result)) {
		std::cout << "Connecting to remote server failed: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		//NymphRemoteServer::shutdown();
		return false;
	}
	
	// Send message and wait for response.
	std::vector<NymphType*> values;
	values.push_back(new NymphString(clientId));
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "connect", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		//NymphRemoteServer::shutdown();
		return false;
	}
	
	if (returnValue->type() != NYMPH_BOOL) {
		std::cout << "Return value wasn't a boolean. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		//NymphRemoteServer::shutdown();
		return false;
	}
	
	delete returnValue;
	returnValue = 0;
	
	
	// The remote NymphCast server works in a pull fashion, which means that we have to register
	// a callback with the server. This callback will be called whenever the server needs more
	// data from the file which we are streaming.
		
	// Register callback and send message with its ID to the server. Then wait
	// for the callback to be called.
	using namespace std::placeholders; 
	NymphRemoteServer::registerCallback("MediaReadCallback", 
										std::bind(&NymphCastClient::MediaReadCallback,
																	this, _1, _2, _3), 0);
	NymphRemoteServer::registerCallback("MediaStopCallback", 
										std::bind(&NymphCastClient::MediaStopCallback,
																	this, _1, _2, _3), 0);
	NymphRemoteServer::registerCallback("MediaSeekCallback", 
										std::bind(&NymphCastClient::MediaSeekCallback,
																	this, _1, _2, _3), 0);
	
	return true;
}


// --- DISCONNECT SERVER ---
bool NymphCastClient::disconnectServer(uint32_t handle) {
	// TODO: don't shutdown entire remote server.
	
	// Shutdown.
	std::string result;
	NymphRemoteServer::disconnect(handle, result);
	NymphRemoteServer::shutdown();
}


// --- CAST FILE ---
bool NymphCastClient::castFile(uint32_t handle, std::string filename) {
	//
	
	fs::path filePath(filename);
	if (!fs::exists(filePath)) {
		std::cerr << "File " << filename << " doesn't exist." << std::endl;
		return 1;
	}
	
	std::cout << "Opening file " << filename << std::endl;
	
	source.open(filename, std::ios::binary);
	if (!source.good()) {
		std::cerr << "Failed to read input file." << std::endl;
		return 1;
	}
	
	// Start the session
	// TODO: send meta data via this method.
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	NymphStruct* ms = new NymphStruct;
	ms->addPair("filesize", new NymphUint32(fs::file_size(filePath)));
	values.clear();
	values.push_back(ms);
	if (!NymphRemoteServer::callMethod(handle, "session_start", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		//NymphRemoteServer::shutdown();
		return false;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		//NymphRemoteServer::shutdown();
		return false;
	}
	
	return true;
}


// --- CAST URL ---
bool NymphCastClient::castUrl(uint32_t handle, std::string url) {
	//
	
	
	return true;
}


// --- VOLUME SET ---
uint8_t NymphCastClient::volumeSet(uint32_t handle, uint8_t volume) {
	//
	
	return 0;
}


// --- VOLUME UP ---
uint8_t NymphCastClient::volumeUp(uint32_t handle) {
	//
	return 0;
}


// --- VOLUME DOWN ---
uint8_t NymphCastClient::volumeDown(uint32_t handle) {
	//
	return 0;
}


// --- PLAYBACK START ---
uint8_t NymphCastClient::playbackStart(uint32_t handle) {
	//
	return 0;
}


// --- PLAYBACK STOP ---
uint8_t NymphCastClient::playbackStop(uint32_t handle) {
	//
	return 0;
}


// --- PLAYBACK PAUSE ---
uint8_t NymphCastClient::playbackPause(uint32_t handle) {
	//
	return 0;
}


// --- PLAYBACK REWIND ---
uint8_t NymphCastClient::playbackRewind(uint32_t handle) {
	//
	return 0;
}


// --- PLAYBACK FORWARD ---
uint8_t NymphCastClient::playbackForward(uint32_t handle) {
	//
	return 0;
}


// --- PLAYBACK SEEK ---
uint8_t NymphCastClient::playbackSeek(uint32_t handle, uint64_t location) {
	//
	return 0;
}
