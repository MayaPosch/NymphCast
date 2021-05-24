// test_databuffer_mm.cpp
//
// Alternatingly write a number of bytes in the databuffer and read them from the buffer.
// Specifically take a look at wrap-around.

#include "../server/databuffer.h"
#include <iostream>
#include <vector>

template< typename T >
char to_char( T v )
{
	return static_cast<char>( v );
}

template< typename T >
int to_int( T v )
{
	return static_cast<int>( v );
}

std::string create_range(int begin, int end)
{
	std::string data;

	for ( int i = begin; i < end; ++i )
		data.append(1, to_char(i));

	return data;
}

#if 0
template< int N >
std::string create_data( uint8_t (&bytes)[N])
{
	std::string data;

	for ( int i = 0; i < N; ++i )
		data.append(1, bytes[i]);

	return data;
}
#endif

template< typename T >
std::string create_data( std::vector<T> const & bytes )
{
	std::string data;

	for ( int i = 0; i < bytes.size(); ++i )
		data.append(1, bytes[i]);

	return data;
}

void print( std::string const & prefix, std::string const & data, std::string const & postfix)
{
	std::cout << prefix << "[";
	for (int i = 0; i < data.length(); ++i)
		std::cout << to_int(data[i]) << " ";
	std::cout << "]" << postfix;
}

template< typename T >
void print( std::string const & prefix, std::vector<T> const & bytes, std::string const & postfix )
{
	std::cout << prefix << "[";
	for (int i = 0; i < bytes.size(); ++i)
		std::cout << to_int(bytes[i]) << " ";
	std::cout << "]" << postfix;
}

int main()
{
	const int size_buffer = 20;

	DataBuffer::init(size_buffer);

	const int Nrepeat_size     = 30;
	const int size_write_begin = 3;
	const int size_write_end   = 3;

	for ( int size_write = size_write_begin; size_write <= size_write_end; ++size_write )
	{
		const int size_read = size_write;

		// DataBuffer::cleanup();
		// DataBuffer::init(size_buffer);

		const int b = 1;
		const int e = size_write + 1;
		std::string data = create_range(b, e);

		for ( int i = 0; i < Nrepeat_size; ++i )
		{
			std::vector<uint8_t> bytes(size_read, to_char(99));

			const uint32_t Nw = DataBuffer::write(data);
			const uint32_t Nr = DataBuffer::read(size_read, bytes.data());

			std::cout << "*** Iteration #" << i+1 << ":\n";

			print("*** ", data, "\n");
			print("*** ", bytes, "\n");

			if ( Nr != Nw || create_data(bytes) != data)
			{
				std::cout << "*** Failed with size_write of '" << size_write << "': ***\n";
				return EXIT_FAILURE;
			}
		}
	}
}

// g++ -std=c++17 -g3 -O0 -o bin/test_databuffer_mm -I../. ../server/databuffer.cpp test_databuffer_mm.cpp -pthread
