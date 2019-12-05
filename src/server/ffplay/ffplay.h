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



struct DataBuffer {
	std::vector<std::string> data;	// The data byte, inside string instances.
	std::atomic<uint64_t> size;				// Number of bytes in data vector.
	std::atomic<uint64_t> currentIndex;		// The current index into the vector element.
	std::atomic<uint32_t> currentSlot;		// The current vector slot we're using.
	std::atomic<uint32_t> slotSize;			// Slot size in bytes.
	std::atomic<uint32_t> slotBytesLeft;		// Bytes left in the current slot.
	std::atomic<uint32_t> numSlots;			// Total number of slots in the data vector.
	std::atomic<uint32_t> nextSlot;			// Next slot to fill in the buffer vector.
	std::atomic<uint32_t> freeSlots;			// Slots free to write new data into.
	std::atomic<uint32_t> buffBytesLeft;		// Number of bytes available for reading in the buffer.
	std::atomic<bool> eof;					// Whether End of File for the source file has been reached.
	
	std::atomic<uint64_t> buffIndexLow;		// File index at the buffer front.
	std::atomic<uint64_t> buffIndexHigh;		// File index at the buffer back.
	Poco::Mutex mutex;
	Poco::Mutex dataMutex;
	std::atomic<uint32_t> activeSession;
	Poco::Condition bufferDelayCondition;
	Poco::Mutex bufferDelayMutex;
	Poco::Condition requestCondition;
	Poco::Mutex requestMutex;
	std::atomic<bool> requestInFlight;
	
	Poco::Mutex streamTrackQueueMutex;
	std::queue<std::string> streamTrackQueue;
};



// --- Globals ---
extern DataBuffer media_buffer;
extern std::atomic<bool> playerStarted;
	
void resetDataBuffer(); // Defined in NymphCastServer.cpp

	
class Ffplay : public Poco::Runnable {
	static int media_read(void* opaque, uint8_t* buf, int buf_size);
	static int64_t media_seek(void* opaque, int64_t pos, int whence);
	
public:
	//void setBuffer(DataBuffer* buffer);
	virtual void run();
	void quit();
};


#endif
