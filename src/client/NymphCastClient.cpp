/*
	NymphCastClient.cpp - Client for creating sessions with a NymphCast server.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
		
	2019/01/25, Maya Posch
*/


//#include <nymph/nymph.h>
#include "../client_lib/nymphcast_client.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem> 		// C++17
#include <csignal>

namespace fs = std::filesystem;

using namespace std;

#include <Poco/Condition.h>

using namespace Poco;

#include "sarge.h"


// Globals
Condition cnd;
Mutex mtx;

uint32_t handle;		// NymphRPC handle.
std::ifstream source;
// ---


/* void logFunction(int level, string logStr) {
	cout << level << " - " << logStr << endl;
} */


void signal_handler(int signal) {
	cnd.signal();
}


/* // Callback to register with the server. 
// This callback will be called once by the server and then discarded. This is
// useful for one-off events, but can also be used for callbacks during the 
// life-time of the client.
void MediaReadCallback(uint32_t session, NymphMessage* msg, void* data) {
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


void MediaStopCallback(uint32_t session, NymphMessage* msg, void* data) {
	std::cout << "Client callback function called.\n";
	
	// Remove the callbacks.
	NymphRemoteServer::removeCallback("MediaReadCallback");
	NymphRemoteServer::removeCallback("MediaStopCallback");
	NymphRemoteServer::removeCallback("MediaSeekCallback");
	
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


void MediaSeekCallback(uint32_t session, NymphMessage* msg, void* data) {
	//
} */


int main(int argc, char *argv[]) {	
	// Parse the command line arguments.
	Sarge sarge;
	sarge.setArgument("h", "help", "Get this help message.", false);
	sarge.setArgument("v", "version", "Output the NymphCast client version and exit.", false);
	sarge.setArgument("f", "file", "Name of file to stream to remote receiver.", true);
	sarge.setArgument("i", "ip", "IP address of the target NymphCast receiver.", true);
	sarge.setDescription("NymphCast client application. For use with NymphCast servers. More details: http://nyanko.ws/nymphcast.php.");
	sarge.setUsage("nymphcast_client <options>");
	
	sarge.parseArguments(argc, argv);
	
	if (sarge.flagCount() == 0) {
		sarge.printHelp();
		return 0;
	}
	
	if (sarge.exists("help")) {
		sarge.printHelp();
		return 0;
	}
	
	if (sarge.exists("version")) {
		std::cout << "NymphCast client version: " << __VERSION << std::endl;
		return 0;
	}
	
	// Allow the IP address of the server to be passed on the command line.	
	// Try to open the file.	
	std::string filename;
	std::string serverip = "127.0.0.1";
	sarge.getFlag("file", filename);
	sarge.getFlag("ip", serverip);
	
	std::cout << "Opening file " << filename << std::endl;
	
	// Install signal handler to terminate the client application.
	signal(SIGINT, signal_handler);
	
	// TODO: set unique client name.
	
	// Set up client library.
	NymphCastClient client;
	uint32_t handle = 0;
	if (!client.connectServer(serverip, handle)) {
		std::cerr << "Failed to connect to server..." << std::endl;
		return 1;
	}
	
	// Send file.
	client.castFile(handle, filename);
	
	
	std::cout << "Press Ctrl+c to quit." << std::endl;
	
	/* fs::path filePath(filename);
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
	long timeout = 60000; // 60 seconds.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	// Connect to the remote server.
	std::string result;
	if (!NymphRemoteServer::connect(serverip, 4004, handle, 0, result)) {
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
	returnValue = 0; */
	
	
	// The remote NymphCast server works in a pull fashion, which means that we have to register
	// a callback with the server. This callback will be called whenever the server needs more
	// data from the file which we are streaming.
		
	// Register callback and send message with its ID to the server. Then wait
	// for the callback to be called.
	/* NymphRemoteServer::registerCallback("MediaReadCallback", MediaReadCallback, 0);
	NymphRemoteServer::registerCallback("MediaStopCallback", MediaStopCallback, 0);
	NymphRemoteServer::registerCallback("MediaSeekCallback", MediaSeekCallback, 0);
	
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
	} */
	
	// Wait for the condition to be signalled.
	mtx.lock();
	cnd.wait(mtx);
	
	cout << "Shutting down client...\n";
	
	// Shutdown.
	/* NymphRemoteServer::disconnect(handle, result);
	NymphRemoteServer::shutdown(); */
	
	client.disconnectServer(handle);
	
	return 0;
}
