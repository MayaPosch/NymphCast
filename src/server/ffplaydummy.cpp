/*
	ffplaydummy.cpp - Implementation file for the dummy ffplay class.
	
	Revision 0
	
	Notes:
			- 
		
	2021/09/29 - Maya Posch
*/


#include "ffplaydummy.h"

#include <Poco/NumberFormatter.h>
#include <nymph/nymph_logger.h>


// Static definitions.
std::string FfplayDummy::loggerName = "FfplayDummy";
ChronoTrigger FfplayDummy::ct;
int FfplayDummy::buf_size = 32 * 1024;
uint8_t* FfplayDummy::buf;


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
	NYMPH_LOG_INFORMATION("Read " + Poco::NumberFormatter::format(bytesRead) + " bytes.");
	if (bytesRead == 0) {
		std::cout << "EOF is " << DataBuffer::isEof() << std::endl;
		if (DataBuffer::isEof()) { return AVERROR_EOF; }
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
	NYMPH_LOG_INFORMATION("media_seek: offset " + Poco::NumberFormatter::format(offset) + 
							", whence " + Poco::NumberFormatter::format(whence));
	
	int64_t new_offset = -1;
	switch (whence) {
		case SEEK_SET:	// Seek from the beginning of the file.
			NYMPH_LOG_INFORMATION("media_seek: SEEK_SET");
			new_offset = DataBuffer::seek(DB_SEEK_START, offset);
			break;
		case SEEK_CUR:	// Seek from the current position.
			NYMPH_LOG_INFORMATION("media_seek: SEEK_CUR");
			new_offset = DataBuffer::seek(DB_SEEK_CURRENT, offset);
			break;
		case SEEK_END:	// Seek from the end of the file.
			NYMPH_LOG_INFORMATION("media_seek: SEEK_END");
			new_offset = DataBuffer::seek(DB_SEEK_END, offset);
			break;
		case AVSEEK_SIZE:
			NYMPH_LOG_INFORMATION("media_seek: received AVSEEK_SIZE, returning file size.");
			return DataBuffer::getFileSize();
			break;
		default:
			NYMPH_LOG_ERROR("media_seek: default. The universe broke.");
	}
	
	if (new_offset < 0) {
		// Some error occurred.
		NYMPH_LOG_ERROR("Error during seeking.");
		new_offset = -1;
	}
	
	NYMPH_LOG_INFORMATION("New offset: " + Poco::NumberFormatter::format(new_offset));
	
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
	// Call read() with a 32 kB data request. This data is then discarded.
	if (media_read(0, buf, buf_size) == -1) {
		ct.finish();
	}
}


void FfplayDummy::cleanUp() {
	// Signal the player thread that the playback has ended.
	dummyCon.signal();
}


// --- RUN ---
void FfplayDummy::run() {
	buf = (uint8_t*) malloc(buf_size);
	
	// Start dummy playback:
	// - Request a 32 kB data block every N milliseconds.
	// - Once EOF is returned, wait N milliseconds, then quit.
	ct.setCallback(FfplayDummy::triggerRead, 0);
	ct.setStopCallback(FfplayDummy::cleanUp);
	ct.start(50 * 1000);	// Trigger time in milliseconds.
	
	// Wait here until playback has finished.
	// The read thread in StreamHandler will signal this condition variable.
	dummyMutex.lock();
	dummyCon.wait(playerMutex);
	dummyMutex.unlock();
	
	DataBuffer::reset();	// Clears the data buffer (file data buffer).
	finishPlayback();		// Calls handler for post-playback steps.
	
	// Clean up.
	free(buf);
}

 
// --- QUIT ---
void FfplayDummy::quit() {
	ct.stop();
}
