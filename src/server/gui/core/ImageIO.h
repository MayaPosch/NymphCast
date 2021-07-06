#pragma once
#ifndef ES_CORE_IMAGE_IO
#define ES_CORE_IMAGE_IO

#include <stdlib.h>
#include <vector>

class ImageIO
{
public:
	static std::vector<unsigned char> loadFromMemoryRGBA32(const unsigned char * data, const size_t size, size_t & width, size_t & height);
	static void flipPixelsVert(unsigned char* imagePx, const size_t& width, const size_t& height);
};

#endif // ES_CORE_IMAGE_IO
