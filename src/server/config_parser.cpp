/*
	config.h - INI parser class.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- Requires C++14 level features.
			- Uses the PocoProject INI parser, but alternatives can be used.
			
	2018/07/30, Maya Posch
*/


#include "config_parser.h"


// --- CONSTRUCTOR ---
IniParser::IniParser() {
	parser = new IniFileConfiguration();
}


// --- LOAD ---
// Loads the provided INI configuration file.
// Return true on success, false on failure.
bool IniParser::load(std::string &file) {
	try {
		parser->load(file);
	}
	catch (...) {
		// An exception has occurred. Return false.
		return false;
	}
	
	return true;
}

