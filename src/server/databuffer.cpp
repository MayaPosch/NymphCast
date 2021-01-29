/*
	databuffer.cpp - Implementation of the DataBufer class.
	
	Revision 0.
	
	Notes:
			- 
			
	2020/11/19, Maya Posch
*/


#define DEBUG 1

#include "databuffer.h"

#include <cstring>
#include <chrono>
#ifdef DEBUG
#include <iostream>
/* #include <fstream>
std::ofstream outFile("out.mp3", std::ofstream::out | std::ofstream::binary); */
#endif


// Static initialisations.
uint8_t* DataBuffer::buffer = 0;
uint8_t* DataBuffer::begin = 0;
uint8_t* DataBuffer::end = 0;
uint8_t* DataBuffer::front = 0;
uint8_t* DataBuffer::back = 0;
uint8_t* DataBuffer::index = 0;
uint32_t DataBuffer::capacity = 0;
uint32_t DataBuffer::size = 0;
int64_t DataBuffer::filesize = 0;
uint32_t DataBuffer::unread = 0;
uint32_t DataBuffer::unreadLow = 0;
uint32_t DataBuffer::unreadHigh = 0;
uint32_t DataBuffer::bytesFreeLow = 0;
uint32_t DataBuffer::bytesFreeHigh = 0;
uint32_t DataBuffer::byteIndex = 0;
uint32_t DataBuffer::byteIndexLow = 0;
uint32_t DataBuffer::byteIndexHigh = 0;
std::atomic<bool> DataBuffer::eof = false;
std::atomic<DataBuffer::BufferState> DataBuffer::state;
std::mutex DataBuffer::bufferMutex;
SeekRequestCallback DataBuffer::seekRequestCallback = 0;
std::condition_variable* DataBuffer::dataRequestCV = 0;
std::mutex DataBuffer::dataWaitMutex;
std::condition_variable DataBuffer::dataWaitCV;
std::atomic<bool> DataBuffer::dataRequestPending = false;
std::mutex DataBuffer::seekRequestMutex;
std::condition_variable DataBuffer::seekRequestCV;
std::atomic<bool> DataBuffer::seekRequestPending = false;
uint32_t DataBuffer::sessionHandle = 0;

std::mutex DataBuffer::streamTrackQueueMutex;
std::queue<std::string> DataBuffer::streamTrackQueue;


// --- INIT ---
// Initialises new data buffer. Capacity is provided in bytes.
// Returns false on error, otherwise true.
bool DataBuffer::init(uint32_t capacity) {
	bufferMutex.lock();
	if (buffer != 0) {
		// An existing buffer exists. Erase it first.
		delete[] buffer;
	}
	
	// Allocate new buffer and return result.
	buffer = new uint8_t[capacity];
	DataBuffer::capacity = capacity;
	begin = buffer;
	end = begin + capacity;
	front = begin;
	back = begin;
	index = begin;
	eof = false;
	size = 0;
	unread = 0;
	unreadLow = 0;
	unreadHigh = 0;
	bytesFreeLow = 0;
	bytesFreeHigh = capacity;
	byteIndex = 0;
	byteIndexLow = 0;
	byteIndexHigh = 0;
	state = DBS_IDLE;
	bufferMutex.unlock();
	
	return true;
}


// --- CLEAN UP ---
// Clean up resources, delete the buffer.
bool DataBuffer::cleanup() {
	if (buffer != 0) {
		delete[] buffer;
		buffer = 0;
	}
	
	return true;
}


// --- SET SEEK REQUEST CALLBACK ---
void DataBuffer::setSeekRequestCallback(SeekRequestCallback cb) {
	seekRequestCallback = cb;
}


// --- SET DATA REQUEST CONDITION ---
void DataBuffer::setDataRequestCondition(std::condition_variable* condition) {
	dataRequestCV = condition;
}


// -- SET SESSION HANDLE ---
void DataBuffer::setSessionHandle(uint32_t handle) {
	sessionHandle = handle;
}


// --- GET SESSION HANDLE ---
uint32_t DataBuffer::getSessionHandle() {
	return sessionHandle;
}


// --- SET FILE SIZE ---
void DataBuffer::setFileSize(int64_t size) {
	filesize = size;
}


// --- GET FILE SIZE ---
int64_t DataBuffer::getFileSize() {
	return filesize;
}


// --- START ---
// Starts calling the data request handler to obtain data.
bool DataBuffer::start() {
	if (dataRequestCV == 0) { return false; }
	
	dataRequestPending = true;
	dataRequestCV->notify_one();
	
	return true;
}


// --- REQUEST DATA ---
void DataBuffer::requestData() {
	if (dataRequestCV == 0) { return; }
	
	// Trigger a data request from the client.
	dataRequestPending = true;
	dataRequestCV->notify_one();
	
	// Wait until we have received data or time out.
	std::unique_lock<std::mutex> lk(dataWaitMutex);
	using namespace std::chrono_literals;
	while (dataWaitCV.wait_for(lk, 150ms) != std::cv_status::timeout) {
		if (!dataRequestPending) { break; }
	}
}


// --- RESET ---
// Reset the buffer to the initialised state. This leaves the existing allocated buffer intact, 
// but erases its contents.
bool DataBuffer::reset() {
	front = begin;
	back = begin;
	size = 0;
	index = begin;
	unread = 0;
	unreadLow = 0;
	unreadHigh = 0;
	bytesFreeLow = 0;
	bytesFreeHigh = capacity;
	byteIndex = 0;
	byteIndexLow = 0;
	byteIndexHigh = 0;
	eof = false;
	state = DBS_IDLE;
	dataRequestPending = false;
	seekRequestPending = false;
	
	return true;
}


// --- SEEK ---
// Seek to a specific point in the data.
// Returns the new absolute byte position in the file, or -1 in case of failure.
int64_t DataBuffer::seek(DataBufferSeek mode, int64_t offset) {
#ifdef DEBUG
	std::cout << "DataBuffer::seek: mode " << mode << ", offset: " << offset << std::endl;
#endif
	
	// Calculate absolute byte index.
	int64_t new_offset = -1;
	if 		(mode == DB_SEEK_START)		{ new_offset = offset; }
	else if (mode == DB_SEEK_CURRENT) 	{ new_offset = byteIndex + offset; }
	else if (mode == DB_SEEK_END)		{ new_offset = filesize - offset; }
	
#ifdef DEBUG
	std::cout << "New offset: " << new_offset << std::endl;
#endif

	// Ensure that the new offset isn't past the end of the file. If so, return -1.
	if (new_offset > filesize) {
#ifdef DEBUG
		std::cout << "New offset larger than file size. Returning -1." << std::endl;
#endif
		return -1;
	}
	
	// Check whether we have the requested data in the buffer.
	if (new_offset < byteIndexLow || new_offset > byteIndexHigh) {
#ifdef DEBUG
	/* if (outFile.is_open()) {
		outFile.close();
	}
	
	outFile.open("out.mp3", std::ios::out | std::ios::binary | std::ios::trunc); */
#endif
		// Data is not in buffer. Reset buffer and send seek request to client.
		reset();
		byteIndexLow = new_offset;
		byteIndexHigh = new_offset;
		if (seekRequestCallback == 0) { return -1; }
		seekRequestPending = true;
		state = DBS_SEEKING;
		seekRequestCallback(sessionHandle, new_offset);
		
		// Wait for response.
		std::unique_lock<std::mutex> lk(seekRequestMutex);
		using namespace std::chrono_literals;
		while (!seekRequestPending) {
			std::cv_status stat = seekRequestCV.wait_for(lk, 150ms);
			if (stat == std::cv_status::timeout) { return -1; }
		}
		
		state = DBS_IDLE;
	}
	else {	
#ifdef DEBUG
		std::cout << "Setting new buffer position." << std::endl;
#endif

		// Set the new position in the buffer.
		uint32_t oldUnread = unread;
		byteIndex = new_offset;							// Absolute byte index.
		index = front + (new_offset - byteIndexLow);	// Index into buffer translation.
		unread = byteIndexHigh - new_offset;
		unreadLow += new_offset - byteIndexLow;
		unreadHigh -= unread - oldUnread;
	}
	
	return new_offset;
}


// --- SEEKING ---
bool DataBuffer::seeking() {
	return (state == DBS_SEEKING);
}


// --- READ ---
// Try to read 'len' bytes from the buffer, into the provided buffer.
// Returns the number of bytes read, or 0 in case of an error.
uint32_t DataBuffer::read(uint32_t len, uint8_t* bytes) {
#ifdef DEBUG
	std::cout << "DataBuffer::read: len " << len << ". EOF: " << eof << std::endl;
#endif

	// Request more data if the buffer does not have enough unread data left, and EOF condition
	// has not been reached.
	if (!eof && len > unread) {
		// More data should be available on the client, try to request it.
#ifdef DEBUG
		std::cout << "Requesting more data..." << std::endl;
#endif
		requestData();
	}
	
	if (unread == 0 && eof) {
#ifdef DEBUG
		std::cout << "Reached EOF." << std::endl;
#endif
		return 0;
	}
	
	bufferMutex.lock();
	uint32_t bytesRead = 0;
	if (len <= unreadHigh) {
#ifdef DEBUG
		std::cout << "Read whole block." << std::endl;
#endif
		// Read requested data in one chunk from the buffer back.
		memcpy(bytes, index, len);
		
		// Update counters, etc.
		index += len;
		unreadHigh -= len;
		bytesRead += len;
		bytesFreeLow += len;
		
		// Read from the front if the back is exhausted.
		if (unreadHigh == 0 && bytesFreeHigh == 0) { index = begin; } 
	}
	else if (unreadHigh > 0 && bytesFreeHigh > 0) {
		// Read the bytes we have got from the back in the buffer.
#ifdef DEBUG
		std::cout << "Read data from back." << std::endl;
#endif
		memcpy(bytes, index, unreadHigh);
		index += unreadHigh;
		bytesRead += unreadHigh;
		bytesFreeLow += unreadHigh;
		unreadHigh = 0;
	}
	else if (unreadHigh == 0 && unreadLow > 0) {
		// Read what we need from the front.
		if (len <= unreadLow) {
			// Read the remaining requested bytes in one chunk.
			memcpy(bytes, index, len);
			index += len;
			bytesRead += len;
			unreadLow -= len;
		}
		else {
			// Not enough bytes left in the buffer. Read what we can, then return.
			memcpy(bytes, index, unreadLow);
			index += unreadLow;
			bytesRead += unreadLow;
			unreadLow = 0;
		}
	}
	else {
#ifdef DEBUG
		std::cout << "Read back, then front." << std::endl;
#endif
		// Read the remaining unread bytes from the back, then continue reading from the front.
		memcpy(bytes, index, unreadHigh);
		index = begin;
		bytesRead += unreadHigh;
		bytesFreeLow += unreadHigh;
		unreadHigh = 0;
		uint32_t bytesToRead = len - bytesRead;
		if (bytesToRead <= unreadLow) {
			// Read the remaining requested bytes in one chunk.
			memcpy(bytes + bytesRead, index, bytesToRead);
			index += bytesToRead;
			bytesRead += bytesToRead;
			unreadLow -= bytesToRead;
		}
		else {
			// Not enough bytes left in the buffer. Read what we can, then return.
			memcpy(bytes + bytesRead, index, unreadLow);
			index += unreadLow;
			bytesRead += unreadLow;
			unreadLow = 0;
		}
	}
	
	// Update counters.
	unread -= bytesRead;
	byteIndex += bytesRead;
	size -= bytesRead;
	
#ifdef DEBUG
		std::cout << "unread " << unread << ", byteIndex " << byteIndex << std::endl;
		std::cout << "bytesRead: " << bytesRead << std::endl;
		std::cout << "unreadLow: " << unreadLow << ", unreadHigh: " << unreadHigh
					<< ", bytesFreeHigh: " << bytesFreeHigh << ", bytesFreeLow: " << bytesFreeLow
					<< std::endl;
#endif
	
	bufferMutex.unlock();
	
	return bytesRead;
}


// --- WRITE ---
// Write data into the buffer.
uint32_t DataBuffer::write(std::string &data) {
/* #ifdef DEBUG
		std::cout << "DataBuffer::write called for bytes: " << data.length() << std::endl;
		outFile.write(data.data(), data.length());
#endif */
	// First check whether we can perform a straight copy. For this we need enough available bytes
	// at the end of the buffer. Else we have to attempt to write the remainder into the front of
	// the buffer.
	bufferMutex.lock();
	dataRequestPending = false;
	uint32_t bytesWritten = 0;
	if (data.length() <= bytesFreeHigh) {
#ifdef DEBUG
		std::cout << "Write whole chunk at back. BytesFreeHigh: " << bytesFreeHigh << std::endl;
#endif
		// Copy the data into the buffer.
		memcpy(back, data.data(), data.length());
		bytesWritten = data.length();
		bytesFreeHigh -= data.length();
		unreadHigh += data.length();
		
		// Move back pointer to just beyond the newly written data.
		back += bytesWritten;
		if (bytesFreeHigh == 0) {
			back = begin;
		}
	}
	else if (bytesFreeHigh == 0) {
#ifdef DEBUG
		std::cout << "Write front." << std::endl;
#endif
		if (data.length() > bytesFreeLow) {
			// Buffer is too small for remaining bytes. Copy what we can.
			memcpy(back, data.data(), bytesFreeLow);
			bytesWritten += bytesFreeLow;
			unreadLow += bytesFreeLow;
			byteIndexLow += bytesFreeLow;
			back += bytesFreeLow;
			bytesFreeLow = 0;
		}
		else {
			// Straight copy into buffer.
			memcpy(back, data.data(), data.length());
			bytesWritten = data.length();
			unreadLow += bytesWritten;
			bytesFreeLow -= bytesWritten;
			byteIndexLow += bytesWritten;
			back += bytesWritten;
		}
	}
	else {
#ifdef DEBUG
		std::cout << "Write back, then front." << std::endl;
#endif
		// Copy what we can, then try to copy the rest into the beginning of the buffer
		memcpy(back, data.data(), bytesFreeHigh);
		bytesWritten = bytesFreeHigh;
		back = begin; // Buffer loops back to the front after it fills up.
		
		uint32_t bytesToWrite = data.length() - bytesFreeHigh;
		unreadHigh += bytesFreeHigh;
		bytesFreeHigh = 0;
		if (bytesToWrite > bytesFreeLow) {
			// Buffer is too small for remaining bytes. Copy what we can.
			memcpy(back, data.data() + bytesWritten, bytesFreeLow);
			bytesWritten += bytesFreeLow;
			unreadLow += bytesFreeLow;
			byteIndexLow += bytesFreeLow;
			bytesFreeLow = 0;
		}
		else {
			// Straight copy into buffer.
			memcpy(back, data.data() + bytesWritten, bytesToWrite);
			bytesWritten += bytesToWrite;
			unreadLow += bytesToWrite;
			bytesFreeLow -= bytesToWrite;
			byteIndexLow += bytesToWrite;
		}
	}
	
#ifdef DEBUG
		std::cout << "unreadLow: " << unreadLow << ", unreadHigh: " << unreadHigh
					<< ", bytesFreeHigh: " << bytesFreeHigh << ", bytesFreeLow: " << bytesFreeLow
					<< std::endl;
#endif
		
	// Update counters.
	size += bytesWritten;
	unread += bytesWritten;
	byteIndexHigh += bytesWritten;
	
#ifdef DEBUG
		std::cout << "size: " << size << ", unread: " << unread << ", byteIndexHigh: "
					<< byteIndexHigh << ", bytesWritten: " << bytesWritten << std::endl;
#endif
	
	bufferMutex.unlock();
	
	// If we're in seeking mode, signal that we're done.
	if (state == DBS_SEEKING) {
		seekRequestPending = false;
		seekRequestCV.notify_one();
	}
	
	return bytesWritten;
}


// --- SET EOF ---
// Set the End-Of-File status of the file being streamed.
void DataBuffer::setEof(bool eof) {
	DataBuffer::eof = eof;
/* #ifdef DEBUG
	if (eof == true) {
		outFile.close();
	}
#endif */
}


// --- IS EOF ---
bool DataBuffer::isEof() {
	return DataBuffer::eof;
}


// --- ADD STREAM TRACK ---
// Add a streaming track to the queue.
void DataBuffer::addStreamTrack(std::string track) {
	streamTrackQueueMutex.lock();
	streamTrackQueue.push(track);
	streamTrackQueueMutex.unlock();
}


// --- HAS STREAM TRACK ---
bool DataBuffer::hasStreamTrack() {
	return !streamTrackQueue.empty();
}


// --- GET STREAM TRACK ---
// Returns the next stream string in the queue, or an empty string if queue is empty.
std::string DataBuffer::getStreamTrack() {
	streamTrackQueueMutex.lock();
	if (streamTrackQueue.empty()) { return std::string(); }
	std::string tStr = streamTrackQueue.front();
	streamTrackQueue.pop();
	streamTrackQueueMutex.unlock();
	
	return tStr;
}
