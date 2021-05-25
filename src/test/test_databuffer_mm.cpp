// test_databuffer_mm.cpp
//
// Tests:
// - Wrap-around: Alternatingly write a number of bytes in the databuffer and read them from the buffer.
// - Reset:
// - Seek:

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

int test_wraparound()
{
	std::cout << "\n*** Test wrap-around ***\n";

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

			std::cout << "*** Test wraparound: Iteration #" << i+1 << ":\n";

			print("*** ", data, "\n");
			print("*** ", bytes, "\n");

			if ( Nr != Nw || create_data(bytes) != data)
			{
				std::cout << "*** Test wraparound: Failed with size_write of '" << size_write << "'***\n";
				return EXIT_FAILURE;
			}
		}
	}

	return EXIT_SUCCESS;
}

int test_reset()
{
	std::cout << "\n*** Test reset ***\n";

	const int size_buffer = 20;

	DataBuffer::init(size_buffer);

	const int size_write = 3;
	const int size_read  = size_write;

	const int b = 1;
	const int e = size_write + 1;
	std::string data = create_range(b, e);

	// test writing and reading data:
	{
		std::vector<uint8_t> bytes(size_read, to_char(99));

		const uint32_t Nw = DataBuffer::write(data);

#if 0
		if ( Nw != DataBuffer::size() )
		{
			std::cout << "*** Test reset: expecting '" << Nw << "' items in buffer, got '" << DataBuffer::size() << "'\n";
			return EXIT_FAILURE;
		}
#endif
		const uint32_t Nr = DataBuffer::read(size_read, bytes.data());

		if ( Nr != Nw || create_data(bytes) != data)
		{
			std::cout << "*** Test reset: expected to read '" << size_read << "', got '" << Nr << "'\n";
			return EXIT_FAILURE;
		}
	}

	// test reading data after reset:
	{
		std::vector<uint8_t> bytes(size_read, to_char(99));

		const uint32_t Nw = DataBuffer::write(data);

		// clear buffer, keep allocated memory:
		std::cout << "*** Test reset: Reset buffer\n";
		DataBuffer::reset();

		const uint32_t Nr = DataBuffer::read(size_read, bytes.data());

		if ( Nr != 0 || bytes[0] != to_char(99))
		{
			std::cout << "*** Test reset: Failed with Nr is '" << Nr << "', bytes[0] is '" << to_int(bytes[0]) << "', expected Nr of 0.\n";
			print("*** ", data, "\n");
			print("*** ", bytes, "\n");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}


// --- SEEKING HANDLER ---
void seekingHandler(uint32_t session, int64_t offset) {
	if (DataBuffer::seeking()) {
		const int size_write = 3;
		const int size_read  = size_write;

		const int b = 1;
		const int e = size_write + 1;
		std::string data = create_range(b, e);
		
		const uint32_t Nw = DataBuffer::write(data);
				
		return; 
	}
}


int test_seek()
{
	std::cout << "\n*** Test seek ***\n";

	const int size_buffer = 20;

	DataBuffer::init(size_buffer);

	DataBuffer::setSeekRequestCallback(seekingHandler);

	const int size_write = 3;
	const int size_read  = size_write;

	const int b = 1;
	const int e = size_write + 1;
	std::string data = create_range(b, e);

	// test various variants of seeking:
	{
		const uint32_t Nw = DataBuffer::write(data);
		DataBuffer::setFileSize(Nw);

		// seek start:
		{
			std::cout << "\n*** Test seek: seek(DB_SEEK_START) ***\n";
			const int64_t Ns = DataBuffer::seek(DB_SEEK_START, 1);

			if ( Ns != 1 )
			{
				std::cout << "*** Test seek: seek(DB_SEEK_START, 1): '" << Ns << "', expected '" << 1 << "'\n";
				return EXIT_FAILURE;
			}
		}

		// seek current:
		std::cout << "\n*** Test seek: seek(DB_SEEK_CURRENT) ***\n";
		{
			const int64_t Ns = DataBuffer::seek(DB_SEEK_CURRENT, 1);

			if ( Ns != 2 )
			{
				std::cout << "*** Test seek: seek(DB_SEEK_CURRENT, 1): '" << Ns << "', expected '" << 2 << "'\n";
				return EXIT_FAILURE;
			}
		}

		// seek end:
		std::cout << "\n*** Test seek: seek(DB_SEEK_END) ***\n";
		{
			const int64_t  Ns = DataBuffer::seek(DB_SEEK_END, 0);

			if ( Ns != 2 )
			{
				std::cout << "*** Test seek: seek(DB_SEEK_END, 0): '" << Ns << "', expected '" << 2 << "'\n";
				return EXIT_FAILURE;
			}
		}

		// seek begin, read data:
		std::cout << "\n*** Test seek: seek(DB_SEEK_START, read data) ***\n";
		{
			std::vector<uint8_t> bytes(size_read, to_char(99));

			const int64_t  Ns = DataBuffer::seek(DB_SEEK_START, 0);
			const uint32_t Nr = DataBuffer::read(size_read, bytes.data());

			if ( Nr != Nw || create_data(bytes) != data)
			{
				std::cout << "*** Test seek: Failed to read written data:\n";
				print("*** ", data, "\n");
				print("*** ", bytes, "\n");
				return EXIT_FAILURE;
			}
		}

		// verify buffer is empty: suggestion: add public method `size()`:
		std::cout << "\n*** Test seek: seek(expecting empty buffer) ***\n";
		{
#if 0
			if ( 0 != DataBuffer::size() )
			{
				std::cout << "*** Test seek: expecting empty buffer***\n";
				return EXIT_FAILURE;
			}
#else
			std::vector<uint8_t> bytes(size_read, to_char(99));

			const uint32_t Nr = DataBuffer::read(size_read, bytes.data());

			if ( Nr != 0 || bytes[0] != to_char(99))
			{
				std::cout << "*** Test seek: expecting empty buffer***\n";
				return EXIT_FAILURE;
			}
#endif
		}
	}

	// TODO: other kind of tests:
	{
	}

	return EXIT_SUCCESS;
}

int main()
{
	return test_wraparound()
		|| test_reset()
		|| test_seek();
}

// g++ -std=c++17 -g3 -O0 -o bin/test_databuffer_mm -I../. ../server/databuffer.cpp test_databuffer_mm.cpp -pthread
