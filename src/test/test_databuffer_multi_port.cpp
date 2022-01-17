/*
	test_databuffer_multi_port.cpp - DataBuffer test with simultaneous read & write.
	
*/


/* #include <cstdio>

#define NYMPH_LOG_INFORMATION(msg) \
	printf("%s", msg);\
	} */


#include "../server/databuffer.h"
#include "../server/ffplaydummy.h"

#include <csignal>
#include <iostream>
#include <thread>
#include <string>
#include <atomic>

#include <Poco/Condition.h>
#include <Poco/Thread.h>
#include <Poco/Logger.h>

#include "ffplay/types.h"


std::atomic<uint32_t> audio_volume;


Logger NymphLogger::log;
Poco::Message::Priority NymphLogger::priority = Poco::Message::PRIO_INFORMATION;

Logger& NymphLogger::logger(std::string &name) {
	return log;
}


void Logger::information(std::string msg, std::string file, std::string &line) {
	std::cout << msg << ", " << file << ", " << line << std::endl;
}


void Logger::error(std::string msg, std::string file, std::string &line) {
	std::cout << msg << ", " << file << ", " << line << std::endl;
}
	

void finishPlayback() {
	// 
}

void sendGlobalStatusUpdate() {
	//
}


Poco::Condition gCon;
Poco::Mutex gMutex;

uint32_t chunk_size = 200 * 1024; // 200 kB
char* chunk;
FfplayDummy ffplay;
Poco::Thread avThread;

std::atomic<bool> running = { true };
std::atomic<bool> playerRunning = { false };
std::condition_variable dataRequestCv;
std::mutex dataRequestMtx;

uint8_t writeCnt = 0;
uint8_t readCnt = 0;


// --- DATA REQUEST FUNCTION ---
// This function can be signalled with the condition variable to request data from the client.
void dataRequestFunction() {
	// Create and share condition variable with DataBuffer class.
	DataBuffer::setDataRequestCondition(&dataRequestCv);
	
	while (running) {
		// Wait for the condition to be signalled.
		std::unique_lock<std::mutex> lk(dataRequestMtx);
		using namespace std::chrono_literals;
		dataRequestCv.wait(lk);
		
		if (!running) {
			std::cout << "Shutting down data request function..." << std::endl;
			break;
		}
		
		// Set data request as pending.
		DataBuffer::dataRequestPending = true;
		
		//std::cout << "Asking for data..." << std::endl;
	
		// Write into buffer after a brief delay.
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		
		// Create fresh chunk using counter.
		for (uint32_t i = 0; i < chunk_size; ++i) {
			chunk[i] = writeCnt++;
		}
		
		DataBuffer::write(chunk, chunk_size);
		
		if (!playerRunning) {
			// Start playback locally.
			ffplay.playTrack();
			playerRunning = true;
			
			DataBuffer::startBufferAhead();
		}
	}
}


// --- SEEKING HANDLER ---
void seekingHandler(uint32_t session, int64_t offset) {
	if (DataBuffer::seeking()) {
		std::cout << "Seeking..." << std::endl;
	
		// Write into buffer after a brief delay.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		DataBuffer::write(chunk, chunk_size);
	}
}


// --- VERIFY CALLBACK ---
void verifyCallback(uint8_t* buff, uint32_t len) {
	for (uint32_t i = 0; i < len; ++i) {
		if (buff[i] != readCnt++) {
			std::cout << " === Data Check Fail ===" << std::endl;
			std::cout << "Expected: " << (uint16_t) readCnt - 1 << 
						", got: " << (uint16_t) buff[i] << "." << std::endl;
			std::cout << "\nSequence:" << std::endl;
			std::cout.write((char*) buff, len);
			
			// Exit.
			gCon.signal();
		}
	}
	
	std::cout << "\t PASS...";
}


void signal_handler(int signal) {
	std::cout << "SIGINT handler called. Shutting down..." << std::endl;
	gCon.signal();
}


int main() {
	// Set up buffer.
	chunk = new char[chunk_size];
	
	// Init DataBuffer.
	uint32_t buffer_size = 1 * (1024 * 1024); // 1 MB
	DataBuffer::init(buffer_size);
	DataBuffer::setSeekRequestCallback(seekingHandler);
	
	// Start the data request handler in its own thread.
	std::thread drq(dataRequestFunction);
	
	// Start the player thread.
	// Start the ffplay dummy thread.
	ffplay.setVerifyCallback(verifyCallback);
	avThread.start(ffplay);
	
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	
	if (!DataBuffer::start()) {
		std::cerr << "DataBuffer start failed." << std::endl;
		return 1;
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	
	// Wait here until SIGINT.
	signal(SIGINT, signal_handler);
	
	gMutex.lock();
	gCon.wait(gMutex);
	gMutex.unlock();
	
	std::cout << "Exiting..." << std::endl;
	
	// Clean-up.
	running = false;
	ffplay.quit();
	DataBuffer::cleanup();
	dataRequestCv.notify_one();
	drq.join();
	avThread.join();
	
	delete[] chunk;
	
	return 0;
}
