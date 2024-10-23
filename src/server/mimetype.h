/*
	mimetype.h - Maps file extensions to file mime types.
	
	Notes:
			- Only covers video, image and audio mime types.
	
	2020/12/09, Maya Posch
*/


#ifndef MIMETYPE_H
#define MIMETYPE_H


#include <cstdint>
#include <string>
#include <map>


class MimeType {
	static std::map<std::string, std::string> mimes;
	
public:
	static std::string getMimeType(std::string extension);
	static bool hasExtension(std::string extension, uint8_t &type);
};

#endif
