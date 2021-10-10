/*
	databuffer.h - Data Buffer header.
	
	Revision 0
	
	Features:
			- Provides API for a ring buffer implementation.
			
	2020/11/19, Maya Posch
*/


#ifndef DATABUFFER_H
#define DATABUFFER_H


#include <atomic>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <queue>
#include <string>


typedef std::function<void(uint32_t, int64_t)> SeekRequestCallback;

enum DataBufferSeek {
	DB_SEEK_START = 0,
	DB_SEEK_CURRENT,
	DB_SEEK_END
};


class DataBuffer {
	enum BufferState {
		DBS_IDLE = 0,
		DBS_BUFFERING,
		DBS_SEEKING
	};
	
	static uint8_t* buffer;		// Pointer to buffer.
	static uint8_t* end;		// Pointer to buffer end (idx Nsize).
	static uint8_t* front;		// Pointer to front of data in buffer (low).
	static uint8_t* back;		// Pointer to back of data in buffer (last byte + 1).
	static uint8_t* index;		// Pointer to first unread byte or buffer start.
	static uint32_t capacity;	// Total capacity of buffer in bytes.
	static uint32_t size;		// Total size of data in buffer in bytes.
	static int64_t filesize;	// Size of the media file being streamed, in bytes.
	static std::atomic<uint32_t> unread;		// Number of unread bytes in the buffer.
	static std::atomic<uint32_t> free;		// Number of free bytes in the buffer.
	static uint32_t byteIndex;		// First unread byte index into the media file data.
	static uint32_t byteIndexLow;	// Lowest media file byte index present in the buffer.
	static uint32_t byteIndexHigh;	// Highest media file byte index present in the buffer.
	static std::mutex bufferMutex;
	static std::atomic<bool> eof;
	static std::atomic<BufferState> state;
	static SeekRequestCallback seekRequestCallback;
	static std::condition_variable* dataRequestCV;
	static std::mutex dataWaitMutex;
	static std::condition_variable dataWaitCV;
	static std::atomic<bool> dataRequestPending;
	static std::mutex seekRequestMutex;
	static std::condition_variable seekRequestCV;
	static std::atomic<bool> seekRequestPending;
	static uint32_t sessionHandle;		// Active session this buffer is associated with.
	
	static std::mutex streamTrackQueueMutex;
	static std::queue<std::string> streamTrackQueue;
	
public:
	static bool init(uint32_t capacity);
	static bool cleanup();
	static void setSeekRequestCallback(SeekRequestCallback cb);
	static void setDataRequestCondition(std::condition_variable* condition);
	static void setSessionHandle(uint32_t handle);
	static uint32_t getSessionHandle();
	static void setFileSize(int64_t size);
	static int64_t getFileSize();
	static bool start();
	static void requestData();
	static bool reset();
	static int64_t seek(DataBufferSeek mode, int64_t offset);
	static bool seeking();
	static uint32_t read(uint32_t len, uint8_t* bytes);
	static uint32_t write(std::string &data);
	static uint32_t write(const char* data, uint32_t length);
	static void setEof(bool eof);
	static bool isEof();
	
	static void addStreamTrack(std::string track);
	static bool hasStreamTrack();
	static std::string getStreamTrack();
};

#endif

