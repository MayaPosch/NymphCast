/*
	bytebauble.cpp - Implementation of the ByteBauble library.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
			
	2019/05/31, Maya Posch
*/


#include "bytebauble.h"

#include <iostream>


// --- CONSTRUCTOR ---
ByteBauble::ByteBauble() {
	// Perform host endianness detection.
	detectHostEndian();
}


// --- DETECT HOST ENDIAN ---
// Perform a check for the endianness of the host system.
void ByteBauble::detectHostEndian() {
	// This check uses a single 16-bit integer, using it as a single byte after assignment.
	// Depending on which byte the (<255) value ended up on, we can determine the host endianness.
	uint16_t bytes = 1;
	if (*((uint8_t*) &bytes) == 1) {
		// Detected little endian.
		std::cout << "Detected Host Little Endian." << std::endl;
		hostEndian = BB_LE;
	}
	else {
		// Detected big endian.
		std::cout << "Detected Host Big Endian." << std::endl;
		hostEndian = BB_BE;
	}
}


// --- READ PACKED INT ---
// Packed integer format that uses the MSB of each byte to indicate that another byte follows.
// This method supports the input of a single unsigned 32-bit integer.
// Second parameter and output is a 32-bit integer, of which at most 28 bits can contain a value from the 
// packed integer.
// Return value is the number of bytes in the packed integer.
uint32_t ByteBauble::readPackedInt(uint32_t packed, uint32_t &output) {
	// Check the special bits while copying the data bits.
	output = 0;
	int idx = 0; 	// Index into output integer.
	int src = 0; 	// Index into input packed integer.
	int i;
	for (i = 0; i < 4; ++i) {
		// Copy data bits of current byte.
		for (int j = 0; j < 7; ++j) {
			if ((packed >> src++) & 1UL) {
				output |= (1UL << idx++);
			}
			else {
				//output &= ~(1UL << idx++);
				idx++;
			}
		}
		
		// Check for presence of follow-up byte.
		if (!((packed >> src++) & 1UL)) {
			// Not found, we're done.
			break;
		}
	}
	
	return i + 1;
}


// --- WRITE PACKED INT ---
uint32_t ByteBauble::writePackedInt(uint32_t integer, uint32_t &output) {
	// First determine how many bytes we will be needing.
	uint32_t totalBytes = 0;
	if (integer <= 0x80) { totalBytes = 1; }
	else if (integer <= 0x4000) { totalBytes = 2; }
	else if (integer <= 0x200000) { totalBytes = 3; }
	else if (integer <= 0x10000000) { totalBytes = 4; }
	else {
		// Out of range, return error.
		return 0;
	}
	
	// Copy the source data bits to as many packed bytes as needed.
	output = 0;
	int idx = 0;	// Index into packed (output) integer.
	int src = 0; 	// Index into source integer.
	for (int i = 0; i < totalBytes; ++i) {
		//
		for (int j = 0; j < 7; ++j) {
			if ((integer >> src++) & 1UL) {
				output |= (1UL << idx++);
			}
			else {
				//output &= ~(1UL << idx++);
				idx++;
			}
		}
			
		// Check for follow-up byte and set if necessary.
		if ((i + 1) < totalBytes) {
			output |= (1UL << idx++);
		}
	}
	
	return totalBytes;
}
