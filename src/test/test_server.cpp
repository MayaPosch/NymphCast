/*
	test_server.cpp - Test server implementation.
	
*/


#include <iostream>

#include <nymph/nymph.h>


// Callback for the connect function.
NymphMessage* connectClient(int session, NymphMessage* msg, void* data) {
	std::cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << "\n";
	
	std::string clientStr = ((NymphString*) msg->parameters()[0])->getValue();
	std::cout << "Client string: " << clientStr << "\n";
	
	// Register this client with its ID. Return error if the client ID already exists.
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	NymphBoolean* retVal = 0;
	retVal = new NymphBoolean(true);
	
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


// Client disconnects from server.
// bool disconnect()
NymphMessage* disconnect(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	returnMsg->setResultValue(new NymphBoolean(true));
	return returnMsg;
}


// Client starts a session.
// Return value: OK (0), ERROR (1).
// int session_start(struct fileInfo)
NymphMessage* session_start(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Obtain the filesize from the client, which we use with the buffer management.
	NymphStruct* fileInfo = ((NymphStruct*) msg->parameters()[0]);
	NymphType* num = 0;
	if (!fileInfo->getValue("filesize", num)) {
		returnMsg->setResultValue(new NymphUint8(1));
		return returnMsg;
	}
	
	it->second.filesize = ((NymphUint32*) num)->getValue();
	
	std::cout << "Starting new session for file with size: " << it->second.filesize << std::endl;
	
	// Start transferring data.
	
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// Client sends a chunk of track data.
// Returns: OK (0), ERROR (1).
// int session_data(string buffer, boolean done)
NymphMessage* session_data(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// Client ends the session.
// Returns: OK (0), ERROR (1).
// int session_end()
NymphMessage* session_end(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- LOG FUNCTION ---
void logFunction(int level, std::string logStr) {
	std::cout << level << " - " << logStr << std::endl;
}


int main() {
	// Handle command-line input.
	// The test configuration for the test server is setup using global variables, which are
	// defined here.
	Sarge sarge;
	sarge.setArgument("h", "help", "Get this help message.", false);
	sarge.setArgument("v", "version", "Output the NymphCast server version and exit.", false);
	sarge.setArgument("f", "file", "Name of file to stream to remote receiver.", true);
	sarge.setArgument("i", "ip", "IP address of the target NymphCast receiver.", true);
	sarge.setDescription("NymphCast test server. For testing NymphCast functions.");
	sarge.setUsage("test_server <options>");
	
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
	
	
	std::cout << "Starting test server..." << std::endl;
	
	// Initialise the server.
	long timeout = 5000; // 5 seconds.
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	// Define all of the RPC methods we want to export for clients.
	std::cout << "Registering methods...\n";
	std::vector<NymphTypes> parameters;
	
	// Client connects to server.
	// bool connect(string client_id)
	parameters.push_back(NYMPH_STRING);
	NymphMethod connectFunction("connect", parameters, NYMPH_BOOL);
	connectFunction.setCallback(connectClient);
	NymphRemoteClient::registerMethod("connect", connectFunction);
	
	// Client disconnects from server.
	// bool disconnect()
	parameters.clear();
	NymphMethod disconnectFunction("disconnect", parameters, NYMPH_BOOL);
	disconnectFunction.setCallback(disconnect);
	NymphRemoteClient::registerMethod("disconnect", disconnectFunction);
	
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
	
	
	// Install signal handler to terminate the server.
	signal(SIGINT, signal_handler);
	
	// Start server on port 4004.
	NymphRemoteClient::start(4004);
	
	
	// Wait for the condition to be signalled.
	gMutex.lock();
	gCon.wait(gMutex);
	
	NymphRemoteClient::shutdown();
	
	return 0;
}
