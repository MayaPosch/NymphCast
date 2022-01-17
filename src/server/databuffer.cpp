/*
	databuffer.cpp - Implementation of the DataBufer class.
	
	Revision 0.
	
	Notes:
			- 
			
	2020/11/19, Maya Posch
*/


//#define DEBUG 1

#include "databuffer.h"

#include <cstring>
#include <chrono>
#include <thread>
#ifdef DEBUG
#include <iostream>
#endif

// Enable profiling.
//#define PROFILING_DB 1
#ifdef PROFILING_DB
#include <chrono>
#include <fstream>
std::ofstream db_debugfile;
uint32_t profcount = 0;

//#include <cstdio>
std::ofstream db_readsfile;
//FILE* db_readsfile;
#endif


// Static initialisations.
uint8_t* DataBuffer::buffer = 0;
uint8_t* DataBuffer::end = 0;
uint8_t* DataBuffer::front = 0;
uint8_t* DataBuffer::back = 0;
uint8_t* DataBuffer::index = 0;
uint32_t DataBuffer::capacity = 0;
uint32_t DataBuffer::size = 0;
int64_t DataBuffer::filesize = 0;
std::atomic<uint32_t> DataBuffer::unread = { 0 };
std::atomic<uint32_t> DataBuffer::free = { 0 };
uint32_t DataBuffer::byteIndex = 0;
uint32_t DataBuffer::byteIndexLow = 0;
uint32_t DataBuffer::byteIndexHigh = 0;
std::atomic<bool> DataBuffer::eof = { false };
std::atomic<DataBuffer::BufferState> DataBuffer::state;
std::mutex DataBuffer::bufferMutex;
SeekRequestCallback DataBuffer::seekRequestCallback = 0;
std::condition_variable* DataBuffer::dataRequestCV = 0;
std::mutex DataBuffer::dataWaitMutex;
std::condition_variable DataBuffer::dataWaitCV;
std::atomic<bool> DataBuffer::dataRequestPending = { false };
std::mutex DataBuffer::seekRequestMutex;
std::condition_variable DataBuffer::seekRequestCV;
std::atomic<bool> DataBuffer::seekRequestPending = { false };
std::atomic<bool> DataBuffer::resetRequest = { false };
std::atomic<bool> DataBuffer::writeStarted = { false };
std::atomic<bool> DataBuffer::bufferAhead = { false };
uint32_t DataBuffer::sessionHandle = 0;

std::mutex DataBuffer::streamTrackQueueMutex;
std::queue<std::string> DataBuffer::streamTrackQueue;


// --- INIT ---
// Initialises new data buffer. Capacity is provided in bytes.
// Returns false on error, otherwise true.
bool DataBuffer::init(uint32_t capacity) {
	if (buffer != 0) {
		// An existing buffer exists. Erase it first.
		delete[] buffer;
	}
	
	// Allocate new buffer and return result.
	buffer = new uint8_t[capacity];
	DataBuffer::capacity = capacity;
	
	end = buffer + capacity;
	front = buffer;
	back = buffer;
	index = buffer;
	
	size = 0;
	unread = 0;
	free = capacity;
	
	byteIndex = 0;
	byteIndexLow = 0;
	byteIndexHigh = 0;
	
	eof = false;
	dataRequestPending = false;
	seekRequestPending = false;
	resetRequest = false;
	writeStarted = false;
	bufferAhead = false;
	state = DBS_IDLE;
	
#ifdef PROFILING_DB
	if (!db_debugfile.is_open()) {
		db_debugfile.open("profiling_databuffer.txt");
	}
	
	if (!db_readsfile.is_open()) {
		db_readsfile.open("db_reads.txt", std::ios::out | std::ios::trunc | std::ios::binary);
	}
	//db_readsfile = fopen("db_reads.txt", "wb");
#endif
	
	return true;
}


// --- CLEAN UP ---
// Clean up resources, delete the buffer.
bool DataBuffer::cleanup() {
	if (buffer != 0) {
		delete[] buffer;
		buffer = 0;
	}
	
#ifdef PROFILING_DB
	if (db_debugfile.is_open()) {
		db_debugfile.flush();
		db_debugfile.close();
	}
	
	if (db_readsfile.is_open()) {
		db_readsfile.flush();
		db_readsfile.close();
	}
	//fclose(db_readsfile);
#endif
	
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
	
	bufferAhead = false;
	writeStarted = true;
	dataRequestCV->notify_one();
	
	return true;
}


// --- REQUEST DATA ---
void DataBuffer::requestData() {
	if (dataRequestCV == 0) { return; }
	
	// Trigger a data request from the client.
	dataRequestCV->notify_one();
	
	// Wait until we have received data or time out.
	std::unique_lock<std::mutex> lk(dataWaitMutex);
	using namespace std::chrono_literals;
	uint32_t timeout = 5000;
	while (1) { 
		dataWaitCV.wait_for(lk, 100us);
		if (!dataRequestPending) { break; }
		if (--timeout == 0) {
#ifdef DEBUG
			std::cerr << "RequestData timeout after 500 ms." << std::endl;
#endif
			break;
		}
	}
}


// --- RESET ---
// Reset the buffer to the initialised state. This leaves the existing allocated buffer intact, 
// but erases its contents.
bool DataBuffer::reset() {
	front = buffer;
	back = buffer;
	size = 0;
	index = buffer;
	
	unread = 0;
	free = capacity;
	
	byteIndex = 0;
	byteIndexLow = 0;
	byteIndexHigh = 0;
	
	//eof = false;
	dataRequestPending = false;
	seekRequestPending = false;
	resetRequest = false;
	state = DBS_IDLE;
	
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
	else if (mode == DB_SEEK_END)		{ new_offset = filesize - offset - 1; }
	
#ifdef DEBUG
	std::cout << "New offset: " << new_offset << std::endl;
	std::cout << "ByteIndex: " << byteIndex << std::endl;
#endif

	// Ensure that the new offset isn't past the beginning/end of the file. If so, return -1.
	if (new_offset > filesize || new_offset < 0) {
#ifdef DEBUG
		std::cout << "New offset larger than file size or negative. Returning -1." << std::endl;
#endif
		return -1;
	}
	
	// Ensure we're not in the midst of a data request action.
	while (dataRequestPending) {
		// Sleep in 1 ms segments until the data request is done.
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
#ifdef PROFILING_DB
	// Truncate file.
	if (db_readsfile.is_open()) {
		db_readsfile.close();
		db_readsfile.open("db_reads.txt", std::ios::out | std::ios::trunc | std::ios::binary);
	}
	//fclose(db_readsfile);
	//db_readsfile = fopen("db_reads.txt", "wb");
#endif
	
	// TODO: removing local check. We just assume the local data isn't in the buffer and reload.
	// In testing, the local data check has offered little benefits (not enough data in buffer).
	// Check that this feature can be fully removed, or maybe reimplemented.
	
	// Check whether we have the requested data in the buffer.
	//if (new_offset < byteIndexLow || new_offset > byteIndexHigh) {
		// Data is not in buffer. Reset buffer and send seek request to client.
		/* if (writeStarted) {
			resetRequest = true;
			while (resetRequest) {
				// Wait for the write thread to acknowledge the request.
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		} */
		
		reset();
		byteIndexLow = (uint32_t) new_offset;
		byteIndexHigh = (uint32_t) new_offset;
		if (seekRequestCallback == 0) { return -1; }
		seekRequestPending = true;
		state = DBS_SEEKING;
		seekRequestCallback(sessionHandle, new_offset);
		
		// Wait for response.
		std::unique_lock<std::mutex> lk(seekRequestMutex);
		using namespace std::chrono_literals;
		while (seekRequestPending) {
			std::cv_status stat = seekRequestCV.wait_for(lk, 1s);
			if (stat == std::cv_status::timeout) {
#ifdef DEBUG
				std::cout << "Time-out on seek request. Returning -1." << std::endl;
#endif
				return -1; 
			}
		}
		
		state = DBS_IDLE;
	/* }
	else {	
#ifdef DEBUG
		std::cout << "Setting new buffer position." << std::endl;
#endif

		// Set the new position in the buffer.
		uint32_t oldUnread = unread;
		//byteIndex = new_offset;							// Absolute byte index.
		index = front + (new_offset - byteIndexLow);	// Index into buffer translation.
		unread = byteIndexHigh - new_offset;
		//unreadLow += new_offset - byteIndexLow;
		//unreadHigh -= unread - oldUnread;
	} */
	
	byteIndex = (uint32_t) new_offset;
	
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
		
#ifdef PROFILING_DB
	std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
#endif

	// Request more data if the buffer does not have enough unread data left, and EOF condition
	// has not been reached.
	if (!eof && len > unread) {
		// More data should be available on the client, try to request it.
#ifdef DEBUG
		std::cout << "Requesting more data..." << std::endl;
#endif

#ifdef PROFILING_DB
		db_debugfile << "Requesting more data... Unread: " << unread << ".\n";
#endif
		requestData();
	}
	
	if (unread == 0) {
		if (eof) {
#ifdef DEBUG
			std::cout << "Reached EOF." << std::endl;
#endif
			return 0;
		}
		else {
#ifdef DEBUG
			std::cout << "Read failed due to empty buffer." << std::endl;
#endif
			return 0;
		}
	}
	
	uint32_t bytesRead = 0;
	
	// Determine the number of bytes we can read in one copy operation.
	// This depends on the location of the write pointer ('back') compared to the 
	// read pointer ('index'). If the write pointer is ahead of the read pointer, we can read up 
	// till there, otherwise to the end of the buffer.
	uint32_t locunread = unread;
	uint32_t bytesSingleRead = locunread;
	if ((end - index) < bytesSingleRead) { bytesSingleRead = end - index; } // Unread section wraps around.
	
#ifdef DEBUG
	std::cout << "bytesSingleRead: " << bytesSingleRead << std::endl;
#endif
	
	if (len <= bytesSingleRead) {
		// Can read requested data in single chunk.
#ifdef DEBUG
		std::cout << "Read whole block." << std::endl;
		std::cout << "Index: " << index - buffer << ", Back: " << back - buffer << std::endl;
#endif
		memcpy(bytes, index, len);
		index += len;		// Advance read pointer.
		bytesRead += len;
		byteIndex += len;
		unread -= len;		// Unread bytes decreases by read byte count.
		free += len;		// Read bytes become free for overwriting.
		
		if (index >= end) {
			index = buffer;	// Read pointer went past the buffer end. Reset to buffer begin.
		}

#ifdef PROFILING_DB
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		db_debugfile << "SR. Duration: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "µs. ";
#endif
	}
	else if (bytesSingleRead > 0 && locunread == bytesSingleRead) {
		// Less data in buffer than needed & nothing at the front.
		// Read what we can from the back, then return.
#ifdef DEBUG
		std::cout << "Read partial data from back." << std::endl;
		std::cout << "Index: " << index - buffer << ", Back: " << back - buffer << std::endl;
#endif
		memcpy(bytes, index, bytesSingleRead);
		index += bytesSingleRead;		// Advance read pointer.
		bytesRead += bytesSingleRead;
		byteIndex += bytesSingleRead;
		unread -= bytesSingleRead;		// Unread bytes decreases by read byte count.
		free += bytesSingleRead;		// Read bytes become free for overwriting.
		
		if (index >= end) {
			index = buffer;	// Read pointer went past the buffer end. Reset to buffer begin.
		}

#ifdef PROFILING_DB
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		db_debugfile << "PR. Duration: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "µs.";
#endif
	}
	else if (bytesSingleRead > 0 && locunread > bytesSingleRead) {
		// Read part from the end of the buffer, then read rest from the front.
#ifdef DEBUG
		std::cout << "Read back, then front." << std::endl;
		std::cout << "Index: " << index - buffer << ", Back: " << back - buffer << std::endl;
#endif
		memcpy(bytes, index, bytesSingleRead);
		index += bytesSingleRead;		// Advance read pointer.
		bytesRead += bytesSingleRead;
		byteIndex += bytesSingleRead;
		unread -= bytesSingleRead;		// Unread bytes decreases by read byte count.
		free += bytesSingleRead;		// Read bytes become free for overwriting.
		
		index = buffer;	// Switch read pointer to front of the buffer.
		
		// Read remainder from front.
		uint32_t bytesToRead = len - bytesRead;
#ifdef DEBUG
		std::cout << "bytesRead: " << bytesRead << ", bytesToRead: " << bytesToRead << std::endl;
#endif
		if (bytesToRead <= locunread) {
			// Read the remaining bytes we need.
			memcpy(bytes + bytesRead, index, bytesToRead);
			index += bytesToRead;
			bytesRead += bytesToRead;
			byteIndex += bytesToRead;
			unread -= bytesToRead;
			free += bytesToRead;
		}
		else {
			// Read the unread bytes still available in the buffer.
			memcpy(bytes + bytesRead, index, locunread);
			index += locunread;
			bytesRead += locunread;
			byteIndex += locunread;
			unread -= locunread;
			free += locunread;
		}

#ifdef PROFILING_DB
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		db_debugfile << "FB. Duration: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "µs.";
#endif
	}
	else {
		// Default case.
#ifdef DEBUG
		std::cout << "FIXME: Default case." << std::endl;
#endif
		// FIXME: This shouldn't happen.
		
	}
	
	// Trigger a data request from the client if we have space.
	if (eof) {
		// Do nothing.
	}
	else if (bufferAhead && !dataRequestPending && free > 204799) {
		// Single block is 200 kB (204,800 bytes). We have space, so request another block.
		// TODO: make it possible to request a specific block size from client.
		if (dataRequestCV != 0) {
#ifdef DEBUG
			std::cout << "DataBuffer::read: requesting more data." << std::endl;
#endif
			dataRequestCV->notify_one();
		}
	}

#ifdef PROFILING_DB
		std::chrono::high_resolution_clock::time_point end2 = std::chrono::high_resolution_clock::now();
		db_debugfile << "\t\tDuration2: " << std::chrono::duration_cast<std::chrono::microseconds>(end2 - begin).count() << "µs.\n";
		
		// Write read bytes to file.
		db_readsfile.write((char*) bytes, bytesRead);
		//fwrite(bytes, sizeof(uint8_t), bytesRead, db_readsfile);
#endif
	
#ifdef DEBUG
	std::cout << "unread " << unread << ", free " << free << std::endl;
	std::cout << "bytesRead: " << bytesRead << std::endl;
#endif
	
	return bytesRead;
}


// --- WRITE ---
// Write data into the buffer.
uint32_t DataBuffer::write(std::string &data) {
	return write(data.data(), data.length());
}


uint32_t DataBuffer::write(const char* data, uint32_t length) {
#ifdef DEBUG
	std::cout << "DataBuffer::write: len " << length << std::endl;
	std::cout << "Index: " << index - buffer << ", Back: " << back - buffer << std::endl;
#endif

	// First check whether we can perform a straight copy. For this we need enough available bytes
	// at the end of the buffer. Else we have to attempt to write the remainder into the front of
	// the buffer.
	// The bytesFreeLow and bytesFreeHigh counters are for keeping track of the number of free bytes
	// at the low (beginning) and high (end) side respectively.
	//bufferMutex.lock();
	uint32_t bytesWritten = 0;
	
	// Determine the number of bytes we can write in one copy operation.
	// This depends on the number of 'free' bytes, and the location of the read pointer ('index') 
	// compared to the  write pointer ('back'). If the read pointer is ahead of the write pointer, 
	// we can write up till there, otherwise to the end of the buffer.
	uint32_t locfree = free;
	uint32_t bytesSingleWrite = locfree;
	if ((end - back) < bytesSingleWrite) { bytesSingleWrite = end - back; }
	
	if (length <= bytesSingleWrite) {
#ifdef DEBUG
		std::cout << "Write whole chunk at back. Single write: " << bytesSingleWrite << std::endl;
#endif
		// Enough space to write the data in one go.
		memcpy(back, data, length);
		bytesWritten = length;
		back += bytesWritten;
		unread += bytesWritten;
		free -= bytesWritten;
		
		if (back >= end) {
			back = buffer;
		}
	}
	else if (bytesSingleWrite > 0 && locfree == bytesSingleWrite) {
#ifdef DEBUG
		std::cout << "Partial write at back. Single write: " << bytesSingleWrite << std::endl;
#endif
		// Only enough space in buffer to write to the back. Write what we can, then return.
		memcpy(back, data, bytesSingleWrite);
		bytesWritten = bytesSingleWrite;
		back += bytesWritten;
		unread += bytesWritten;
		free -= bytesWritten;
		
		if (back >= end) {
			back = buffer;
		}
	}
	else if (bytesSingleWrite > 0 && locfree > bytesSingleWrite) {
#ifdef DEBUG
		std::cout << "Partial write at back, rest at front. Single write: " << bytesSingleWrite << std::endl;
#endif
		// Write to the back, then the rest at the front.
		memcpy(back, data, bytesSingleWrite);
		bytesWritten = bytesSingleWrite;
		unread += bytesWritten;
		free -= bytesWritten;
		
		back = buffer;
		
		// Write remainder at the front.
		uint32_t bytesToWrite = length - bytesWritten;
#ifdef DEBUG
	std::cout << "Write remainder: " << bytesToWrite << std::endl;
	std::cout << "Index: " << index - buffer << ", Back: " << back - buffer << std::endl;
#endif
		if (bytesToWrite <= locfree) {
			// Write the remaining bytes we have.
			memcpy(back, data + bytesWritten, bytesToWrite);
			bytesWritten += bytesToWrite;
			unread += bytesToWrite;
			free -= bytesToWrite;
			back += bytesToWrite;
		}
		else {
			// Write the unread bytes still available in the buffer.
			memcpy(back, data + bytesWritten, locfree);
			bytesWritten += locfree;
			unread += locfree;
			free -= locfree;
			back += locfree;
		}
	}
	else {
		// FIXME: shouldn't happen.
#ifdef DEBUG
		std::cout << "FIXME: Default case." << std::endl;
#endif
	}
	
#ifdef DEBUG
		std::cout << "unread: " << unread << ", free: " 
					<< free << ", bytesWritten: " << bytesWritten << std::endl;
#endif
	
	// If we're in seeking mode, signal that we're done.
	if (state == DBS_SEEKING) {
#ifdef DEBUG
		std::cout << "In seeking mode. Notifying seeking routine." << std::endl;
#endif
		seekRequestPending = false;
		dataRequestPending = false;
		seekRequestCV.notify_one();
		
		return bytesWritten;
	}
	
	dataRequestPending = false;
	
	// Trigger a data request from the client if we have space.
	if (eof) {
		// Do nothing.
	}/* 
	else if (resetRequest) {
		// Reset request is pending. Ensure no data requests are pending before we signal okay.
		while (dataRequestPending) {
			// Sleep in 1 ms segments until the data request is done.
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		
		resetRequest = false;
	} */
	else if (bufferAhead && free > 204799) {
		// Single block is 200 kB (204,800 bytes). We have space, so request another block.
		// TODO: make it possible to request a specific block size from client.
		if (dataRequestCV != 0) {
#ifdef DEBUG
			std::cout << "DataBuffer::write: requesting more data." << std::endl;
#endif
			dataRequestCV->notify_one();
		}
	}
	
	return bytesWritten;
}


// --- SET EOF ---
// Set the End-Of-File status of the file being streamed.
void DataBuffer::setEof(bool eof) {
	DataBuffer::eof = eof;
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
