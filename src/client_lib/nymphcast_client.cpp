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


void NymphCastClient::ReceiveFromAppCallback(uint32_t session, NymphMessage* msg, void* data) {
	std::string appId = ((NymphString*) msg->parameters()[0])->getValue();
	std::string message = ((NymphString*) msg->parameters()[1])->getValue();
	
	if (appMessageFunction) {
		appMessageFunction(appId, message);
	}
}


// --- CONSTRUCTOR ---
NymphCastClient::NymphCastClient() {
	//
	
	// Initialise the remote client instance.
	long timeout = 60000; // 60 seconds.
	NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	appMessageFunction = 0;
}


// --- DESTRUCTOR ---
NymphCastClient::~NymphCastClient() {
	//
	NymphRemoteServer::shutdown();
}


// --- SET APPLICATION CALLBACK ---
void NymphCastClient::setApplicationCallback(AppMessageFunction function) {
	appMessageFunction = function;
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
		//NymphRemoteServer::disconnect(handle, result);
		return std::string();
	}
	
	if (returnValue->type() != NYMPH_STRING) {
		std::cout << "Return value wasn't a string. Type: " << returnValue->type() << std::endl;
		//NymphRemoteServer::disconnect(handle, result);
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
		//NymphRemoteServer::disconnect(handle, result);
		return std::string();
	}
	
	if (returnValue->type() != NYMPH_STRING) {
		std::cout << "Return value wasn't a string. Type: " << returnValue->type() << std::endl;
		//NymphRemoteServer::disconnect(handle, result);
		return std::string();
	}
	
	return ((NymphString*) returnValue)->getValue();
}


// --- mDNS disaster zone start ---
/* #include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#  define sleep(x) Sleep(x * 1000)
#else
#  include <netdb.h>
#endif

static char addrbuffer[64];
static char namebuffer[256];
static mdns_record_txt_t txtbuffer[128];

static mdns_string_t ipv4_address_to_string(char* buffer, size_t capacity, 
												const struct sockaddr_in* addr) {
	char host[NI_MAXHOST] = {0};
	char service[NI_MAXSERV] = {0};
	int ret = getnameinfo((const struct sockaddr*)addr, sizeof(struct sockaddr_in),
	                      host, NI_MAXHOST, service, NI_MAXSERV,
	                      NI_NUMERICSERV | NI_NUMERICHOST);
	int len = 0;
	if (ret == 0) {
		if (addr->sin_port != 0)
			len = snprintf(buffer, capacity, "%s:%s", host, service);
		else
			len = snprintf(buffer, capacity, "%s", host);
	}
	if (len >= (int)capacity)
		len = (int)capacity - 1;
	mdns_string_t str = {buffer, len};
	return str;
}

static mdns_string_t ipv6_address_to_string(char* buffer, size_t capacity, 
													const struct sockaddr_in6* addr) {
	char host[NI_MAXHOST] = {0};
	char service[NI_MAXSERV] = {0};
	int ret = getnameinfo((const struct sockaddr*)addr, sizeof(struct sockaddr_in6),
	                      host, NI_MAXHOST, service, NI_MAXSERV,
	                      NI_NUMERICSERV | NI_NUMERICHOST);
	int len = 0;
	if (ret == 0) {
		if (addr->sin6_port != 0)
			len = snprintf(buffer, capacity, "[%s]:%s", host, service);
		else
			len = snprintf(buffer, capacity, "%s", host);
	}
	if (len >= (int)capacity)
		len = (int)capacity - 1;
	mdns_string_t str = {buffer, len};
	return str;
}

static mdns_string_t ip_address_to_string(char* buffer, size_t capacity, 
												const struct sockaddr* addr) {
	if (addr->sa_family == AF_INET6)
		return ipv6_address_to_string(buffer, capacity, (const struct sockaddr_in6*)addr);
	return ipv4_address_to_string(buffer, capacity, (const struct sockaddr_in*)addr);
}

static int callback(const struct sockaddr* from, 
						mdns_entry_type_t entry, uint16_t type,
						uint16_t rclass, uint32_t ttl,
						const void* data, size_t size, size_t offset, size_t length,
						void* user_data) {
	mdns_string_t fromaddrstr = ip_address_to_string(addrbuffer, sizeof(addrbuffer), from);
	const char* entrytype = (entry == MDNS_ENTRYTYPE_ANSWER) ? "answer" :
	                        ((entry == MDNS_ENTRYTYPE_AUTHORITY) ? "authority" : "additional");
	if (type == MDNS_RECORDTYPE_PTR) {
		mdns_string_t namestr = mdns_record_parse_ptr(data, size, offset, length,
		                                              namebuffer, sizeof(namebuffer));
		printf("%.*s : %s PTR %.*s type %u rclass 0x%x ttl %u length %d\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       MDNS_STRING_FORMAT(namestr), type, rclass, ttl, (int)length);
	}
	else if (type == MDNS_RECORDTYPE_SRV) {
		mdns_record_srv_t srv = mdns_record_parse_srv(data, size, offset, length,
		                                              namebuffer, sizeof(namebuffer));
		printf("%.*s : %s SRV %.*s priority %d weight %d port %d\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       MDNS_STRING_FORMAT(srv.name), srv.priority, srv.weight, srv.port);
	}
	else if (type == MDNS_RECORDTYPE_A) {
		struct sockaddr_in addr;
		mdns_record_parse_a(data, size, offset, length, &addr);
		mdns_string_t addrstr = ipv4_address_to_string(namebuffer, sizeof(namebuffer), &addr);
		printf("%.*s : %s A %.*s\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       MDNS_STRING_FORMAT(addrstr));
	}
	else if (type == MDNS_RECORDTYPE_AAAA) {
		struct sockaddr_in6 addr;
		mdns_record_parse_aaaa(data, size, offset, length, &addr);
		mdns_string_t addrstr = ipv6_address_to_string(namebuffer, sizeof(namebuffer), &addr);
		printf("%.*s : %s AAAA %.*s\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       MDNS_STRING_FORMAT(addrstr));
	}
	else if (type == MDNS_RECORDTYPE_TXT) {
		size_t parsed = mdns_record_parse_txt(data, size, offset, length,
		                                      txtbuffer, sizeof(txtbuffer) / sizeof(mdns_record_txt_t));
		for (size_t itxt = 0; itxt < parsed; ++itxt) {
			if (txtbuffer[itxt].value.length) {
				printf("%.*s : %s TXT %.*s = %.*s\n",
				       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
				       MDNS_STRING_FORMAT(txtbuffer[itxt].key),
				       MDNS_STRING_FORMAT(txtbuffer[itxt].value));
			}
			else {
				printf("%.*s : %s TXT %.*s\n",
				       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
				       MDNS_STRING_FORMAT(txtbuffer[itxt].key));
			}
		}
	}
	else {
		printf("%.*s : %s type %u rclass 0x%x ttl %u length %d\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       type, rclass, ttl, (int)length);
	}
	return 0;
} */

// --- mDNS disaster zone end ---


// --- FIND SERVERS ---
void NymphCastClient::findServers() {
	// Perform an mDNS/DNS-SD service discovery run for NymphCast receivers.
	
	// Open socket.
	/* int sock = mdns_socket_open_ipv4();
	if (sock < 0) {
		printf("Failed to open socket: %s\n", strerror(errno));
		return -1;
	}
	
	// Send DNS-SD query.
	if (mdns_discovery_send(sock)) {
		printf("Failed to send DNS-DS discovery: %s\n", strerror(errno));
		goto quit;
	}
	
	// Read DNS-SD replies.
	size_t capacity = 2048;
	void* buffer = 0;
	void* user_data = 0;
	buffer = malloc(capacity);
	for (int i = 0; i < 10; ++i) {
		records = mdns_discovery_recv(sock, buffer, capacity, callback, user_data);
		sleep(1);
	}
	
	// Get details for specific record.
	if (mdns_query_send(sock, MDNS_RECORDTYPE_PTR,
	                    MDNS_STRING_CONST("_ssh._tcp.local."),
	                    buffer, capacity)) {
		printf("Failed to send mDNS query: %s\n", strerror(errno));
		goto quit;
	}
	
	// Read the mDNS replies.
	for (int i = 0; i < 10; ++i) {
		records = mdns_query_recv(sock, buffer, capacity, callback, user_data, 1);
		sleep(1);
	}
	
	// Close the socket.
	free(buffer);
	mdns_socket_close(sock); */
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
	// uint8 playback_url(string)
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	values.push_back(new NymphString(url));
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


// --- VOLUME SET ---
uint8_t NymphCastClient::volumeSet(uint32_t handle, uint8_t volume) {
	//
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
	//
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
	//
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
	//
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
	//
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
	//
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
	//
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
	//
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
uint8_t NymphCastClient::playbackSeek(uint32_t handle, uint64_t location) {
	//
	// uint8 playback_seek(uint64)
	std::vector<NymphType*> values;
	std::string result;
	NymphType* returnValue = 0;
	values.push_back(new NymphUint64(location));
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
