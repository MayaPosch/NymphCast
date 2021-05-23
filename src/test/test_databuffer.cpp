/*
	test_databuffer.cpp - Test runner for the DataBuffer class.
*/

#include "../server/databuffer.h"


#include <iostream>
#include <string>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>


// Globals
uint8_t lastnum = 0;
std::condition_variable dataRequestCv;
std::atomic<bool> running = true;


// --- SEEKING HANDLER ---
void seekingHandler(uint32_t session, int64_t offset) {
	//
}

void dataRequestFunction() {
	std::cout << "Starting data request function..." << std::endl;
	
	// Create and share condition variable with DataBuffer class.
	std::mutex dataRequestMtx;
	DataBuffer::setDataRequestCondition(&dataRequestCv);
	
	std::cout << "Entering data request loop..." << std::endl;
	
	while (running) {
		// Wait for the condition to be signalled.
		std::unique_lock<std::mutex> lk(dataRequestMtx);
		dataRequestCv.wait(lk);
		
		if (!running) { 
			std::cout << "Shutting down data request function..." << std::endl;
			break;
		}
				
		// Write into the buffer. We use a 10 byte pattern.
		std::string data;
		for (uint32_t i = 0; i < 10; ++i) {
			data.append(1, (char) lastnum++);
		}
		
		uint32_t wrote = DataBuffer::write(data);
		
		std::cout << "Wrote " << wrote << " \t- ";
		for (uint32_t i = 0; i < wrote; ++i) {
			std::cout << (uint16_t) data[i] << " ";
		}
		
		std::cout << std::endl;
		
		if (lastnum >= 100) {
			DataBuffer::setEof(true);
		}
	}
}


int main() {
	std::cout << "Running DataBuffer test..." << std::endl;
	
	// Create 20 byte buffer.
	DataBuffer::init(20); 
	
	// Set seek handler.
	//DataBuffer::setSeekRequestCallback(seekingHandler);
	
	// Start the data request handler in its own thread.
	std::thread drq(dataRequestFunction);
	
	// Wait for the thread to start up.
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);
	
	// The test file data is 100 bytes. We read 10 byte chunks
	uint8_t bytes[8];
	uint8_t expected = 0;
	bool abort = false;
	while (!DataBuffer::isEof()) {
		uint32_t read = DataBuffer::read(8, bytes);
		if (read == 0) {
			std::cout << "Failed to read. Aborting." << std::endl;
			break;
		}
		
		std::cout << "Read " << read << "\t- ";
		for (uint32_t i = 0; i < 8; ++i) {
			std::cout << (uint16_t) bytes[i] << " ";
			
			if (expected++ != bytes[i]) {
				abort = true;
			}
		}
		
		std::cout << std::endl;
		if (abort) {
			std::cout << "Detected mismatch. Aborting read..." << std::endl;
			break;
		}
	}
	
	std::cout << std::endl << "Test result: ";
	if (abort) {
		std::cout << "Failed." << std::endl;
	}
	else {
		std::cout << "Success." << std::endl;
	}
	
	std::cout << "Shutting down..." << std::endl;
	
	running = false;
	dataRequestCv.notify_one();
	drq.join();
	
	std::cout << "Done." << std::endl;
	
	return 0;	
}
