/*
	ffplay.h - Header file for the static Ffplay class.
	
	Revision 0
	
	Notes:
			- 
		
	2019/10/15 - Maya Posch
*/


#ifndef FFPLAY_H
#define FFPLAY_H


#include <iostream>
#include <atomic>
#include <queue>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mutex.h>


#include <Poco/Condition.h>
#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include "types.h"



struct DataBuffer {
	std::vector<std::string> data;			// The data byte, inside string instances.
	std::atomic<uint64_t> size;				// Number of bytes in data vector.
	std::atomic<uint64_t> currentIndex;		// The current index into the vector element.
	std::atomic<uint32_t> currentSlot;		// The current vector slot we're using.
	std::atomic<uint32_t> slotSize;			// Slot size in bytes.
	std::atomic<uint32_t> slotBytesLeft;	// Bytes left in the current slot.
	std::atomic<uint32_t> numSlots;			// Total number of slots in the data vector.
	std::atomic<uint32_t> nextSlot;			// Next slot to fill in the buffer vector.
	std::atomic<uint32_t> freeSlots;		// Slots free to write new data into.
	std::atomic<uint32_t> buffBytesLeft;	// Number of bytes available for reading in the buffer.
	std::atomic<bool> eof;					// Whether End of File for the source file has been reached.
	
	std::atomic<uint64_t> buffIndexLow;		// File index at the buffer front (low index).
	std::atomic<uint64_t> buffIndexHigh;	// File index at the buffer back (high index).
	Poco::Mutex mutex;
	Poco::Mutex dataMutex;
	std::atomic<uint32_t> activeSession;
	Poco::Condition bufferDelayCondition;
	Poco::Mutex bufferDelayMutex;
	Poco::Condition requestCondition;
	Poco::Mutex requestMutex;
	std::atomic<bool> requestInFlight;
	std::atomic<bool> seeking;				// Are we performing a seeking operation?
	std::atomic<uint64_t> seekingPosition;	// Position to seek to.
	
	Poco::Mutex streamTrackQueueMutex;
	std::queue<std::string> streamTrackQueue;
};


struct FileMetaInfo {
	//std::atomic<uint32_t> filesize;		// bytes.
	std::atomic<uint64_t> duration;		// seconds
	std::atomic<double> position;		// seconds with remainder.
	std::atomic<uint32_t> width;		// pixels
	std::atomic<uint32_t> height;		// pixels
	std::atomic<uint32_t> video_rate;	// kilobits per second
	std::atomic<uint32_t> audio_rate;	// kilobits per second
	std::atomic<uint32_t> framrate;
	std::atomic<uint8_t> audio_channels;
	std::string title;
	std::string artist;
	std::string album;
	Poco::Mutex mutex;	// Use only with non-atomic entries.
};


// --- Globals ---
extern DataBuffer media_buffer;
extern FileMetaInfo file_meta;
extern std::atomic<bool> playerStarted;
	
void resetDataBuffer(); // Defined in NymphCastServer.cpp

	
class Ffplay : public Poco::Runnable {
    VideoState* is = 0;
	
	static int media_read(void* opaque, uint8_t* buf, int buf_size);
	static int64_t media_seek(void* opaque, int64_t pos, int whence);
	
public:
	//void setBuffer(DataBuffer* buffer);
	virtual void run();
	uint8_t getVolume();
	void setVolume(uint8_t volume);
	void quit();
};


#endif
