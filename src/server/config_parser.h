/*
	config.h - INI parser class.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
			
	2018/07/30, Maya Posch
*/


#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <memory>
#include <sstream>
#include <iostream>
#include <type_traits>

#include <Poco/Util/IniFileConfiguration.h>
#include <Poco/AutoPtr.h>

using Poco::AutoPtr;
using namespace Poco::Util;


class IniParser {
	AutoPtr<IniFileConfiguration> parser;
	
public:
	IniParser();
	
	bool load(std::string &file);
	
	// --- GET VALUE ---
	// Gets the specified key's value from the INI file.
	template<typename T>
	auto getValue(std::string key, T defaultValue) -> T {
		std::string value;
		try {
			value = parser->getRawString(key);
		}
		catch (Poco::NotFoundException &e) {
			// Key wasn't found. Use default value.
			return defaultValue;
		}
		
		// Convert the value to our output type, if possible.
		std::stringstream ss;
		if (value[0] == '0' && value[1] == 'x') {
			value.erase(0, 2);
			ss << std::hex << value; // Read as hexadecimal.
		}
		else {
			ss.str(value);
		}
		
		T retVal;
		if constexpr (std::is_same<T, std::string>::value) { retVal = ss.str(); }
		else { ss >> retVal; }
		
		return retVal;
	}
};

#endif
