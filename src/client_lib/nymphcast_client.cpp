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

#include "nyansd.h"


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
		return;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't an int. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(session, result);
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
	std::cout << "Seeking to position: " << position << std::endl;
	if (source.eof()) {
		std::cout << "Clearing EOF flag..." << std::endl;
		source.clear();
	}
	
	// Seek from the beginning of the file.
	std::cout << "Seeking from file beginning..." << std::endl;
	source.seekg(0);
	source.seekg(position);
	
	// Read in first segment.
	// Call the 'session_data' remote function with new data buffer.
	// Read N bytes from the file.
	// TODO: receive desired block size here from remote?
	
	// FIXME: we're using 2M blocks for now. This should be made adjustable by the remote.
	uint32_t bufLen = 2048 * 1024;
	char* buffer = new char[bufLen];
	source.read(buffer, bufLen);
	
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
		return;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't an int. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(session, result);
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


// --- CONSTRUCTOR ---
/**
	Initialise the remote client instance with default settings.
*/
NymphCastClient::NymphCastClient() {
	// Initialise the remote client instance.
	long timeout = 2000; // 2 seconds.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	appMessageFunction = 0;
	statusUpdateFunction = 0;
}


// --- DESTRUCTOR ---
NymphCastClient::~NymphCastClient() {
	NymphRemoteServer::shutdown();
}


// --- SET CLIENT ID ---
/**
	Set the callback to call when a remote application sends data.
	
	@param id A unique string ID.
*/
void NymphCastClient::setClientId(std::string id) {
	clientId = id;
}


// --- SET APPLICATION CALLBACK ---
/**
	Set the callback to call when a remote application sends data.
	
	@param function The callback function.
*/
void NymphCastClient::setApplicationCallback(AppMessageFunction function) {
	appMessageFunction = function;
}


// --- SET STATUS UPDATE FUNCTION ---
/**
	Set callback to call when the remote sends a status update on e.g. playback.
	
	@param function The callback function.
*/
void NymphCastClient::setStatusUpdateCallback(StatusUpdateFunction function) {
	statusUpdateFunction = function;
}


// --- GET APPLICATION LIST ---
/**
	Obtain a list of application available on the remote.
	
	@param handle The handle for the remote server.
	
	@return A string containing the application list, separated by newlines (\n).
*/
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
/**
	Send a string to an application on the remote.
	
	@param handle The handle for the remote server.
	@param appId ID of the remote application.
	@param message Message to send to the remote application.
	
	@return String with any response from the remote.
*/
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


// --- LOAD RESOURCE ---
// Obtain the named file data for either the appId or global if left empty.
/**
	Load a specific resource from a remote application. E.g. an image.
	
	@param handle 	The handle for the remote server.
	@param appId 	ID of the remote application.
	@param name 	The name of the resource.
	
	@return A string containing the (binary) data, if successful.
*/
std::string NymphCastClient::loadResource(uint32_t handle, std::string appId, std::string name) {
	// string app_loadResource(string appId, string name)
	std::vector<NymphType*> values;
	values.push_back(new NymphString(appId));
	values.push_back(new NymphString(name));
	NymphType* returnValue = 0;
	std::string result;
	if (!NymphRemoteServer::callMethod(handle, "app_loadResource", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		return std::string();
	}
	
	if (returnValue->type() != NYMPH_BLOB) {
		std::cout << "Return value wasn't a string. Type: " << returnValue->type() << std::endl;
		return std::string();
	}
	
	return ((NymphBlob*) returnValue)->getValue();
}


bool isDuplicate(std::vector<NymphCastRemote> &remotes, NymphCastRemote &rm) {
	for (uint32_t j = 0; j < remotes.size(); ++j) {
		if (remotes[j].ipv4 == rm.ipv4 &&
			remotes[j].ipv6 == rm.ipv6 &&
			remotes[j].name == rm.name &&
			remotes[j].port == rm.port) {
			return true;
		}
	}
	
	return false;
}


// --- FIND SERVERS ---
/**
	Find remote NymphCast servers using a NyanSD query.
	
	@return A vector with any found remotes.
*/
std::vector<NymphCastRemote> NymphCastClient::findServers() {
	// Perform a NyanSD service discovery run for NymphCast receivers.
	std::vector<NYSD_query> queries;
	std::vector<NYSD_service> responses;
	std::vector<NymphCastRemote> remotes;
	
	NYSD_query query;
	query.protocol = NYSD_PROTOCOL_ALL;
	query.filter = "nymphcast";
	queries.push_back(query);
	if (!NyanSD::sendQuery(4004, queries, responses)) { return remotes; }
	
	// Process responses. 
	// Check for potential duplicate responses.
	for (int i = 0; i < responses.size(); ++i) {
		NymphCastRemote rm;
		rm.ipv4 = NyanSD::ipv4_uintToString(responses[i].ipv4);
		rm.ipv6 = responses[i].ipv6;
		rm.name = responses[i].hostname;
		rm.port = responses[i].port;
		
		// Check for duplicates.
		if (isDuplicate(remotes, rm)) {
			std::cout << "Skipping duplicate for " << rm.name << std::endl;
			continue;
		}
		
		remotes.push_back(rm);
	}
	
	return remotes;
}


bool isDuplicateName(std::vector<NymphCastRemote> &remotes, NymphCastRemote &rm) {
	for (uint32_t j = 0; j < remotes.size(); ++j) {
		if (remotes[j].name == rm.name &&
			remotes[j].port == rm.port) {
			return true;
		}
	}
	
	return false;
}


// --- FIND SHARES ---
/**
	Find any NymphCast Media Servers on the network using a NyanSD query.
	
	@return Vector containing any found media server instances.
*/
std::vector<NymphCastRemote> NymphCastClient::findShares() {
	// Perform NyanSD service discovery query for NymphCast media servers.
	std::vector<NYSD_query> queries;
	std::vector<NYSD_service> responses;
	std::vector<NymphCastRemote> remotes;
	
	NYSD_query query;
	query.protocol = NYSD_PROTOCOL_ALL;
	query.filter = "nymphcast_mediaserver";
	queries.push_back(query);
	if (!NyanSD::sendQuery(4004, queries, responses)) { return remotes; }
	
	// Process responses.
	for (int i = 0; i < responses.size(); ++i) {
		NymphCastRemote rm;
		rm.ipv4 = NyanSD::ipv4_uintToString(responses[i].ipv4);
		rm.ipv6 = responses[i].ipv6;
		rm.name = responses[i].hostname;
		rm.port = responses[i].port;
		
		// Check for duplicates.
		if (isDuplicate(remotes, rm) || isDuplicateName(remotes, rm)) {
			std::cout << "Skipping duplicate for " << rm.name << std::endl;
			continue;
		}
		
		remotes.push_back(rm);
	}
	
	return remotes;
}


// --- CONNECT SERVER ---
/**
	Attempt to connect to the specified remote NymphCast server.
	
	@param ip		The IP address of the target server.
	@param handle 	The new handle for the remote server.
	
	@return True if the operation succeeded.
*/
bool NymphCastClient::connectServer(std::string ip, uint32_t &handle) {
	std::string serverip = "127.0.0.1";
	if (!ip.empty()) {
		serverip = ip;
	}
		
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
	
	return true;
}


// --- DISCONNECT SERVER ---
/**
	Disconnect from the remote server.
	
	@param handle The handle for the remote server.
	
	@return True if the operation succeeded.
*/
bool NymphCastClient::disconnectServer(uint32_t handle) {
	// TODO: don't shutdown entire remote server.
	
	// Remove the callbacks.
	NymphRemoteServer::removeCallback("MediaReadCallback");
	NymphRemoteServer::removeCallback("MediaStopCallback");
	NymphRemoteServer::removeCallback("MediaSeekCallback");
	
	// Send disconnect command.
	std::string result;
	std::vector<NymphType*> values;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "disconnect", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return false;
	}
	
	// Shutdown.
	NymphRemoteServer::disconnect(handle, result);
	
	return true;
}


// --- GET SHARES ---
/**
	Attempt to obtain the list of shared media files from a NymphCast Media Server.
	
	@param mediaserver	Information on the target media server instance.
	
	@return Vector with the list of available files, if successful.
*/
std::vector<NymphMediaFile> NymphCastClient::getShares(NymphCastRemote mediaserver) {
	std::vector<NymphMediaFile> files;
	
	// Establish new connection to mediaserver.
	uint32_t mshandle;
	std::string result;
	if (!NymphRemoteServer::connect(mediaserver.ipv4, 4004, mshandle, 0, result)) {
		std::cout << "Connecting to remote server failed: " << result << std::endl;
		return files;
	}
	
	// Call RPC function to get the list of shared files on the server.
	std::vector<NymphType*> values;
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(mshandle, "getFileList", values, returnValue, result)) {
		std::cout << "Error calling remote method getFileList: " << result << std::endl;
		return files;
	}
	
	if (returnValue->type() != NYMPH_ARRAY) {
		std::cout << "Return value wasn't an array. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(mshandle, result);
		return files;
	}
	
	// Disconnect from remote mediaserver.
	NymphRemoteServer::disconnect(mshandle, result);
	
	// Parse array and return it.
	std::vector<NymphType*> ncf = ((NymphArray*) returnValue)->getValues();
	for (int j = 0; j < ncf.size(); ++j) {
		NymphMediaFile file;
		file.mediaserver = mediaserver;
		NymphType* value = 0;
		if (!((NymphStruct*) ncf[j])->getValue("id", value)) { return files; }
		file.id = ((NymphUint32*) value)->getValue();
		if (!((NymphStruct*) ncf[j])->getValue("filename", value)) { return files; }
		file.name = ((NymphString*) value)->getValue();
		if (!((NymphStruct*) ncf[j])->getValue("section", value)) { return files; }
		file.section = ((NymphString*) value)->getValue();
		
		files.push_back(file);
	}
		
	return files;
}


// --- PLAY SHARE ---
/**
	Instruct a Media Server instance to play back a specific shared file on the target remote servers.
	
	If multiple receivers are specified, the first one becomes the Master receiver, with the remaining receivers configured as Slave receivers to that one Master receiver. This allows for synchronous playback.
	
	@param file			Definition of the shared 
	@param receivers 	Vector of remote servers to play the content back on.
	
	@return True if the operation succeeded.
*/
bool NymphCastClient::playShare(NymphMediaFile file, std::vector<NymphCastRemote> receivers) {
	if (receivers.empty()) { return false; }
	
	// Establish new connection to mediaserver.
	uint32_t mshandle;
	std::string result;
	if (!NymphRemoteServer::connect(file.mediaserver.ipv4, 4004, mshandle, 0, result)) {
		std::cout << "Connecting to remote server failed: " << result << std::endl;
		return false;
	}
	
	// Encode receivers.
	NymphArray* recArr = new NymphArray();
	for (int i = 0; i < receivers.size(); ++i) {
		NymphStruct* remote = new NymphStruct;
		remote->addPair("name", new NymphString(receivers[i].name));
		remote->addPair("ipv4", new NymphString(receivers[i].ipv4));
		remote->addPair("ipv6", new NymphString(receivers[i].ipv6));
		recArr->addValue(remote);
	}
	
	// Encode file data.
	NymphUint32* fileId = new NymphUint32(file.id);
	
	// Call RPC function to get the list of shared files on the server.
	std::vector<NymphType*> values;
	values.push_back(fileId);
	values.push_back(recArr);
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(mshandle, "playMedia", values, returnValue, result)) {
		std::cout << "Error calling remote method playMedia: " << result << std::endl;
		return false;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(mshandle, result);
		return false;
	}
	
	// Disconnect from remote mediaserver.
	NymphRemoteServer::disconnect(mshandle, result);
	
	// Check result.
	uint8_t res = ((NymphUint8*) returnValue)->getValue();
	if (res != 0) { return false; }
	
	return true;
}


// --- ADD SLAVES ---
// Send a list of slave remotes which the target remote will mirror its playback status on.
// This includes audio and video playback.
/**
	Configure a Master remote to use the provided Slave remotes for synchronous playback of content.
	
	@param handle 	The handle for the remote server.
	@param remotes 	Vector of remotes to configure as Slaves.
	
	@return True if the operation succeeded.
*/
bool NymphCastClient::addSlaves(uint32_t handle, std::vector<NymphCastRemote> remotes) {
	NymphArray* sArr = new NymphArray;
	for (int i = 0; i < remotes.size(); ++i) {
		NymphStruct* remote = new NymphStruct;
		remote->addPair("name", new NymphString(remotes[i].name));
		remote->addPair("ipv4", new NymphString(remotes[i].ipv4));
		remote->addPair("ipv6", new NymphString(remotes[i].ipv6));
		sArr->addValue(remote);
	}
	
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	values.push_back(sArr);
	if (!NymphRemoteServer::callMethod(handle, "session_add_slave", values, returnValue, result)) {
		std::cout << "Error calling remote method session_add_slave: " << result << std::endl;
		return false;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't a uint8. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		return false;
	}
	
	return true;
}


// --- CAST FILE ---
/**
	Stream file to remote. Use the provided file path to open the media file and stream its contents to the remote.
	
	@param handle The handle for the remote server.
	@param filename The (relative) path to the media file.
	
	@return True if the operation succeeded.
*/
bool NymphCastClient::castFile(uint32_t handle, std::string filename) {
	// Empty filename not handled by `!file.exists()` below.
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
/**
	Send the provided URL to the remote, which will attempt to play back any media content found.
	
	@param handle 	The handle for the remote server.
	@param url		URL to play back from.
	
	@return True if the operation succeeded.
*/
bool NymphCastClient::castUrl(uint32_t handle, std::string url) {
	// uint8 playback_url(string)
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	values.push_back(new NymphString(url));
	if (!NymphRemoteServer::callMethod(handle, "playback_url", values, returnValue, result)) {
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
/**
	Set the volume on the target remote. Volume is set within a range of 0 - 128.
	
	@param handle 	The handle for the remote server.
	@param volume	Target volume level.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Increase the volume on the remote server.
	
	@param handle 	The handle for the remote server.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Decrease the volume on the remote server.
	
	@param handle 	The handle for the remote server.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Start or resume playback on the remote.
	
	@param handle 	The handle for the remote server.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Stop playback on the remote.
	
	@param handle 	The handle for the remote server.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Pause playback on the remote.
	
	@param handle 	The handle for the remote server.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Rewind playback on the remote.
	
	@param handle 	The handle for the remote server.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Forward playback on the remote.
	
	@param handle 	The handle for the remote server.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Seek to specific byte offset in the file.
	
	@param handle 	The handle for the remote server.
	@param location	New offset in the file, in bytes.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Seek to percentage of the media file length.
	
	@param handle 		The handle for the remote server.
	@param percentage	New percentage in the file of media length.
	
	@return 0 if the operation succeeded.
*/
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
/**
	Request the playback status from the remote.
	
	@param handle 	The handle for the remote server.
	
	@return A NymphPlaybackStatus struct with playback information.
*/
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
