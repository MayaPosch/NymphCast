#include "ImageIO.h"

#include "Log.h"
#include <FreeImage.h>
#include <string.h>

std::vector<unsigned char> ImageIO::loadFromMemoryRGBA32(const unsigned char * data, const size_t size, size_t & width, size_t & height)
{
	std::vector<unsigned char> rawData;
	width = 0;
	height = 0;
	FIMEMORY * fiMemory = FreeImage_OpenMemory((BYTE *)data, (DWORD)size);
	if (fiMemory != nullptr) {
		//detect the filetype from data
		FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeFromMemory(fiMemory);
		if (format != FIF_UNKNOWN && FreeImage_FIFSupportsReading(format))
		{
			//file type is supported. load image
			FIBITMAP * fiBitmap = FreeImage_LoadFromMemory(format, fiMemory);
			if (fiBitmap != nullptr)
			{
				//loaded. convert to 32bit if necessary
				if (FreeImage_GetBPP(fiBitmap) != 32)
				{
					FIBITMAP * fiConverted = FreeImage_ConvertTo32Bits(fiBitmap);
					if (fiConverted != nullptr)
					{
						//free original bitmap data
						FreeImage_Unload(fiBitmap);
						fiBitmap = fiConverted;
					}
				}
				if (fiBitmap != nullptr)
				{
					width = FreeImage_GetWidth(fiBitmap);
					height = FreeImage_GetHeight(fiBitmap);
					//loop through scanlines and add all pixel data to the return vector
					//this is necessary, because width*height*bpp might not be == pitch
					unsigned char * tempData = new unsigned char[width * height * 4];
					for (size_t i = 0; i < height; i++)
					{
						const BYTE * scanLine = FreeImage_GetScanLine(fiBitmap, (int)i);
						memcpy(tempData + (i * width * 4), scanLine, width * 4);
					}
					//convert from BGRA to RGBA
					for(size_t i = 0; i < width*height; i++)
					{
						RGBQUAD bgra = ((RGBQUAD *)tempData)[i];
						RGBQUAD rgba;
						rgba.rgbBlue = bgra.rgbRed;
						rgba.rgbGreen = bgra.rgbGreen;
						rgba.rgbRed = bgra.rgbBlue;
						rgba.rgbReserved = bgra.rgbReserved;
						((RGBQUAD *)tempData)[i] = rgba;
					}
					rawData = std::vector<unsigned char>(tempData, tempData + width * height * 4);
					//free bitmap data
					FreeImage_Unload(fiBitmap);
					delete[] tempData;
				}
			}
			else
			{
				LOG(LogError) << "Error - Failed to load image from memory!";
			}
		}
		else
		{
			LOG(LogError) << "Error - File type " << (format == FIF_UNKNOWN ? "unknown" : "unsupported") << "!";
		}
		//free FIMEMORY again
		FreeImage_CloseMemory(fiMemory);
	}
	return rawData;
}

void ImageIO::flipPixelsVert(unsigned char* imagePx, const size_t& width, const size_t& height)
{
	unsigned int temp;
	unsigned int* arr = (unsigned int*)imagePx;
	for(size_t y = 0; y < height / 2; y++)
	{
		for(size_t x = 0; x < width; x++)
		{
			temp = arr[x + (y * width)];
			arr[x + (y * width)] = arr[x + (height * width) - ((y + 1) * width)];
			arr[x + (height * width) - ((y + 1) * width)] = temp;
		}
	}
}
