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

#include <SDL2/SDL.h>
#include <SDL2/SDL_mutex.h>
#include <vlc/vlc.h>
#include <nymph/nymph.h>

#include <Poco/Condition.h>
#include <Poco/Thread.h>

using namespace Poco;


#define SAMPLE_RATE (44100)

#define WIDTH 640
#define HEIGHT 480
 
#define VIDEOWIDTH 320
#define VIDEOHEIGHT 240
 
struct context {
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_mutex *mutex;
	int n;
};


// Global objects.
Condition gCon;
Mutex gMutex;
// ---


// Debug
#include <fstream>
std::ofstream OUT_FILE("amv_out.mpg", std::ios::binary | std::ios::app);


 
// VLC prepares to render a video frame.
static void *lock(void *data, void **p_pixels) {
 
	struct context *c = (context *)data;
 
	int pitch;
	SDL_LockMutex(c->mutex);
	SDL_LockTexture(c->texture, NULL, p_pixels, &pitch);
 
	return NULL; // Picture identifier, not needed here.
}
 
// VLC just rendered a video frame.
static void unlock(void *data, void *id, void *const *p_pixels) {
 
	struct context *c = (context *)data;
 
	SDL_UnlockTexture(c->texture);
	SDL_UnlockMutex(c->mutex);
}
 
// VLC wants to display a video frame.
static void display(void *data, void *id) {
 
	struct context *c = (context *)data;
 
	SDL_Rect rect;
	rect.w = WIDTH;
	rect.h = HEIGHT;
	rect.x = 0;
	rect.y = 0;
 
	SDL_SetRenderDrawColor(c->renderer, 0, 80, 0, 255);
	SDL_RenderClear(c->renderer);
	SDL_RenderCopy(c->renderer, c->texture, NULL, &rect);
	SDL_RenderPresent(c->renderer);
}
 
static void quit(int c) {
	SDL_Quit();
	exit(c);
}

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


struct DataBuffer {
	std::vector<std::string> data;	// The data byte, inside string instances.
	uint64_t size;				// Number of bytes in data vector.
	uint64_t currentIndex;		// The current index into the vector element.
	uint32_t currentSlot;		// The current vector slot we're using.
	uint32_t slotSize;			// Slot size in bytes.
	uint32_t slotBytesLeft;		// Bytes left in the current slot.
	uint32_t numSlots;			// Total number of slots in the data vector.
	uint32_t nextSlot;			// Next slot to fill in the buffer vector.
	uint32_t freeSlots;			// Slots free to write new data into.
	uint32_t buffBytesLeft;		// Number of bytes available for reading in the buffer.
	bool eof;					// Whether End of File for the source file has been reached.
	
	uint64_t buffIndexLow;		// File index at the buffer front.
	uint64_t buffIndexHigh;		// File index at the buffer back.
};


// --- Globals ---
libvlc_media_player_t* mp;
DataBuffer media_buffer;
Poco::Mutex bufferMutex;
Poco::Condition requestCondition;
Poco::Mutex requestMutex;
int activeSession;
Poco::Mutex activeSessionMutex;
bool playerStarted;
bool requestInFlight;
// ---


// --- DATA REQUEST FUNCTION ---
// This function can be signalled with the condition variable to request data from the client.
void dataRequestFunction() {
	while (1) {
		// Wait for the condition to be signalled.
		requestMutex.lock();
		requestCondition.wait(requestMutex);
		
		if (requestInFlight) { continue; }
		
		activeSessionMutex.lock();
		int session = activeSession;
		activeSessionMutex.unlock();
		
		// Request more data.
		// TODO: Initial buffer size is 1 MB. Make this dynamically scale.
		std::vector<NymphType*> values;
		std::string result;
		NymphBoolean* resVal = 0;
		if (!NymphRemoteClient::callCallback(session, "MediaReadCallback", values, result)) {
			std::cerr << "Calling callback failed: " << result << std::endl;
			return;
		}
		
		requestInFlight = true;
	}
}


// --- END SESSION FUNCTION ---
//
/* void endSessionFunction() {
	// Wait for the condition to be signalled.
	requestMutex.lock();
	requestCondition.wait(requestMutex);
	
	activeSessionMutex.lock();
	int session = activeSession;
	activeSessionMutex.unlock();
		
	std::vector<NymphType*> values;
	std::string result;
	NymphBoolean* resVal = 0;
	if (!NymphRemoteClient::callCallback(session, "MediaStopCallback", values, result)) {
		std::cerr << "Calling stop callback failed: " << result << std::endl;
	}
	else {
		it->second.sessionActive = false;
	}
} */


// LibVLC callbacks.
// --- MEDIA OPEN CB ---
// 0 on success, non-zero on error. In case of failure, the other callbacks will not be invoked 
// and any value stored in *datap and *sizep is discarded.
int media_open_cb(void *opaque, void **datap, uint64_t *sizep) {
	// Debug
	std::cout << "Media open callback called." << std::endl;
	
	DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
    *sizep = db->data.size();
    *datap = opaque;
    return 0;
}


// --- MEDIA READ CB ---
ssize_t media_read_cb(void *opaque, unsigned char *buf, size_t len) {
	DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
	// Fill the buffer from the memory buffer.
	// Return the read length.
	// TODO: account for buffer underrun.
	int bytesToCopy = 0;

	bufferMutex.lock();
	if (db->buffBytesLeft >= len) {  	// At least as many bytes remaining as requested
		bytesToCopy = len;
	} 
	else if (db->buffBytesLeft < len) {	// Fewer than requested number of bytes remaining
		bytesToCopy = db->buffBytesLeft;
	} 
	else {
		bufferMutex.unlock();
		return 0;   // No bytes left to copy
	}

	// Each slot has a limited depth. Check that we can copy the whole requested buffer
	// from a single slot, only updating the index into that slot.
	// Else, copy what is left in the current slot into the buffer, then copy the rest from the
	// next slot.
	if (db->slotBytesLeft < bytesToCopy) {
		uint32_t secondBytes = bytesToCopy - db->slotBytesLeft;
		uint32_t bufOffset = db->slotBytesLeft;
		
		// Copy the rest of the bytes in the slot, move onto the next slot.
		std::copy(db->data[db->currentSlot].begin() + db->currentIndex, 
				(db->data[db->currentSlot].begin() + db->currentIndex) + db->slotBytesLeft, 
				buf);
		
		db->currentSlot++;
		if (!(db->currentSlot < db->numSlots)) { db->currentSlot = 0; }
		db->slotSize = db->data[db->currentSlot].length();
		db->currentIndex = 0;
		db->slotBytesLeft = db->slotSize;
		db->freeSlots++; // The used buffer slot just became available for more data.
		
		std::copy(db->data[db->currentSlot].begin() + db->currentIndex, 
				(db->data[db->currentSlot].begin() + db->currentIndex) + secondBytes, 
				(buf + bufOffset));
				
		db->currentIndex += secondBytes;
		db->slotBytesLeft -= secondBytes;
	}
	else {
		// Debug
		std::cout << "Index: " << db->currentIndex << "/" << db->slotSize 
					<< ", Copy: " << bytesToCopy << "/" << db->slotBytesLeft << std::endl;
		
		// Just copy the bytes from the slot, adjusting the index and bytes left count.
		std::copy(db->data[db->currentSlot].begin() + db->currentIndex, 
				(db->data[db->currentSlot].begin() + db->currentIndex) + bytesToCopy, 
				buf);
		db->currentIndex += bytesToCopy;    // Increment bytes read count
		db->slotBytesLeft -= bytesToCopy;
	}
	
	db->buffBytesLeft -= bytesToCopy;
	
	// If there are free slots in the buffer, request more data from the client.
	if (!requestInFlight && !(db->eof) && db->freeSlots > 0) {
		requestCondition.signal();
	}
	
	bufferMutex.unlock();
	
	// Debug
	OUT_FILE.write((const char*) buf, bytesToCopy);

	return bytesToCopy;
}

int media_seek_cb(void* opaque, uint64_t offset) {
	DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
	// Try to find the index in the buffered data. If unavailable, read from file.
	//db->index = offset;
	
	// TODO: implement.
	
	return 0;
}

void media_close_cb(void *opaque) {
	// 
	activeSessionMutex.lock();
	int session = activeSession;
	activeSessionMutex.unlock();
		
	std::vector<NymphType*> values;
	std::string result;
	NymphBoolean* resVal = 0;
	if (!NymphRemoteClient::callCallback(session, "MediaStopCallback", values, result)) {
		std::cerr << "Calling stop callback failed: " << result << std::endl;
	}
	/* else {
		it->second.sessionActive = false;
	} */
	
	// Debug
	OUT_FILE.close();
}
// End LibVLC callbacks.


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
NymphMessage* connect(int session, NymphMessage* msg, void* data) {
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
	
	activeSessionMutex.lock();
	activeSession = session;
	activeSessionMutex.unlock();
	
	// Start calling the client's read callback method to obtain data. Once the data buffer
	// has been filled sufficiently, start the playback.
	// TODO: Initial buffer size is 1 MB. Make this dynamically scale.
	requestInFlight = false;
	requestCondition.signal();
	it->second.sessionActive = true;
	
	returnMsg->setResultValue(new NymphUint8(0));
	return returnMsg;
}


// Client sends meta data for the track.
// Returns: OK (0), ERROR (1).
// int session_meta(string artist, string album, int track, string name)
NymphMessage* session_meta(int session, NymphMessage* msg, void* data) {
	// X unused function.
}


// Client sends a chunk of track data.
// Returns: OK (0), ERROR (1).
// int session_data(string buffer, boolean done)
NymphMessage* session_data(int session, NymphMessage* msg, void* data) {
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// TODO: if this boolean is false already, dismiss message?
	requestInFlight = false;
	
	// Get iterator to the session instance for the client.
	std::map<int, CastClient>::iterator it;
	it = clients.find(session);
	if (it != clients.end()) {
		returnMsg->setResultValue(new NymphUint8(1));
		return returnMsg;
	}
	
	// Safely write the data for this session to the buffer.
	std::string mediaData = ((NymphString*) msg->parameters()[0])->getValue();
	bool done = ((NymphBoolean*) msg->parameters()[1])->getValue();
	
	// Copy pointer into free slot of vector, delete data if not empty.
	// Reset the next slot value if the end of the vector has been reached.
	// 
	// TODO: optimise.
	// TODO: prevent accidental overwriting.
	// TODO: update front/back index counters.
	bufferMutex.lock();
	if (media_buffer.freeSlots > 0) {
		std::cout << "Writing into buffer slot: " << media_buffer.nextSlot << std::endl;
		media_buffer.data[media_buffer.nextSlot] = mediaData;
		std::cout << "Next buffer slot: " << media_buffer.nextSlot << std::endl;
		if (media_buffer.nextSlot == media_buffer.currentSlot) {
			media_buffer.slotSize = mediaData.length();
			media_buffer.slotBytesLeft = mediaData.length();
		}
		
		media_buffer.nextSlot++;
		if (!(media_buffer.nextSlot < media_buffer.numSlots)) { media_buffer.nextSlot = 0; }
		
		media_buffer.freeSlots--;
		media_buffer.buffBytesLeft += mediaData.length();
	}
	
	bufferMutex.unlock();
	
	// Start the player if it hasn't yet. This ensures we have a buffer ready.
	if (!playerStarted) {
		playerStarted = true;
		libvlc_media_player_play(mp);
	}
	
	// if 'done' is true, the client has sent the last bytes. Signal session end in this case.
	if (done) {		
		bufferMutex.lock();
		media_buffer.eof = true;
		bufferMutex.unlock();
	}
	else {
		// Request as much data from the client as we have space in the buffer.
		/* std::vector<NymphType*> values;
		std::string result;
		NymphBoolean* resVal = 0;
		if (!NymphRemoteClient::callCallback(session, "MediaReadCallback", values, result)) {
			std::cerr << "Calling read callback failed: " << result << std::endl;
			resVal = new NymphBoolean(false);
		}
		else { 
			resVal = new NymphBoolean(true);
			it->second.sessionActive = true;
		} */
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


// --- LOG FUNCTION ---
void logFunction(int level, std::string logStr) {
	std::cout << level << " - " << logStr << std::endl;
}


int main() {
	// Initialise the server.
	std::cout << "Initialising server...\n";
	long timeout = 5000; // 5 seconds.
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	// Initialise the PortAudio library.
	/* PaError err = Pa_Initialize();
	if (err != paNoError) {
		std::cerr << "PortAudio initialisation error: " << Pa_GetErrorText(err) << std::endl;
		// TODO: handle.
	} */
	
	// Open audio output.
	//paTestData data;
    //err = Pa_OpenDefaultStream( &stream,
      //                          0,          /* no input channels */
       //                         2,          /* stereo output */
        //                        paFloat32,  /* 32 bit floating point output */
         //                       SAMPLE_RATE,
         //                       paFramesPerBufferUnspecified,        /* frames per buffer, i.e. the number
         //                                          of sample frames that PortAudio will
         //                                          request from the callback. Many apps
         //                                          may want to use
          //                                         paFramesPerBufferUnspecified, which
          //                                         tells PortAudio to pick the best,
         //                                          possibly changing, buffer size.*/
         //                       patestCallback, /* this is your callback function */
         //                       &data ); /*This is a pointer that will be passed to
         //                                          your callback*/
   /*  if (err != paNoError) {
		std::cerr << "PortAudio initialisation error: " << Pa_GetErrorText(err) << std::endl;
		// TODO: handle.
	} */
	
	// Initialise LibVLC.
	libvlc_instance_t *libvlc;
	libvlc_media_t *m;
	//libvlc_media_player_t *mp;
	char const *vlc_argv[] = {
 
		//"--no-audio", // Don't play audio.
		"--no-xlib", // Don't use Xlib.
		"--verbose=2"
 
		// Apply a video filter.
		//"--video-filter", "sepia",
		//"--sepia-intensity=200"
	};
	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
 
	SDL_Event event;
	int done = 0, action = 0, pause = 0, n = 0;
 
	struct context context;
	
	// Initialise libSDL.
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		return EXIT_FAILURE;
	}
 
	// Create SDL graphics objects.
	SDL_Window * window = SDL_CreateWindow(
			"NymphCast",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			WIDTH, HEIGHT,
			SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
	if (!window) {
		fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
		quit(3);
	}
 
	context.renderer = SDL_CreateRenderer(window, -1, 0);
	if (!context.renderer) {
		fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
		quit(4);
	}
 
	context.texture = SDL_CreateTexture(
			context.renderer,
			SDL_PIXELFORMAT_BGR565, SDL_TEXTUREACCESS_STREAMING,
			VIDEOWIDTH, VIDEOHEIGHT);
	if (!context.texture) {
		fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
		quit(5);
	}
 
	context.mutex = SDL_CreateMutex();
 
	// If you don't have this variable set you must have plugins directory
	// with the executable or libvlc_new() will not work!
	printf("VLC_PLUGIN_PATH=%s\n", getenv("VLC_PLUGIN_PATH"));
 
	// Initialise libVLC.
	libvlc = libvlc_new(vlc_argc, vlc_argv);
	if(NULL == libvlc) {
		printf("LibVLC initialization failure.\n");
		return EXIT_FAILURE;
	}
	
	
	// Define all of the RPC methods we want to export for clients.
	std::cout << "Registering methods...\n";
	std::vector<NymphTypes> parameters;
	
	// Client connects to server.
	// bool connect(string client_id)
	parameters.push_back(NYMPH_STRING);
	NymphMethod connectFunction("connect", parameters, NYMPH_BOOL);
	connectFunction.setCallback(connect);
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
	parameters.push_back(NYMPH_STRING);
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
	// End client callback registration.
	
	// Create empty buffer with N entries, initialised as empty strings.
	bufferMutex.lock();
	media_buffer.data.assign(50, std::string());
	media_buffer.size = media_buffer.data.size();
	media_buffer.currentIndex = 0;		// The current index into the vector element.
	media_buffer.currentSlot = 0;		// The current vector slot we're using.
	media_buffer.numSlots = 50;			// Total number of slots in the data vector.
	media_buffer.nextSlot = 0;			// Next slot to fill in the buffer vector.
	media_buffer.buffIndexLow = 0;		// File index at the buffer front.
	media_buffer.buffIndexHigh = 0;	
	media_buffer.freeSlots = 50;
	media_buffer.eof = false;
	bufferMutex.unlock();
	
	// Set up callbacks.
	m = libvlc_media_new_callbacks(
                                    libvlc,
									media_open_cb,
									media_read_cb,
									media_seek_cb,
									media_close_cb,
									&media_buffer);
	
	// Debug
	std::cout << "Created new LibVLC player." << std::endl;
 
	mp = libvlc_media_player_new_from_media(m);
 
	libvlc_video_set_callbacks(mp, lock, unlock, display, &context);
	libvlc_video_set_format(mp, "RV16", VIDEOWIDTH, VIDEOHEIGHT, VIDEOWIDTH * 2);
	//libvlc_media_player_play(mp);
	playerStarted = false;
	
	// Debug
	//std::cout << "Entering main loop." << std::endl;
	
	
	// Install signal handler to terminate the server.
	signal(SIGINT, signal_handler);
	
	// Start server on port 4004.
	NymphRemoteClient::start(4004);
	
	// Start the data request handler.
	dataRequestFunction();
	
	// Wait for the condition to be signalled.
	gMutex.lock();
	gCon.wait(gMutex);
	
	// Clean-up
	// Stop stream and clean up libVLC.
	libvlc_media_player_stop(mp);
	libvlc_media_player_release(mp);
	libvlc_release(libvlc);
 
	// Close window and clean up libSDL.
	SDL_DestroyMutex(context.mutex);
	SDL_DestroyRenderer(context.renderer);
	
	NymphRemoteClient::shutdown();
	
	// Wait before exiting, giving threads time to exit.
	Thread::sleep(2000); // 2 seconds.
	
	return 0;
}
