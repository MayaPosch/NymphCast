/*
	nymphcast_client.cpp - Implementation file for the NymphCast client library.
	
	Revision 0
	
	Notes:
			-
			
	2019/10/26, Maya Posch
*/


#include "nymphcast_client.h"

#include <iostream>
#include <vector>
//#include <filesystem> 		// C++17

//namespace fs = std::filesystem;

#include <Poco/Path.h>
#include <Poco/File.h>

#include "zeroconf.hpp"


enum {
	NYMPH_SEEK_TYPE_BYTES = 1,
	NYMPH_SEEK_TYPE_PERCENTAGE = 2
};


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
		exit(1);
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't an int. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(session, result);
		exit(1);
	}
}


void NymphCastClient::MediaStopCallback(uint32_t session, NymphMessage* msg, void* data) {
	std::cout << "Media Stop callback function called.\n";
		
	// TODO: signal the application that playback was ended.
}


void NymphCastClient::MediaSeekCallback(uint32_t session, NymphMessage* msg, void* data) {
	std::cout << "Media Seek callback called." << std::endl;
	
	// Seek to the indicated position in the file.
	uint64_t position = ((NymphUint64*) msg->parameters()[0])->getValue();
	source.seekg(position);
	
	// Read in first segment.
	// Call the 'session_data' remote function with new data buffer.
	// Read N bytes from the file.
	// TODO: receive desired block size here from remote?
	
	// FIXME: we're using 2M blocks for now. This should be made adjustable by the remote.
	uint32_t bufLen = 2048 * 1024;
	char* buffer = new char[bufLen];
	source.read(buffer, bufLen);
	if (!source.good()) {
		std::cerr << "Error while seeking. Stream bad." << std::endl;
		return;
	}
	
	// Check characters read, set EOF if at the end.
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
		exit(1);
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't an int. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(session, result);
		exit(1);
	}
}


// --- MEDIA STATUS CALLBACK ---
// Gets called every time the active remote media changes status.
void NymphCastClient::MediaStatusCallback(uint32_t session, NymphMessage* msg, void* data) {
	// Send received data to registered callback.
	NymphPlaybackStatus status;
	status.error = true;
	
	NymphStruct* nstruct = ((NymphStruct*) msg->parameters()[0]);
	NymphType* splay;
	if (!nstruct->getValue("playing", splay)) {
		std::cerr << "MediaStatusCallback: Failed to find value 'playing' in struct." << std::endl;
		return;
	}
	
	status.error = false;
	status.playing = ((NymphBoolean*) splay)->getValue();
	NymphType* duration;
	NymphType* position;
	NymphType* volume;
	if (status.playing) {
		if (!nstruct->getValue("duration", duration)) {
			std::cerr << "MediaStatusCallback: Failed to find value 'duration' in struct." << std::endl;
			return;
		}
		
		if (!nstruct->getValue("position", position)) {
			std::cerr << "MediaStatusCallback: Failed to find value 'position' in struct." << std::endl;
			return;
		}
		
		if (!nstruct->getValue("volume", volume)) {
			std::cerr << "MediaStatusCallback: Failed to find value 'volume' in struct." << std::endl;
			return;
		}
		
		status.duration = ((NymphUint64*) duration)->getValue();
		status.position = ((NymphDouble*) position)->getValue();
		status.volume = ((NymphUint8*) volume)->getValue();
	}
	
	if (statusUpdateFunction) {
		statusUpdateFunction(session, status);
	}
}


void NymphCastClient::ReceiveFromAppCallback(uint32_t session, NymphMessage* msg, void* data) {
	std::string appId = ((NymphString*) msg->parameters()[0])->getValue();
	std::string message = ((NymphString*) msg->parameters()[1])->getValue();
	
	if (appMessageFunction) {
		appMessageFunction(appId, message);
	}
}


void PrintLog(Zeroconf::LogLevel level, const std::string& message) {
    switch (level) {
        case Zeroconf::LogLevel::Error:
            std::cout << "E: " << message << std::endl;
            break;
        case Zeroconf::LogLevel::Warning:
            std::cout << "W: " << message << std::endl;
            break;
    }
}


// --- CONSTRUCTOR ---
NymphCastClient::NymphCastClient() {
	// Initialise the remote client instance.
	long timeout = 60000; // 60 seconds.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	appMessageFunction = 0;
	statusUpdateFunction = 0;
	
	Zeroconf::SetLogCallback(PrintLog);
}


// --- DESTRUCTOR ---
NymphCastClient::~NymphCastClient() {
	NymphRemoteServer::shutdown();
}


// --- SET CLIENT ID ---
void NymphCastClient::setClientId(std::string id) {
	clientId = id;
}


// --- SET APPLICATION CALLBACK ---
void NymphCastClient::setApplicationCallback(AppMessageFunction function) {
	appMessageFunction = function;
}


// --- SET STATUS UPDATE FUNCTION ---
void NymphCastClient::setStatusUpdateCallback(StatusUpdateFunction function) {
	statusUpdateFunction = function;
}


// --- GET APPLICATION LIST ---
std::string NymphCastClient::getApplicationList(uint32_t handle) {
	// Request the application list from the remote receiver.
	// string app_list()
	std::vector<NymphType*> values;
	NymphType* returnValue = 0;
	std::string result;
	if (!NymphRemoteServer::callMethod(handle, "app_list", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		return std::string();
	}
	
	if (returnValue->type() != NYMPH_STRING) {
		std::cout << "Return value wasn't a string. Type: " << returnValue->type() << std::endl;
		return std::string();
	}
	
	return ((NymphString*) returnValue)->getValue();
}


// --- SEND APPLICATION MESSAGE ---
std::string NymphCastClient::sendApplicationMessage(uint32_t handle, std::string appId, 
																		std::string message) {
	// string app_send(uint32 appId, string data)
	std::vector<NymphType*> values;
	values.push_back(new NymphString(appId));
	values.push_back(new NymphString(message));
	NymphType* returnValue = 0;
	std::string result;
	if (!NymphRemoteServer::callMethod(handle, "app_send", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		return std::string();
	}
	
	if (returnValue->type() != NYMPH_STRING) {
		std::cout << "Return value wasn't a string. Type: " << returnValue->type() << std::endl;
		return std::string();
	}
	
	return ((NymphString*) returnValue)->getValue();
}


void* get_in_addr(sockaddr_storage* sa) {
	if (sa->ss_family == AF_INET) {
		return &reinterpret_cast<sockaddr_in*>(sa)->sin_addr;
	}

	if (sa->ss_family == AF_INET6) {
		return &reinterpret_cast<sockaddr_in6*>(sa)->sin6_addr;
	}

	return nullptr;
}


// --- FIND SERVERS ---
std::vector<NymphCastRemote> NymphCastClient::findServers() {
	// Perform an mDNS/DNS-SD service discovery run for NymphCast receivers.
	std::vector<Zeroconf::mdns_responce> items;
	bool res = Zeroconf::Resolve("_nymphcast._tcp.local", 3, &items);
	
	std::vector<NymphCastRemote> remotes;
	if (!res) {
		std::cout << "Error resolving DNS-SD request." << std::endl;
		return remotes;
	}
	
	std::cout << "Found " << items.size() << " remotes matching _nymphcast._tcp.local" << std::endl;
	
	// Extract the server name, IP address and port.
	if (items.empty()) { return remotes; }
	for (int i = 0; i < items.size(); ++i) {
		NymphCastRemote rm;
		
		char buffer[INET6_ADDRSTRLEN + 1] = {0};
        inet_ntop(items[i].peer.ss_family, get_in_addr(&(items[i].peer)), buffer, INET6_ADDRSTRLEN);
		
		std::cout << "Peer: " << buffer << std::endl;
		rm.ipv4 = std::string(buffer);
		rm.ipv6 = "";
		
		rm.name = items[i].records[0].name;
		rm.port = 4004;
		remotes.push_back(rm);
	}
	
	return remotes;
}


// --- CONNECT SERVER ---
bool NymphCastClient::connectServer(std::string ip, uint32_t &handle) {
	std::string serverip = "127.0.0.1";
	if (!ip.empty()) {
		serverip = ip;
	}
		
	// Connect to the remote server.
	std::string result;
	if (!NymphRemoteServer::connect(serverip, 4004, handle, 0, result)) {
		std::cout << "Connecting to remote server failed: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return false;
	}
	
	// Send message and wait for response.
	std::vector<NymphType*> values;
	values.push_back(new NymphString(clientId));
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "connect", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return false;
	}
	
	if (returnValue->type() != NYMPH_BOOL) {
		std::cout << "Return value wasn't a boolean. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
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
	NymphRemoteServer::registerCallback("MediaStatusCallback", 
										std::bind(&NymphCastClient::MediaStatusCallback,
																	this, _1, _2, _3), 0);
	
	return true;
}


// --- DISCONNECT SERVER ---
bool NymphCastClient::disconnectServer(uint32_t handle) {
	// TODO: don't shutdown entire remote server.
	
	// Remove the callbacks.
	NymphRemoteServer::removeCallback("MediaReadCallback");
	NymphRemoteServer::removeCallback("MediaStopCallback");
	NymphRemoteServer::removeCallback("MediaSeekCallback");
	
	// Shutdown.
	std::string result;
	NymphRemoteServer::disconnect(handle, result);
	
	return true;
}


// --- CAST FILE ---
bool NymphCastClient::castFile(uint32_t handle, std::string filename) {
	/* fs::path filePath(filename);
	if (!fs::exists(filePath)) { */
	Poco::File file(filename);
	if (!file.exists()) {
		std::cerr << "File " << filename << " doesn't exist." << std::endl;
		return 1;
	}
	
	std::cout << "Opening file " << filename << std::endl;
	
	if (source.is_open()) {
		source.close();
	}
	
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
	//ms->addPair("filesize", new NymphUint32(fs::file_size(filePath)));
	ms->addPair("filesize", new NymphUint32((uint32_t) file.getSize()));
	values.clear();
	values.push_back(ms);
	if (!NymphRemoteServer::callMethod(handle, "session_start", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return false;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return false;
	}
	
	return true;
}


// --- CAST URL ---
bool NymphCastClient::castUrl(uint32_t handle, std::string url) {
	// uint8 playback_url(string)
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	values.push_back(new NymphString(url));
	if (!NymphRemoteServer::callMethod(handle, "session_start", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return false;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return false;
	}
	
	return true;
}


// --- VOLUME SET ---
// Volume is set within a range of 0 - 128.
uint8_t NymphCastClient::volumeSet(uint32_t handle, uint8_t volume) {
	// uint8 volume_set(uint8 volume)
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	values.push_back(new NymphUint8(volume));
	if (!NymphRemoteServer::callMethod(handle, "volume_set", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- VOLUME UP ---
uint8_t NymphCastClient::volumeUp(uint32_t handle) {
	// uint8 volume_up()
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "volume_up", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- VOLUME DOWN ---
uint8_t NymphCastClient::volumeDown(uint32_t handle) {
	// uint8 volume_down()
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "volume_down", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- PLAYBACK START ---
uint8_t NymphCastClient::playbackStart(uint32_t handle) {
	// uint8 playback_start()
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "playback_start", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- PLAYBACK STOP ---
uint8_t NymphCastClient::playbackStop(uint32_t handle) {
	// uint8 playback_stop()
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "playback_stop", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- PLAYBACK PAUSE ---
uint8_t NymphCastClient::playbackPause(uint32_t handle) {
	// uint8 playback_pause()
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "playback_pause", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- PLAYBACK REWIND ---
uint8_t NymphCastClient::playbackRewind(uint32_t handle) {
	// uint8 playback_rewind()
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "playback_rewind", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- PLAYBACK FORWARD ---
uint8_t NymphCastClient::playbackForward(uint32_t handle) {
	// uint8 playback_forward()
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "playback_forward", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- PLAYBACK SEEK ---
// Seek to specific byte offset in the file.
uint8_t NymphCastClient::playbackSeek(uint32_t handle, uint64_t location) {
	// uint8 playback_seek(array)
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	
	NymphArray* valArray = new NymphArray();
	valArray->addValue(new NymphUint8(NYMPH_SEEK_TYPE_BYTES));
	valArray->addValue(new NymphUint64(location));	
	values.push_back(valArray);
	
	if (!NymphRemoteServer::callMethod(handle, "playback_seek", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- PLAYBACK SEEK ---
// Seek to percentage of the media file length.
uint8_t NymphCastClient::playbackSeek(uint32_t handle, uint8_t percentage) {
	// uint8 playback_seek(array)
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	
	NymphArray* valArray = new NymphArray();
	valArray->addValue(new NymphUint8(NYMPH_SEEK_TYPE_PERCENTAGE));
	valArray->addValue(new NymphUint8(percentage));	
	values.push_back(valArray);
	
	if (!NymphRemoteServer::callMethod(handle, "playback_seek", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return 0;
	}
	
	return ((NymphUint8*) returnValue)->getValue();
}


// --- PLAYBACK STATUS ---
NymphPlaybackStatus NymphCastClient::playbackStatus(uint32_t handle) {
	NymphPlaybackStatus status;
	status.error = true;
	
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "playback_status", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return status;
	}
	
	if (returnValue->type() != NYMPH_STRUCT) {
		std::cout << "Return value wasn't a struct. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return status;
	}
	
	NymphStruct* nstruct = ((NymphStruct*) returnValue);
	NymphType* splay;
	if (!nstruct->getValue("playing", splay)) {
		return status;
	}
	
	status.playing = ((NymphBoolean*) splay)->getValue();
	NymphType* duration;
	NymphType* position;
	NymphType* volume;
	if (status.playing) {
		if (!nstruct->getValue("duration", duration)) {
			std::cerr << "playbackStatus: Failed to find value 'duration' in struct." << std::endl;
			return status;
		}
		
		if (!nstruct->getValue("position", position)) {
			std::cerr << "playbackStatus: Failed to find value 'position' in struct." << std::endl;
			return status;
		}
		
		
		if (!nstruct->getValue("volume", volume)) {
			std::cerr << "playbackStatus: Failed to find value 'volume' in struct." << std::endl;
			return status;
		}
		
		status.duration = ((NymphUint64*) duration)->getValue();
		status.position = ((NymphDouble*) position)->getValue();
		status.volume = ((NymphDouble*) volume)->getValue();
	}
	
	status.error = false;
	
	return status;
}
