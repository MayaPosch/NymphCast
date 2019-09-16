/*
	NymphCastClient.cpp - Client for creating sessions with a NymphCast server.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
		
	2019/01/25, Maya Posch
*/


#include <nymph/nymph.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem> 		// C++17

namespace fs = std::filesystem;

using namespace std;

#include <Poco/Condition.h>

using namespace Poco;


// Globals
Condition cnd;
Mutex mtx;

int handle;		// NymphRPC handle.
std::ifstream source;
// ---


void logFunction(int level, string logStr) {
	cout << level << " - " << logStr << endl;
}


// Callback to register with the server. 
// This callback will be called once by the server and then discarded. This is
// useful for one-off events, but can also be used for callbacks during the 
// life-time of the client.
void MediaReadCallback(NymphMessage* msg, void* data) {
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
	if (!NymphRemoteServer::callMethod(handle, "session_data", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		exit(1);
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't an int. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		exit(1);
	}
}


void MediaStopCallback(NymphMessage* msg, void* data) {
	std::cout << "Client callback function called.\n";
	
	// Remove the callbacks.
	NymphRemoteServer::removeCallback("MediaReadCallback");
	NymphRemoteServer::removeCallback("MediaStopCallback");
	
	// End NymphCast session and disconnect from server.
	std::vector<NymphType*> values;
	NymphType* returnValue = 0;
	std::string result;
	if (!NymphRemoteServer::callMethod(handle, "session_end", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		exit(1);
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		std::cout << "Return value wasn't an int. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		exit(1);
	}
	
	returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "disconnect", values, returnValue, result)) {
		std::cout << "Error calling remote method: " << result << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		exit(1);
	}
	
	if (returnValue->type() != NYMPH_BOOL) {
		std::cout << "Return value wasn't a boolean. Type: " << returnValue->type() << std::endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		exit(1);
	}
	
	// Signal the condition variable to terminate the application.
	cnd.signal();
}


int main(int argc, char *argv[]) {
	// Locate the available servers.
	// TODO: implement something.
	
	// Try to open the file.
	if (argc != 2) {
		std::cerr << "Usage: nymphcast_client <filename>" << std::endl;
		return 1;
	}
	
	std::string filename(argv[1]);
	
	std::cout << "Opening file " << filename << std::endl;
	
	fs::path filePath(argv[1]);
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
	
	// Initialise the remote client instance.
	long timeout = 15000; // 15 seconds.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	// Connect to the remote server.
	std::string result;
	//if (!NymphRemoteServer::connect("127.0.0.1", 4004, handle, 0, result)) {
	if (!NymphRemoteServer::connect("192.168.178.26", 4004, handle, 0, result)) {
		cout << "Connecting to remote server failed: " << result << endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		return 1;
	}
	
	// Send message and wait for response.
	vector<NymphType*> values;
	values.push_back(new NymphString("NymphClient_21xb"));
	NymphType* returnValue = 0;
	if (!NymphRemoteServer::callMethod(handle, "connect", values, returnValue, result)) {
		cout << "Error calling remote method: " << result << endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		return 1;
	}
	
	if (returnValue->type() != NYMPH_BOOL) {
		cout << "Return value wasn't a boolean. Type: " << returnValue->type() << endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		return 1;
	}
	
	delete returnValue;
	returnValue = 0;
	
	
	// The remote NymphCast server works in a pull fashion, which means that we have to register
	// a callback with the server. This callback will be called whenever the server needs more
	// data from the file which we are streaming.
		
	// Register callback and send message with its ID to the server. Then wait
	// for the callback to be called.
	NymphRemoteServer::registerCallback("MediaReadCallback", MediaReadCallback, 0);
	NymphRemoteServer::registerCallback("MediaStopCallback", MediaStopCallback, 0);
	
	std::cout << "Starting session with file of size: " << fs::file_size(filePath) << std::endl;
	
	// Start the session
	// TODO: send meta data via this method.
	NymphStruct* ms = new NymphStruct;
	ms->addPair("filesize", new NymphUint32(fs::file_size(filePath)));
	values.clear();
	values.push_back(ms);
	if (!NymphRemoteServer::callMethod(handle, "session_start", values, returnValue, result)) {
		cout << "Error calling remote method: " << result << endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		return 1;
	}
	
	if (returnValue->type() != NYMPH_UINT8) {
		cout << "Return value wasn't a uint8. Type: " << returnValue->type() << endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		return 1;
	}
	
	// Wait for the condition to be signalled.
	mtx.lock();
	cnd.wait(mtx);
	
	cout << "Shutting down client...\n";
	
	// Shutdown.
	NymphRemoteServer::disconnect(handle, result);
	NymphRemoteServer::shutdown();
	return 0;
}
