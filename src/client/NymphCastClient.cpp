/*
	NymphCastClient.cpp - Client for creating sessions with a NymphCast server.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
		
	2019/01/25, Maya Posch
*/


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


void signal_handler(int signal) {
	cnd.signal();
}


int main(int argc, char *argv[]) {	
	// Parse the command line arguments.
	Sarge sarge;
	sarge.setArgument("h", "help", "Get this help message.", false);
	sarge.setArgument("v", "version", "Output the NymphCast client version and exit.", false);
	sarge.setArgument("r", "remotes", "Display online NymphCast receivers and quit.", false);
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
	
	// We need access to the NymphCast client library from this point.
	NymphCastClient client;
	
	if (sarge.exists("remotes")) {
		std::cout << "Scanning for online remote receivers..." << std::endl;
		
		std::vector remotes = client.findServers();
		if (remotes.size() < 1) {
			std::cout << "No remotes found." << std::endl;
			return 0;
		}
		
		// Print out list of remotes.
		std::cout << "Found remotes:" << std::endl;
		for (int i = 0; i < remotes.size(); ++i) {
			std::cout << i << ". " << remotes[i].name << " <" << remotes[i].ipv4 << ":" 
									<< remotes[i].port << "> (" << remotes[i].ipv6 << ")" 
									<< std::endl;
		}
		
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
	uint32_t handle = 0;
	if (!client.connectServer(serverip, handle)) {
		std::cerr << "Failed to connect to server..." << std::endl;
		return 1;
	}
	
	// Send file.
	client.castFile(handle, filename);
	
	
	std::cout << "Press Ctrl+c to quit." << std::endl;
	
	// Wait for the condition to be signalled.
	mtx.lock();
	cnd.wait(mtx);
	
	cout << "Shutting down client...\n";
	
	client.disconnectServer(handle);
	
	return 0;
}
