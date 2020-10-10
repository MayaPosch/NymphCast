/*
	bytebauble.h - Header for the ByteBauble library.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
			
	2019/05/31, Maya Posch
*/


#ifndef BYTEBAUBLE_H
#define BYTEBAUBLE_H


#include <climits>
#include <cstdint>

#ifdef _MSC_VER
#include <stdlib.h>
#endif


enum BBEndianness {
	BB_BE,	// Big Endian
	BB_LE	// Little Endian
};


class ByteBauble {
	BBEndianness globalEndian = BB_LE;
	BBEndianness hostEndian = BB_LE;
	
public:
	ByteBauble();
	
	void detectHostEndian();
	BBEndianness getHostEndian() { return hostEndian; }
	
	static uint32_t readPackedInt(uint32_t packed, uint32_t &output);
	static uint32_t writePackedInt(uint32_t integer, uint32_t &output);
	
	void setGlobalEndianness(BBEndianness end) { globalEndian = end; }
	
	// --- TO GLOBAL ---
	// 
	template <typename T>
	T toGlobal(T in, BBEndianness end) {
		// Convert to requested format, if different from global.
		if (end == globalEndian) {
			// Endianness matches, return input.
			return in;
		}
		
		// Perform the conversion.
		// Flip the bytes, so that the MSB and LSB are switched.
		// Compiler intrinsics in GCC/MinGW exist since ~4.3, for MSVC
		std::size_t bytesize = sizeof(in);
#if defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
		if (bytesize == 2) {
			return __builtin_bswap16(in);
		}
		else if (bytesize == 4) {
			return __builtin_bswap32(in);
		}
		else if (bytesize == 8) {
			return __builtin_bswap64(in);
		}
#elif defined(_MSC_VER)
		if (bytesize == 2) {
			return _byteswap_ushort(in);
		}
		else if (bytesize == 4) {
			return _byteswap_ulong(in);
		}
		else if (bytesize == 8) {
			return _byteswap_uint64(in);
		}
#endif

		// Fallback for other compilers.
		// TODO: implement.
		return 0;
	}
	
	// --- TO HOST ---
	// 
	template <typename T>
	T toHost(T in, BBEndianness end) {
		// 
		
		// Convert to requested format, if different from host.
		if (end == hostEndian) {
			// Endianness matches, return input.
			return in;
		}
		
		// Perform the conversion.
		// Flip the bytes, so that the MSB and LSB are switched.
		// Compiler intrinsics in GCC/MinGW exist since ~4.3, for MSVC
		std::size_t bytesize = sizeof(in);
#if defined(__GNUC__) || defined(__MINGW32__) || defined(__MINGW64__)
		if (bytesize == 2) {
			return __builtin_bswap16(in);
		}
		else if (bytesize == 4) {
			return __builtin_bswap32(in);
		}
		else if (bytesize == 8) {
			return __builtin_bswap64(in);
		}
#elif defined(_MSC_VER)
		if (bytesize == 2) {
			return _byteswap_ushort(in);
		}
		else if (bytesize == 4) {
			return _byteswap_ulong(in);
		}
		else if (bytesize == 8) {
			return _byteswap_uint64(in);
		}
#endif

		// Fallback for other compilers.
		// TODO: implement.
		return 0;
	}
};


#endif
