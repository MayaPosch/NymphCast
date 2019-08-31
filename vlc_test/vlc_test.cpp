// libSDL and libVLC sample code.
// License: [http://en.wikipedia.org/wiki/WTFPL WTFPL]
 
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
 
#include <SDL2/SDL.h>
#include <SDL2/SDL_mutex.h>
 
#include <vlc/vlc.h>


#include <fstream>
#include <vector>
#include <iterator>
#include <iostream>

 
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
 
	//uint16_t *pixels = (uint16_t *)*p_pixels;
 
	// We can also render stuff.
	/* int x, y;
	for(y = 10; y < 40; y++) {
		for(x = 10; x < 40; x++) {
			if(x < 13 || y < 13 || x > 36 || y > 36) {
				pixels[y * VIDEOWIDTH + x] = 0xffff;
			} else {
				// RV16 = 5+6+5 pixels per color, BGR.
				pixels[y * VIDEOWIDTH + x] = 0x02ff;
			}
		}
	} */
 
	SDL_UnlockTexture(c->texture);
	SDL_UnlockMutex(c->mutex);
}
 
// VLC wants to display a video frame.
static void display(void *data, void *id) {
 
	struct context *c = (context *)data;
 
	SDL_Rect rect;
	/* rect.w = VIDEOWIDTH;
	rect.h = VIDEOHEIGHT;
	rect.x = (int)((1. + .5 * sin(0.03 * c->n)) * (WIDTH - VIDEOWIDTH) / 2);
	rect.y = (int)((1. + .5 * cos(0.03 * c->n)) * (HEIGHT - VIDEOHEIGHT) / 2); */
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


struct DataBuffer {
	std::vector<uint8_t> data;	// The data bytes.
	uint64_t size;				// Number of bytes in data vector.
	uint64_t index;				// The current index into the vector.
};


// Globals.
/* const int BUFFER_SIZE = 1024;
unsigned char video_buffer[BUFFER_SIZE]; */

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
	// Debug
	//std::cout << "Media read callback called." << std::endl;
	
	DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
	// Fill the buffer from the open file.
	// Return the read length.
	int bytesToCopy = 0;
	//int bytesSoFar = db->index + 1;
	int bytesRemaining = db->size - (db->index + 1);

	if (bytesRemaining >= len) {  // At least as many bytes remaining as requested
		bytesToCopy = len;
	} 
	else if (bytesRemaining < len) {	//less that requested number of bytes remaining
		bytesToCopy = bytesRemaining;
	} else {
		return 0;   // no bytes left to copy
	}

	std::copy(db->data.begin() + db->index, 
				(db->data.begin() + db->index) + bytesToCopy, 
				buf);  											// Copy bytes requested to buffer.
	db->index = db->index + bytesToCopy;    					// Increment bytes read count

	return bytesToCopy;
}

int media_seek_cb(void* opaque, uint64_t offset) {
	DataBuffer* db = static_cast<DataBuffer*>(opaque);
	
	// Try to find the index in the buffered data. If unavailable, read from file.
	db->index = offset;
	
	return 0;
}

void media_close_cb(void *opaque) {
	// Close the file.
	
}
// End LibVLC callbacks.

 
int main(int argc, char *argv[]) {
 
	libvlc_instance_t *libvlc;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;
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
 
	if(argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return EXIT_FAILURE;
	}
 
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
	
	// Set up callbacks.
/* 	LIBVLC_API libvlc_media_t* libvlc_media_new_callbacks 	( 	libvlc_instance_t *  	instance,
		libvlc_media_open_cb  	open_cb,
		libvlc_media_read_cb  	read_cb,
		libvlc_media_seek_cb  	seek_cb,
		libvlc_media_close_cb  	close_cb,
		void *  	opaque 
	) 	 */	
	
	// Read file into memory. Assume we have enough RAM for this.
	std::ifstream source(argv[1], std::ios::binary);
	if (!source.good()) {
		std::cerr << "Failed to read input file." << std::endl;
		return EXIT_FAILURE;
	}
	
	DataBuffer video_buffer;
	std::copy(std::istreambuf_iterator<char>(source), 
				std::istreambuf_iterator<char>(), 
				std::back_inserter(video_buffer.data));
	/* video_buffer.data = std::vector<uint8_t>((std::istreambuf_iterator<uint8_t>(source)), 
								std::istreambuf_iterator<uint8_t>()); */
	video_buffer.index = 0;
	video_buffer.size = video_buffer.data.size();
	
	// Debug
	std::cout << "Read input file with size: " << video_buffer.size << "." << std::endl;
	
	// Debug
	// Dump the vector data for validation.
	//std::ofstream OUT("output.mpg", std::ios::binary);
	//std::copy(video_buffer.data.begin(), video_buffer.data.end(), std::ostream_iterator<uint8_t>(OUT));
	
	m = libvlc_media_new_callbacks(
                                    libvlc,
									media_open_cb,
									media_read_cb,
									media_seek_cb,
									media_close_cb,
									&video_buffer);
	//mp = libvlc_media_player_new(libvlc);
	
	// Debug
	std::cout << "Created new LibVLC player." << std::endl;
 
	/* m = libvlc_media_new_path(libvlc, argv[1]);*/
	mp = libvlc_media_player_new_from_media(m);
	///libvlc_media_release(m);
 
	libvlc_video_set_callbacks(mp, lock, unlock, display, &context);
	libvlc_video_set_format(mp, "RV16", VIDEOWIDTH, VIDEOHEIGHT, VIDEOWIDTH * 2);
	libvlc_media_player_play(mp);
	
	// Debug
	std::cout << "Entering main loop." << std::endl;
 
	// Main loop.
	while(!done) {
 
		action = 0;
 
		// Keys: enter (fullscreen), space (pause), escape (quit).
		while( SDL_PollEvent( &event )) {
 
			switch(event.type) {
				case SDL_QUIT:
					done = 1;
					break;
				case SDL_KEYDOWN:
					action = event.key.keysym.sym;
					break;
			}
		}
 
		switch(action) {
			case SDLK_ESCAPE:
			case SDLK_q:
				done = 1;
				break;
			case ' ':
				printf("Pause toggle.\n");
				pause = !pause;
				break;
		}
 
		if(!pause) { context.n++; }
 
		SDL_Delay(1000/10);
	}
 
	// Stop stream and clean up libVLC.
	libvlc_media_player_stop(mp);
	libvlc_media_player_release(mp);
	libvlc_release(libvlc);
 
	// Close window and clean up libSDL.
	SDL_DestroyMutex(context.mutex);
	SDL_DestroyRenderer(context.renderer);
 
	quit(0);
 
	return 0;
}