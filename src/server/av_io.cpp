/*
	av_io.cpp - Implementation file for the static AV_IO class.
	
	Revision 0
	
	Notes:
			- 
		
	2019/09/15 - Maya Posch
*/


#define SAMPLE_RATE (44100)


#include "av_io.h"

#include <nymph/nymph.h>

// Debug
#include <fstream>
std::ofstream OUT_FILE("out.mpg", std::ios::binary | std::ios::app);


// Static initialisations.
struct SDLContext AV_IO::context;

// Globals
DataBuffer media_buffer;


// -- INIT ---
// Initialise LibSDL2
bool AV_IO::init() {
	// Initialise libSDL.
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		return false;
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
		return false;
	}
 
	context.renderer = SDL_CreateRenderer(window, -1, 0);
	if (!context.renderer) {
		fprintf(stderr, "Couldn't create renderer: %s\n", SDL_GetError());
		return false;
	}
 
	context.texture = SDL_CreateTexture(
			context.renderer,
			SDL_PIXELFORMAT_BGR565, SDL_TEXTUREACCESS_STREAMING,
			VIDEOWIDTH, VIDEOHEIGHT);
			
	if (!context.texture) {
		fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
		return false;
	}
 
	context.mutex = SDL_CreateMutex();
}


bool AV_IO::vlcInit() {
	// Initialise LibVLC.
	char const *vlc_argv[] = {
 
		//"--no-audio", // Don't play audio.
		//"--no-xlib", // Don't use Xlib.
		"--verbose=2"
 
		// Apply a video filter.
		//"--video-filter", "sepia",
		//"--sepia-intensity=200"
	};
	int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
	
	libvlc = libvlc_new(vlc_argc, vlc_argv);
	if(NULL == libvlc) {
		printf("LibVLC initialization failure.\n");
		return false;
	}
	
	// Set up callbacks.
	m = libvlc_media_new_callbacks(
                                    libvlc,
									AV_IO::media_open_cb,
									AV_IO::media_read_cb,
									AV_IO::media_seek_cb,
									AV_IO::media_close_cb,
									&media_buffer);
	
	// Debug
	std::cout << "Created new LibVLC player." << std::endl;
 
	mp = libvlc_media_player_new_from_media(m);
 
	libvlc_video_set_callbacks(mp, lock, AV_IO::unlock, AV_IO::display, &AV_IO::context);
	libvlc_video_set_format(mp, "RV16", VIDEOWIDTH, VIDEOHEIGHT, VIDEOWIDTH * 2);
}


// --- LOCK ---
// VLC prepares to render a video frame.
void* AV_IO::lock(void* data, void** p_pixels) {
	struct SDLContext* c = (SDLContext*) data;
 
	int pitch;
	SDL_LockMutex(c->mutex);
	SDL_LockTexture(c->texture, 0, p_pixels, &pitch);
 
	return 0;
}
 
 
// -- UNLOCK ---
// VLC just rendered a video frame.
void AV_IO::unlock(void* data, void* id, void* const* p_pixels) {
	struct SDLContext* c = (SDLContext*) data;
 
	SDL_UnlockTexture(c->texture);
	SDL_UnlockMutex(c->mutex);
}


// --- DISPLAY ---
// VLC wants to display a video frame.
void AV_IO::display(void *data, void *id) {
	struct SDLContext* c = (SDLContext*) data;
 
	SDL_Rect rect;
	rect.w = WIDTH;
	rect.h = HEIGHT;
	rect.x = 0;
	rect.y = 0;
 
	SDL_SetRenderDrawColor(c->renderer, 0, 80, 0, 255);
	SDL_RenderClear(c->renderer);
	SDL_RenderCopy(c->renderer, c->texture, 0, &rect);
	SDL_RenderPresent(c->renderer);
}


// LibVLC callbacks.
// --- MEDIA OPEN CB ---
// 0 on success, non-zero on error. In case of failure, the other callbacks will not be invoked 
// and any value stored in *datap and *sizep is discarded.
int AV_IO::media_open_cb(void* opaque, void** datap, uint64_t* sizep) {
	// Debug
	std::cout << "Media open callback called." << std::endl;
	
	DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
    //*sizep = db->data.size();
	// FIXME: determine the right size for this...
    //*sizep = 10 * 1024; // 10 kB buffer.
	db->mutex.lock();
    *sizep = db->size; // Set to stream size.
	db->mutex.unlock();
    *datap = opaque;
    return 0;
}


// --- MEDIA READ CB ---
ssize_t AV_IO::media_read_cb(void* opaque, unsigned char* buf, size_t len) {
	DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
	// Fill the buffer from the memory buffer.
	// Return the read length.
	// TODO: account for buffer underrun.
	uint32_t bytesToCopy = 0;
	
	db->mutex.lock();
	
	// Check if we're headed for a buffer underrun.
	if (db->buffBytesLeft < len && !db->eof) {
		db->requestCondition.signal(); // Ask for more data.
		db->mutex.unlock();
		db->bufferDelayMutex.lock();
		db->bufferDelayCondition.tryWait(db->bufferDelayMutex, 150);
		db->mutex.lock();
	}

	if (db->buffBytesLeft >= len) {  	// At least as many bytes remaining as requested
		bytesToCopy = len;
	} 
	else if (db->buffBytesLeft < len) {	// Fewer than requested number of bytes remaining
		bytesToCopy = db->buffBytesLeft;
	} 
	else {
		db->mutex.unlock();
		return 0;   // No bytes left to copy
	}

	// Each slot has a limited depth. Check that we can copy the whole requested buffer
	// from a single slot, only updating the index into that slot.
	// Else, copy what is left in the current slot into the buffer, then copy the rest from the
	// next slot.
	if (db->slotBytesLeft < bytesToCopy) {
					
		uint32_t nextBytes = bytesToCopy;
		
		// Copy the rest of the bytes in the slot, move onto the next slot.
		uint32_t byteCount = 0;
		uint32_t bytesWritten = 0;
		bool nextSlot = false;
		while (nextBytes > 0) {
			if (db->slotBytesLeft < nextBytes) {
				// Cannot copy the remainder of the requested bytes in one go yet.
				// Just copy what we can.
				byteCount = db->slotBytesLeft;
				nextBytes -= db->slotBytesLeft;
				nextSlot = true;
			}
			else {
				// Copy the remaining bytes into the buffer.
				byteCount = nextBytes;
				nextBytes = 0;
				if (db->slotBytesLeft == byteCount) {
					nextSlot = true;
				}
			}
			
			// Debug
			std::cout << "Reading from slot " << db->currentSlot << std::endl;
			std::cout << "Index: " << db->currentIndex << "\t/\t" << db->slotSize 
						<< ", \tCopy: " << bytesToCopy << "\t/\t" << db->slotBytesLeft << std::endl;
			
			std::copy(db->data[db->currentSlot].begin() + db->currentIndex, 
					(db->data[db->currentSlot].begin() + db->currentIndex) + byteCount, 
					(buf + bytesWritten));
							
			db->slotBytesLeft -= byteCount;
			bytesWritten += byteCount;
			if (nextSlot) {
				nextSlot = false;
				db->currentSlot++;
				if (!(db->currentSlot < db->numSlots)) { db->currentSlot = 0; }
				db->slotSize = db->data[db->currentSlot].length();
				db->currentIndex = 0;
				db->slotBytesLeft = db->slotSize;
				db->freeSlots++; // The used buffer slot just became available for more data.
			}
			else {
				db->currentIndex += byteCount;
			}
		}
	}
	else {
		// Debug
		std::cout << "Reading from slot " << db->currentSlot << std::endl;
		std::cout << "Index: " << db->currentIndex << "\t/\t" << db->slotSize 
					<< ", \tCopy: " << bytesToCopy << "\t/\t" << db->slotBytesLeft << std::endl;
		
		// Just copy the bytes from the slot, adjusting the index and bytes left count.
		std::copy(db->data[db->currentSlot].begin() + db->currentIndex, 
				(db->data[db->currentSlot].begin() + db->currentIndex) + bytesToCopy, 
				buf);
		db->currentIndex += bytesToCopy;    // Increment bytes read count
		db->slotBytesLeft -= bytesToCopy;
	}
	
	db->buffBytesLeft -= bytesToCopy;
	
	// If there are free slots in the buffer, request more data from the client.
	if (!db->requestInFlight && !(db->eof) && db->freeSlots > 0) {
		db->requestCondition.signal();
	}
	
	db->mutex.unlock();
	
	// Debug
	OUT_FILE.write((const char*) buf, bytesToCopy);

	return bytesToCopy;
}

int AV_IO::media_seek_cb(void* opaque, uint64_t offset) {
	DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
	// Try to find the index in the buffered data. If unavailable, read from file.
	//db->index = offset;
	
	// TODO: implement.
	
	return 0;
}

void AV_IO::media_close_cb(void *opaque) {
	DataBuffer* db = static_cast<DataBuffer*>(opaque); 
	
	db->mutex.lock();
	int session = db->activeSession;
	db->mutex.unlock();
		
	std::vector<NymphType*> values;
	std::string result;
	NymphBoolean* resVal = 0;
	if (!NymphRemoteClient::callCallback(session, "MediaStopCallback", values, result)) {
		std::cerr << "Calling stop callback failed: " << result << std::endl;
	}
}
// End LibVLC callbacks.


// --- SET BUFFER ---
// Pass pointer to the data buffer structure.
//void AV_IO::setBuffer(DataBuffer* buffer) { this->buffer = buffer; }


// --- RUN ---
void AV_IO::run() {
	init();
	vlcInit();
	running = true;
	
	libvlc_media_player_play(mp);
	
	SDL_Event event;
	int done = 0, action = 0, pause = 0, n = 0;
	
	while (running) {
		action = 0;
 
		// Keys: enter (fullscreen), space (pause), escape (quit).
		while( SDL_PollEvent( &event )) {
 
			switch (event.type) {
				case SDL_QUIT:
					done = 1;
					break;
				case SDL_KEYDOWN:
					action = event.key.keysym.sym;
					break;
			}
		}
 
		switch (action) {
			case SDLK_ESCAPE:
			case SDLK_q:
				done = 1;
				break;
			case ' ':
				printf("Pause toggle.\n");
				pause = !pause;
				break;
		}
 
		if (!pause) { context.n++; }
 
		SDL_Delay(1000/10);
	}
}
 
 
// --- QUIT ---
void AV_IO::quit() {
	// Stop stream and clean up libVLC.
	libvlc_media_player_stop(mp);
	libvlc_media_player_release(mp);
	libvlc_release(libvlc);
	
	SDL_DestroyMutex(context.mutex);
	SDL_DestroyRenderer(context.renderer);
	SDL_Quit();
	
	running = false;
}
