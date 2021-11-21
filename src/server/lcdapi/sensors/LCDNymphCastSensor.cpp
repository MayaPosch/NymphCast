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
	while (_previousValue == getCurrentValue()) {
		sleep(1);
	}
	
	_previousValue = getCurrentValue();
}


// --- GET CURRENT VALUE ---
string LCDNymphCastSensor::getCurrentValue() {
	string value;
	if (playerStarted) {
		value = file_meta.getArtist() + " - " + file_meta.getTitle();
	} 
	else {
		value = _defaultValue;
	}

	return value;
}

} // end of lcdapi namespace
