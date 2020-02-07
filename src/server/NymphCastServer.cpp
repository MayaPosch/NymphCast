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
#include <chrono>
#include <filesystem> 		// C++17
#include <set>

namespace fs = std::filesystem;

#include "ffplay/ffplay.h"
#include "ffplay/types.h"
#include "screensaver.h"

#include <nymph/nymph.h>

#include "config_parser.h"
#include "sarge.h"

#include <Poco/Condition.h>
#include <Poco/Thread.h>
#include <Poco/URI.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>

using namespace Poco;

#include <angelscript.h>
#include <scriptstdstring/scriptstdstring.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptarray/scriptarray.h>

#include <angelscript/json/json.h>

#include "INIReader.h"


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


enum NymphCastAppLocation {
	NYMPHCAST_APP_LOCATION_LOCAL = 1,
	NYMPHCAST_APP_LOCATION_HTTP = 2
};


struct NymphCastApp {
	std::string id;
	
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
	castingUrl = false;
	
	// Check whether we have any queued URLs to stream next.
	media_buffer.streamTrackQueueMutex.lock();
	if (!media_buffer.streamTrackQueue.empty() && !playerStarted) {
		playerStarted = true;
		castUrl = media_buffer.streamTrackQueue.front();
		media_buffer.streamTrackQueue.pop();
		castingUrl = true;
		media_buffer.streamTrackQueueMutex.unlock();
		
		avThread.start(ffplay);
				
		return;
	}
	
	media_buffer.streamTrackQueueMutex.unlock();
	
	// Send message to client indicating that we're done.
	std::vector<NymphType*> values;
	std::string result;
	NymphBoolean* resVal = 0;
	if (!NymphRemoteClient::callCallback(media_buffer.activeSession, "MediaStopCallback", values, result)) {
		std::cerr << "Calling media stop callback failed: " << result << std::endl;
		return;
	}
	
	// Start the Screensaver here for now.
	if (!display_disable) {
		ScreenSaver::start(15);
	}
}


// --- ANGEL SCRIPT SECTION ---

// AngelScript globals
asIScriptEngine* engine = 0;
asIScriptContext* soundcloudContext = 0;
asIScriptFunction* soundcloudFunction = 0;
std::chrono::time_point<std::chrono::steady_clock> timeOut;

//typedef unsigned int DWORD;


// --- MESSAGE CALLBACK ---
// Angel Script runtime callback for messages.
void MessageCallback(const asSMessageInfo *msg, void *param) {
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING )
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION )
		type = "INFO";

	std::cout << msg->section << " (" << msg->row << ", " << msg->col << ") : " << type << " : " 
				<< msg->message << std::endl;
}


std::chrono::time_point<std::chrono::steady_clock> timeGetTime() {
	/* timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec*1000 + time.tv_usec/1000; */
	std::chrono::time_point<std::chrono::steady_clock> now;
	now = std::chrono::steady_clock::now();
	return now;
}


void LineCallback(asIScriptContext *ctx, std::chrono::time_point<std::chrono::steady_clock> *timeOut) {
	// If the time out is reached we abort the script
	if (*timeOut < timeGetTime()) {
		ctx->Abort();
	}

	// It would also be possible to only suspend the script,
	// instead of aborting it. That would allow the application
	// to resume the execution where it left of at a later 
	// time, by simply calling Execute() again.
}


// --- CLIENT SEND ---
// Send a message to a client by a NymphCast app.
void clientSend(uint32_t id, std::string message) {
	// Send a message to a client for an app, if the cliend ID exists.
	
	
	std::vector<NymphType*> values;
	std::string result;
	NymphBoolean* resVal = 0;
	if (!NymphRemoteClient::callCallback(media_buffer.activeSession, "ReceiveFromAppCallback", 
																				values, result)) {
		std::cerr << "Calling callback failed: " << result << std::endl;
		return;
	}
}


// --- PERFORM HTTP QUERY ---
bool performHttpQuery(std::string query, std::string &response) {
	// Create Poco HTTP query, send it off, wait for response.
	Poco::URI uri(query);
	std::string path(uri.getPathAndQuery());
	if (path.empty()) { path = "/"; }
	
	Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
	Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, 
											Poco::Net::HTTPMessage::HTTP_1_1);
	session.sendRequest(req);
	Poco::Net::HTTPResponse httpResponse;
    std::istream& rs = session.receiveResponse(httpResponse);
    std::cout << httpResponse.getStatus() << " " << httpResponse.getReason() << std::endl;
    if (httpResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
        Poco::StreamCopier::copyToString(rs, response);
        return true;
    }
    else {
        //it went wrong ?
        return false;
    }
	
	
	return true;
}


// --- PERFORM HTTPS QUERY ---
bool performHttpsQuery(std::string query, std::string &response) {
	// Create Poco HTTP query, send it off, wait for response.
	Poco::URI uri(query);
	std::string path(uri.getPathAndQuery());
	if (path.empty()) { path = "/"; }
	
	const Poco::Net::Context::Ptr context = new Poco::Net::Context(
        Poco::Net::Context::CLIENT_USE, "", "", "",
        Poco::Net::Context::VERIFY_NONE, 9, false,
        "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	
	Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
	Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, 
											Poco::Net::HTTPMessage::HTTP_1_1);
	session.sendRequest(req);
	Poco::Net::HTTPResponse httpResponse;
    std::istream& rs = session.receiveResponse(httpResponse);
    std::cout << httpResponse.getStatus() << " " << httpResponse.getReason() << std::endl;
    if (httpResponse.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
        Poco::StreamCopier::copyToString(rs, response);
		
		// Debug
		std::cout << "HTTP response:\n---------------\n";
		std::cout << response;
		std::cout << "\n---------------\n";
		
        return true;
    }
    else {
        //it went wrong ?
        return false;
    }
	
	
	return true;
}


// --- STREAM TRACK ---
// Attempt to stream from the indicated URL.
bool streamTrack(std::string url) {
	// TODO: Check that we're not still streaming, otherwise queue the URL.
	// TODO: allow to cancel any currently playing track/empty queue?
	if (playerStarted) {
		// Add to queue.
		media_buffer.streamTrackQueueMutex.lock();
		media_buffer.streamTrackQueue.push(url);
		media_buffer.streamTrackQueueMutex.unlock();
		
		return true;
	}
	
	castUrl = url;
	castingUrl = true;
	
	if (!playerStarted) {
		playerStarted = true;
		avThread.start(ffplay);
	}
	
	return true;
}


// --- ANGEL SCRIPT INIT ---
// Set up AngelScript runtime to allow installed NymphCast applications to be used.
void angelScriptInit() {
	//
	
	// Create the script engine
	engine = asCreateScriptEngine();
	if (engine == 0) {
		std::cout << "Failed to create script engine." << std::endl;
		return;
	}
	
	// The script compiler will write any compiler messages to the callback.
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
	
	// Register the script string type
	RegisterStdString(engine);
	RegisterScriptArray(engine, false);
	RegisterStdStringUtils(engine);
	
	// Register functions.
	int r;
	r = engine->RegisterGlobalFunction(
								"bool performHttpQuery(string, string &out)", 
								asFUNCTION(performHttpQuery), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"bool performHttpsQuery(string, string &out)", 
								asFUNCTION(performHttpsQuery), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"void clientSend(int, string)", 
								asFUNCTION(clientSend), asCALL_CDECL);
	r = engine->RegisterGlobalFunction(
								"bool streamTrack(string)", 
								asFUNCTION(streamTrack), asCALL_CDECL);
								
	// Register further modules.
	initJson(engine);
								
								
	// For the prototype, set up just the SoundCloud app module.
	
	
	// We will load the script from a file on the disk.
	/* FILE *f = fopen("script.as", "rb");
	if( f == 0 ) 	{
		std::cout << "Failed to open the script file 'script.as'." << std::endl;
		return -1;
	}

	// Determine the size of the file	
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);

	// On Win32 it is possible to do the following instead
	// int len = _filelength(_fileno(f));

	// Read the entire file
	string script;
	script.resize(len);
	size_t c = fread(&script[0], len, 1, f);
	fclose(f);

	if (c == 0) {
		std::cout << "Failed to load script file." << endl;
		return -1;
	}

	// Add the script sections that will be compiled into executable code.
	// If we want to combine more than one file into the same script, then 
	// we can call AddScriptSection() several times for the same module and
	// the script engine will treat them all as if they were one. The script
	// section name, will allow us to localize any errors in the script code.
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("script", &script[0], len);
	if (r < 0) {
		std::cout << "AddScriptSection() failed" << endl;
		return -1;
	}
	
	// Compile the script. If there are any compiler messages they will
	// be written to the message stream that we set right after creating the 
	// script engine. If there are no errors, and no warnings, nothing will
	// be written to the stream.
	r = mod->Build();
	if (r < 0) {
		std::cout << "Build() failed" << endl;
		return -1;
	} */
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
NymphMessage* connectClient(int session, NymphMessage* msg, void* data) {
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
	if (!video_disable) {
		ScreenSaver::stop();
	}
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// Client sends meta data for the track.
// Returns: OK (0), ERROR (1).
// int session_meta(string artist, string album, int track, string name)
NymphMessage* session_meta(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// X unused function.
	
	return returnMsg;
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


// --- VOLUME SET ---
// uint8 volume_set(uint8 volume)
NymphMessage* volume_set(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	uint8_t volume = ((NymphUint8*) msg->parameters()[0])->getValue();
	
	// TODO: figure out a way to set volume directly. Maybe in Player?
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- VOLUME UP ---
// uint8 volume_up()
NymphMessage* volume_up(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_0;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- VOLUME DOWN ---
// uint8 volume_down()
NymphMessage* volume_down(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_9;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- VOLUME MUTE ---
// uint8 volume_mute()
NymphMessage* volume_mute(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_m;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK START ---
// uint8 playback_start()
NymphMessage* playback_start(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_SPACE;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK STOP ---
// uint8 playback_stop()
NymphMessage* playback_stop(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_ESCAPE;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK PAUSE ---
// uint8 playback_pause()
NymphMessage* playback_pause(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = SDLK_SPACE;
	SDL_PushEvent(&event);
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK REWIND ---
// uint8 playback_rewind()
NymphMessage* playback_rewind(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK FORWARD ---
// uint8 playback_forward()
NymphMessage* playback_forward(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK SEEK ---
// uint8 playback_seek(uint64)
NymphMessage* playback_seek(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK URL ---
// uint8 playback_url(string)
NymphMessage* playback_url(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	castingUrl = true;
	castUrl = ((NymphString*) msg->parameters()[0])->getValue();
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// --- PLAYBACK STATUS ---
// struct playback_status()
NymphMessage* playback_status(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Set the playback status.
	NymphStruct* response = new NymphStruct;
	response->addPair("playing", new NymphBoolean(playerStarted));
	
	returnMsg->setResultValue(response);
	return returnMsg;
}


// --- APP LIST ---
// string app_list()
// Returns a list of registered apps, separated by a newline and ending with a newline.
NymphMessage* app_list(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Open the 'apps/apps.ini' file and parse it.
	INIReader apps("apps/apps.ini");
	if (apps.ParseError() != 1) {
		returnMsg->setResultValue(new NymphString());
		return returnMsg;
	}
	
	std::set<std::string> sections = apps.Sections();
	
	// We obtain and return the list of available apps here.
	// For now we use the list of folders in the apps/ folder. Each folder name is taken to be
	// the app name.
	/* fs::directory_iterator it = fs::directory_iterator("apps/");
	//std::vector<std::string> appnames;
	std::string names;
	while (it != fs::directory_iterator()) {
		if (fs::is_directory(it->path())) {
			//appnames.push_back(i->path().string());
			names.append(it->path().string());
			names.append("\n");
		}
	} */
	
	// Serialise the sections vector.
	std::string names;
	std::set<std::string>::const_iterator it = sections.cbegin();
	while (it != sections.cend()) {
		names.append(*it);
		names.append("\n");
		it++;
	}
	
	returnMsg->setResultValue(new NymphString(names));
	return returnMsg;
}


// --- APP SEND ---
// string app_send(string appId, string data)
NymphMessage* app_send(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Validate the application ID, try to find running instance, else launch new app instance.
	std::string appId = ((NymphString*) msg->parameters()[0])->getValue();
	std::string message = ((NymphString*) msg->parameters()[1])->getValue();
	
	// FIXME: hardcoding the SoundCloud app for prototype purposes.
	std::string result = "";
	if (appId == "soundcloud") {
		std::cout << "Found SoundCloud app." << std::endl;
		if (soundcloudFunction == 0) {
			// Initialise new instance of the SoundCloud app.
			int r;
			
			std::cout << "Loading SoundCloud app..." << std::endl;

			// We will load the script from a file on the disk.
			FILE *f = fopen("apps/soundcloud/soundcloud.as", "rb");
			if (f == 0) {
				std::cout << "Failed to open the script file 'apps/soundcloud/soundcloud.as'." << std::endl;
				//result = ;
			}

			// Determine the size of the file	
			fseek(f, 0, SEEK_END);
			int len = ftell(f);
			fseek(f, 0, SEEK_SET);

			// On Win32 it is possible to do the following instead
			// int len = _filelength(_fileno(f));

			// Read the entire file
			std::string script;
			script.resize(len);
			size_t c = fread(&script[0], len, 1, f);
			fclose(f);

			if (c == 0) {
				std::cout << "Failed to load script file." << std::endl;
				returnMsg->setResultValue(new NymphString(result));
				return returnMsg;
			}
			
			std::cout << "Creating module." << std::endl;

			// Add the script sections that will be compiled into executable code.
			// If we want to combine more than one file into the same script, then 
			// we can call AddScriptSection() several times for the same module and
			// the script engine will treat them all as if they were one. The script
			// section name, will allow us to localize any errors in the script code.
			asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
			r = mod->AddScriptSection("script", &script[0], len);
			if (r < 0) {
				std::cout << "AddScriptSection() failed" << std::endl;
				returnMsg->setResultValue(new NymphString(result));
				return returnMsg;
			}
			
			std::cout << "Compile script." << std::endl;
			
			// Compile the script. If there are any compiler messages they will
			// be written to the message stream that we set right after creating the 
			// script engine. If there are no errors, and no warnings, nothing will
			// be written to the stream.
			r = mod->Build();
			if (r < 0) {
				std::cout << "Build() failed" << std::endl;
				returnMsg->setResultValue(new NymphString(result));
				return returnMsg;
			}

			// The engine doesn't keep a copy of the script sections after Build() has
			// returned. So if the script needs to be recompiled, then all the script
			// sections must be added again.

			// If we want to have several scripts executing at different times but 
			// that have no direct relation with each other, then we can compile them
			// into separate script modules. Each module use their own namespace and 
			// scope, so function names, and global variables will not conflict with
			// each other.
			
			std::cout << "Creating context." << std::endl;
			
			// Create a context that will execute the script.
			soundcloudContext = engine->CreateContext();
			if (soundcloudContext == 0) {
				std::cout << "Failed to create the context." << std::endl;
				engine->Release();
				returnMsg->setResultValue(new NymphString(result));
				return returnMsg;
			}
			
			std::cout << "Setting line callback." << std::endl;

			// We don't want to allow the script to hang the application, e.g. with an
			// infinite loop, so we'll use the line callback function to set a timeout
			// that will abort the script after a certain time. Before executing the 
			// script the timeOut variable will be set to the time when the script must 
			// stop executing. 
			r = soundcloudContext->SetLineCallback(asFUNCTION(LineCallback), &timeOut, asCALL_CDECL);
			if (r < 0) {
				std::cout << "Failed to set the line callback function." << std::endl;
				soundcloudContext->Release();
				engine->Release();
				returnMsg->setResultValue(new NymphString(result));
				return returnMsg;
			}
			
			std::cout << "Find function." << std::endl;

			// Find the function for the function we want to execute.
			soundcloudFunction = engine->GetModule(0)->GetFunctionByDecl("string command_processor(string input)");
			if (soundcloudFunction == 0) {
				std::cout << "The function 'string command_processor(string input)' was not found." << std::endl;
				soundcloudContext->Release();
				engine->Release();
				returnMsg->setResultValue(new NymphString(result));
				return returnMsg;
			}
		}
		
		std::cout << "Preparing script context." << std::endl;
					
		// Prepare the script context with the function we wish to execute. Prepare()
		// must be called on the context before each new script function that will be
		// executed. Note, that if you intend to execute the same function several 
		// times, it might be a good idea to store the function returned by 
		// GetFunctionByDecl(), so that this relatively slow call can be skipped.
		int r = soundcloudContext->Prepare(soundcloudFunction);
		if (r < 0) {
			std::cout << "Failed to prepare the context." << std::endl;
			soundcloudContext->Release();
			engine->Release();
			returnMsg->setResultValue(new NymphString(result));
			return returnMsg;
		}
		
		std::cout << "Setting app arguments." << std::endl;
		
		// Pass string to app.
		soundcloudContext->SetArgObject(0, (void*) &message);
		
		// Set the timeout before executing the function. Give the function 3 seconds
		// to return before we'll abort it.
		timeOut = timeGetTime() + std::chrono::seconds(3);

		// Execute the function.
		std::cout << "Executing the script." << std::endl;
		std::cout << "---" << std::endl;
		r = soundcloudContext->Execute();
		std::cout << "---" << std::endl;
		if (r != asEXECUTION_FINISHED) {
			// The execution didn't finish as we had planned. Determine why.
			if (r == asEXECUTION_ABORTED) {
				std::cout << "The script was aborted before it could finish. Probably it timed out." 
							<< std::endl;
			}
			else if (r == asEXECUTION_EXCEPTION) {
				std::cout << "The script ended with an exception." << std::endl;

				// Write some information about the script exception
				asIScriptFunction* func = soundcloudContext->GetExceptionFunction();
				std::cout << "func: " << func->GetDeclaration() << std::endl;
				std::cout << "modl: " << func->GetModuleName() << std::endl;
				std::cout << "sect: " << func->GetScriptSectionName() << std::endl;
				std::cout << "line: " << soundcloudContext->GetExceptionLineNumber() << std::endl;
				std::cout << "desc: " << soundcloudContext->GetExceptionString() << std::endl;
			}
			else
				std::cout << "The script ended for some unforeseen reason (" << r << ")." 
							<< std::endl;
		}
		else {
			// Retrieve the return value from the context
			result = *(std::string*) soundcloudContext->GetReturnObject();
			std::cout << "The script function returned: " << result << std::endl;
		}
	}
	
	returnMsg->setResultValue(new NymphString(result));
	return returnMsg;
}


// --- LOG FUNCTION ---
void logFunction(int level, std::string logStr) {
	std::cout << level << " - " << logStr << std::endl;
}


int main(int argc, char** argv) {
	// Parse the command line arguments.
	Sarge sarge;
	sarge.setArgument("c", "configuration", "Path to configuration file.", true);
	sarge.parseArguments(argc, argv);
	
	std::string config_file;
	if (!sarge.getFlag("configuration", config_file)) {
		std::cerr << "No configuration file provided in command line arguments." << std::endl;
		return 1;
	}
	
	// Read in the configuration.
	IniParser config;
	if (!config.load(config_file)) {
		std::cerr << "Unable to load configuration file: " << config_file << std::endl;
		return 1;
	}
	
	is_full_screen = config.getValue<bool>("fullscreen", false);
	display_disable = config.getValue<bool>("disable_video", false);
	
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
	connectFunction.setCallback(connectClient);
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
	
	// Playback control methods.
	//
	// VolumeSet.
	// uint8 volume_set(uint8 volume)
	// Set volume to between 0 - 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	parameters.push_back(NYMPH_UINT8);
	NymphMethod volumeSetFunction("volume_set", parameters, NYMPH_UINT8);
	volumeSetFunction.setCallback(volume_set);
	NymphRemoteClient::registerMethod("volume_set", volumeSetFunction);
	
	// VolumeUp.
	// uint8 volume_up()
	// Increase volume by 10 up to 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	NymphMethod volumeUpFunction("volume_up", parameters, NYMPH_UINT8);
	volumeUpFunction.setCallback(volume_up);
	NymphRemoteClient::registerMethod("volume_up", volumeUpFunction);
		
	// VolumeDown.
	// uint8 volume_down()
	// Decrease volume by 10 up to 100.
	// Returns new volume setting or >100 if failed.
	parameters.clear();
	NymphMethod volumeDownFunction("volume_down", parameters, NYMPH_UINT8);
	volumeDownFunction.setCallback(volume_down);
	NymphRemoteClient::registerMethod("volume_down", volumeDownFunction);
	
	// PlaybackStart.
	// uint8 playback_start()
	// Start playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackStartFunction("playback_start", parameters, NYMPH_UINT8);
	playbackStartFunction.setCallback(playback_start);
	NymphRemoteClient::registerMethod("playback_start", playbackStartFunction);
	
	// PlaybackStop.
	// uint8 playback_stop()
	// Stop playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackStopFunction("playback_stop", parameters, NYMPH_UINT8);
	playbackStopFunction.setCallback(playback_stop);
	NymphRemoteClient::registerMethod("playback_stop", playbackStopFunction);
	
	// PlaybackPause.
	// uint8 playback_pause()
	// Start playback.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackPauseFunction("playback_pause", parameters, NYMPH_UINT8);
	playbackPauseFunction.setCallback(playback_pause);
	NymphRemoteClient::registerMethod("playback_pause", playbackPauseFunction);
	
	// PlaybackRewind.
	// uint8 playback_rewind()
	// Rewind the current file to the beginning.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackRewindFunction("playback_rewind", parameters, NYMPH_UINT8);
	playbackRewindFunction.setCallback(playback_rewind);
	NymphRemoteClient::registerMethod("playback_rewind", playbackRewindFunction);
	
	// PlaybackForward
	// uint8 playback_forward()
	// Forward the current file to the end.
	// Returns success or error number.
	parameters.clear();
	NymphMethod playbackForwardFunction("playback_forward", parameters, NYMPH_UINT8);
	playbackForwardFunction.setCallback(playback_forward);
	NymphRemoteClient::registerMethod("playback_forward", playbackForwardFunction);
	
	// PlaybackSeek
	// uint8 playback_seek(uint64)
	// Seek to the indicated position.
	// Returns success or error number.
	parameters.clear();
	parameters.push_back(NYMPH_UINT64);
	NymphMethod playbackSeekFunction("playback_seek", parameters, NYMPH_UINT8);
	playbackSeekFunction.setCallback(playback_seek);
	NymphRemoteClient::registerMethod("playback_seek", playbackSeekFunction);
	
	// PlaybackUrl.
	// uint8 playback_url(string)
	// Try to the play the media file indicated by the provided URL.
	// Returns success or error number.
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	NymphMethod playbackUrlFunction("playback_url", parameters, NYMPH_UINT8);
	playbackUrlFunction.setCallback(playback_url);
	NymphRemoteClient::registerMethod("playback_url", playbackUrlFunction);
	
	// PlaybackStatus
	// struct playback_status()
	// The current state of the NymphCast server.
	// Return struct with information:
	// ["playing"] => boolean (true/false)
	// 
	parameters.clear();
	NymphMethod playbackStatusFunction("playback_status", parameters, NYMPH_STRUCT);
	playbackStatusFunction.setCallback(playback_status);
	NymphRemoteClient::registerMethod("playback_status", playbackStatusFunction);
	
	
	// ReceiverStatus.
	// 
	
	
	// AppList
	// string app_list()
	// Returns a list of installed applications.
	parameters.clear();
	NymphMethod appListFunction("app_list", parameters, NYMPH_STRING);
	appListFunction.setCallback(app_list);
	NymphRemoteClient::registerMethod("app_list", appListFunction);	
	
	// AppSend
	// string app_send(uint32 appId, string data)
	// Allows a client to send data to a NymphCast application.
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	NymphMethod appSendFunction("app_send", parameters, NYMPH_STRING);
	appSendFunction.setCallback(app_send);
	NymphRemoteClient::registerMethod("app_send", appSendFunction);	
	
	
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
	
	// MediaSeekCallback
	// Sends the desired byte position in the open file to seek to.
	// void MediaSeekCallback(uint64)
	parameters.clear();
	parameters.push_back(NYMPH_UINT64);
	NymphMethod mediaSeekCallback("MediaSeekCallback", parameters, NYMPH_NULL);
	mediaSeekCallback.enableCallback();
	NymphRemoteClient::registerCallback("MediaSeekCallback", mediaSeekCallback);
	
	// ReceiveFromAppCallback
	// Sends message from a NymphCast app to the client.
	// void ReceiveFromAppCallback(string appId, string message)
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	NymphMethod receiveFromAppCallback("ReceiveFromAppCallback", parameters, NYMPH_NULL);
	receiveFromAppCallback.enableCallback();
	NymphRemoteClient::registerCallback("ReceiveFromAppCallback", receiveFromAppCallback);
	
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
	// Transition time is 15 seconds.
	if (!display_disable) {
		ScreenSaver::start(15);
	}
	
	// Initialise AngelScript runtime.
	angelScriptInit();
	
	
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
