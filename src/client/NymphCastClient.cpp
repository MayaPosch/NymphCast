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
#include <vector>

using namespace std;

/* #include <Poco/Condition.h>

using namespace Poco; */


// Globals
/* Condition cnd;
Mutex mtx; */
// ---


void logFunction(int level, string logStr) {
	cout << level << " - " << logStr << endl;
}


// Callback to register with the server. 
// This callback will be called once by the server and then discarded. This is
// useful for one-off events, but can also be used for callbacks during the 
// life-time of the client.
/* void callbackFunction(NymphMessage* msg, void* data) {
	cout << "Client callback function called.\n";
	
	// Remove the callback.
	NymphRemoteServer::removeCallback("helloCallbackFunction");
	
	// Signal the condition variable.
	//cnd.signal();
} */


int main() {
	// Initialise the remote client instance.
	long timeout = 5000; // 5 seconds.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	// Connect to the remote server.
	int handle;
	string result;
	if (!NymphRemoteServer::connect("127.0.0.1", 4004, handle, 0, result)) {
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
	
	//string response = ((NymphString*) returnValue)->getValue();
	
	//cout << "Response string: " << response << endl;
	
	delete returnValue;
	returnValue = 0;
	
	// Register callback and send message with its ID to the server. Then wait
	// for the callback to be called.
	/* NymphRemoteServer::registerCallback("callbackFunction", callbackFunction, 0);
	values.clear();
	values.push_back(new NymphString("callbackFunction"));
	if (!NymphRemoteServer::callMethod(handle, "helloCallbackFunction", values, returnValue, result)) {
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
	
	if (!(((NymphBoolean*) returnValue)->getValue())) {
		cout << "Remote method returned false. " << result << endl;
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		return 1;
	}
	
	delete returnValue;
	returnValue = 0; */
	
	// Wait for the callback method to be called on the client. We wait for
	// 5 seconds or until signalled, whichever comes first.
	/* mtx.lock();
	cnd.tryWait(mtx, 5000);
	mtx.unlock(); */
	
	cout << "Shutting down client...\n";
	
	// Shutdown.
	NymphRemoteServer::disconnect(handle, result);
	NymphRemoteServer::shutdown();
	return 0;
}
