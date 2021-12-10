/* 
	LCDNymphCastSensor.cpp - Implementation of NymphCast sensor for LCDApi. 
	
	Revision 0
	
	Features:
			- For use with the NymphCast server.
			- API for obtaining the currently playing media's information (if any).
			
	2021/11/21, Maya Posch
*/

#include <lcdapi/sensors/LCDNymphCastSensor.h>

#include <unistd.h>

#include "ffplay/ffplay.h"

// debug
#include <iostream>


namespace lcdapi {

using namespace std;


// --- CONSTRUCTOR ---
LCDNymphCastSensor::LCDNymphCastSensor(const std::string& defaultValue) : LCDSensor(), 
								_previousValue("NO"), _defaultValue(defaultValue) {
	//
}


// --- DESTRUCTOR ---
LCDNymphCastSensor::~LCDNymphCastSensor() { }


// --- WAIT FOR CHANGE ---
void LCDNymphCastSensor::waitForChange() {
	//std::cout << "NCSensor: waitForChange()" << std::endl;
	while (_previousValue == getCurrentValue()) {
		sleep(1);
	}
	
	//std::cout << "NCSensor: waitForChange, exited loop." << std::endl;
	
	_previousValue = getCurrentValue();
}


// --- GET CURRENT VALUE ---
string LCDNymphCastSensor::getCurrentValue() {
	string value;
	if (playerStarted) {
		std::cout << "NCSensor: getCurrentValue, playerStarted." << std::endl;
		//value = file_meta.getArtist() + " - " + file_meta.getTitle();
		value = FileMetaInfo::getArtist() + " - " + FileMetaInfo::getTitle();
	} 
	else {
		//std::cout << "NCSensor: getCurrentValue, default." << std::endl;
		value = _defaultValue;
	}

	return value;
}

} // end of lcdapi namespace
