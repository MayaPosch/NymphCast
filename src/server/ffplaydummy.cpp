/*
	ffplaydummy.cpp - Implementation file for the dummy ffplay class.
	
	Revision 0
	
	Notes:
			- 
		
	2021/09/29 - Maya Posch
*/


#include "ffplaydummy.h"


// Disable NymphLogger.
#define NO_NYMPH_LOGGER = 1


#include <Poco/NumberFormatter.h>

#ifndef NO_NYMPH_LOGGER
#include <nymph/nymph_logger.h>
#endif


// Static definitions.
std::string FfplayDummy::loggerName = "FfplayDummy";
ChronoTrigger FfplayDummy::ct;
//int FfplayDummy::buf_size = 32 * 1024;
int FfplayDummy::buf_size = 128 * 1024;
uint8_t* FfplayDummy::buf;
uint8_t FfplayDummy::count = 0;
uint32_t start_size = 27 * 1024; 	// Initial size of a request.
uint32_t step_size = 1 * 1014;		// Request 1 kB more each cycle until buf_size is reached.
uint32_t read_size = 0;


// Global objects.
Poco::Condition dummyCon;
Poco::Mutex dummyMutex;
// ---


#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <string>
#include <cstring>
#include <vector>
#include <chrono>

#include "ffplay/types.h"
#include "databuffer.h"


/**
 * Reads from a buffer into FFmpeg.
 *
 * @param ptr	   A pointer to the user-defined IO data structure.
 * @param buf	   A buffer to read into.
 * @param buf_size  The size of the buffer buff.
 *
 * @return The number of bytes read into the buffer.
 */
int FfplayDummy::media_read(void* opaque, uint8_t* buf, int buf_size) {
	uint32_t bytesRead = DataBuffer::read(buf_size, buf);
#ifndef NO_NYMPH_LOGGER
	NYMPH_LOG_INFORMATION("Read " + Poco::NumberFormatter::format(bytesRead) + " bytes.");
#else
	std::cout << "Read " << bytesRead << " bytes." << std::endl;;
#endif
	if (bytesRead == 0) {
		std::cout << "EOF is " << DataBuffer::isEof() << std::endl;
		if (DataBuffer::isEof()) { return -1; }
		else { return -1; }
	}
	
	return bytesRead;
}


/**
 * Seeks to a given position in the currently open file.
 * 
 * @param opaque  A pointer to the user-defined IO data structure.
 * @param offset  The position to seek to.
 * @param whence  SEEK_SET, SEEK_CUR, SEEK_END (like fseek) and AVSEEK_SIZE.
 *
 * @return  The new byte position in the file or -1 in case of failure.
 */
int64_t FfplayDummy::media_seek(void* opaque, int64_t offset, int whence) {
#ifndef NO_NYMPH_LOGGER
	NYMPH_LOG_INFORMATION("media_seek: offset " + Poco::NumberFormatter::format(offset) + 
							", whence " + Poco::NumberFormatter::format(whence));
#else
	std::cout << "media_seek: offset " << offset << ", whence " << whence << std::endl;
#endif
							
	
	int64_t new_offset = -1;
	switch (whence) {
		case SEEK_SET:	// Seek from the beginning of the file.
#ifndef NO_NYMPH_LOGGER
			NYMPH_LOG_INFORMATION("media_seek: SEEK_SET");
#else
			std::cout << "media_seek: SEEK_SET" << std::endl;
			new_offset = DataBuffer::seek(DB_SEEK_START, offset);
#endif
			break;
		case SEEK_CUR:	// Seek from the current position.
#ifndef NO_NYMPH_LOGGER
			NYMPH_LOG_INFORMATION("media_seek: SEEK_CUR");
#else
			std::cout << "media_seek: SEEK_CUR" << std::endl;;
#endif
			new_offset = DataBuffer::seek(DB_SEEK_CURRENT, offset);
			break;
		case SEEK_END:	// Seek from the end of the file.
#ifndef NO_NYMPH_LOGGER
			NYMPH_LOG_INFORMATION("media_seek: SEEK_END");
#else
			std::cout << "media_seek: SEEK_END" << std::endl;
#endif
			new_offset = DataBuffer::seek(DB_SEEK_END, offset);
			break;
		case AVSEEK_SIZE:
#ifndef NO_NYMPH_LOGGER
			NYMPH_LOG_INFORMATION("media_seek: received AVSEEK_SIZE, returning file size.");
#else
			std::cout << "media_seek: received AVSEEK_SIZE, returning file size." << std::endl;
#endif
			return DataBuffer::getFileSize();
			break;
		default:
#ifndef NO_NYMPH_LOGGER
			NYMPH_LOG_ERROR("media_seek: default. The universe broke.");
#else
			std::cout << "media_seek: default. The universe broke." << std::endl;
#endif
	}
	
	if (new_offset < 0) {
		// Some error occurred.
#ifndef NO_NYMPH_LOGGER
		NYMPH_LOG_ERROR("Error during seeking.");
#else
		std::cout << "Error during seeking." << std::endl;
#endif
		new_offset = -1;
	}
	
#ifndef NO_NYMPH_LOGGER
	NYMPH_LOG_INFORMATION("New offset: " + Poco::NumberFormatter::format(new_offset));
#else
	std::cout << "New offset: " << new_offset << std::endl;;
#endif
	
	return new_offset;
}


// --- GET VOLUME ---
uint8_t FfplayDummy::getVolume() {
	return audio_volume;
}


// --- SET VOLUME ---
void FfplayDummy::setVolume(uint8_t volume) {
	audio_volume = volume;
}


// --- TRIGGER READ ---
void FfplayDummy::triggerRead(int) {
	// TODO: if first read, seek to end - N bytes.
	// If second read, seek to beginning.
	if (count == 0) {
		// Seek to end - 10 kB.
		media_seek(0, DataBuffer::getFileSize() - (10 * 1024), SEEK_SET);
		count++;
		return;
	}
	else if (count == 1) {
		// Seek to beginning.
		media_seek(0, 0, SEEK_SET);
		count++;
		return;
	}
	
	// Call read() with a data request. This data is then discarded.
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	read_size += step_size;
	if (read_size > buf_size) {
		read_size = start_size;
	}
	
	if (media_read(0, buf, read_size) == -1) {
		// Signal the player thread that the playback has ended.
		dummyCon.signal();
	}
	
	// Report time for the media_read call.
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
#ifndef NO_NYMPH_LOGGER
	NYMPH_LOG_INFORMATION("Read duration: " + Poco::NumberFormatter::format(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) + "µs.");
#else
	std::cout << "Read duration: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "µs." << std::endl;
#endif
	
}


void FfplayDummy::cleanUp() {
	//
}


// --- RUN ---
void FfplayDummy::run() {
	buf = (uint8_t*) malloc(buf_size);
	
	read_size = start_size;
	
	// Start dummy playback:
	// - Request a 32 kB data block every N milliseconds.
	// - Once EOF is returned, wait N milliseconds, then quit.
	ct.setCallback(FfplayDummy::triggerRead, 0);
	ct.setStopCallback(FfplayDummy::cleanUp);
	ct.start(50);	// Trigger time in milliseconds.
	
	// Wait here until playback has finished.
	// The read thread in StreamHandler will signal this condition variable.
	dummyMutex.lock();
	dummyCon.wait(dummyMutex);
	dummyMutex.unlock();
	
	ct.stop();
	
	DataBuffer::reset();	// Clears the data buffer (file data buffer).
	finishPlayback();		// Calls handler for post-playback steps.
	
	// Clean up.
	free(buf);
}

 
// --- QUIT ---
void FfplayDummy::quit() {
	dummyCon.signal();
}
