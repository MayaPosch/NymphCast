#include "utils/StringUtil.h"

#include <algorithm>
#include <stdarg.h>

//////////////////////////////////////////////////////////////////////////

namespace Utils
{
	namespace String
	{
		unsigned int chars2Unicode(const std::string& _string, size_t& _cursor)
		{
			const char&  c      = _string[_cursor];
			unsigned int result = '?';

			if((c & 0x80) == 0) // 0xxxxxxx, one byte character
			{
				// 0xxxxxxx
				result = ((_string[_cursor++]       )      );
			}
			else if((c & 0xE0) == 0xC0) // 110xxxxx, two byte character
			{
				// 110xxxxx 10xxxxxx
				result = ((_string[_cursor++] & 0x1F) <<  6) |
						 ((_string[_cursor++] & 0x3F)      );
			}
			else if((c & 0xF0) == 0xE0) // 1110xxxx, three byte character
			{
				// 1110xxxx 10xxxxxx 10xxxxxx
				result = ((_string[_cursor++] & 0x0F) << 12) |
						 ((_string[_cursor++] & 0x3F) <<  6) |
						 ((_string[_cursor++] & 0x3F)      );
			}
			else if((c & 0xF8) == 0xF0) // 11110xxx, four byte character
			{
				// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
				result = ((_string[_cursor++] & 0x07) << 18) |
						 ((_string[_cursor++] & 0x3F) << 12) |
						 ((_string[_cursor++] & 0x3F) <<  6) |
						 ((_string[_cursor++] & 0x3F)      );
			}
			else
			{
				// error, invalid unicode
				++_cursor;
			}

			return result;

		} // chars2Unicode

//////////////////////////////////////////////////////////////////////////

		std::string unicode2Chars(const unsigned int _unicode)
		{
			std::string result;

			if(_unicode < 0x80) // one byte character
			{
				result += ((_unicode      ) & 0xFF);
			}
			else if(_unicode < 0x800) // two byte character
			{
				result += ((_unicode >>  6) & 0xFF) | 0xC0;
				result += ((_unicode      ) & 0x3F) | 0x80;
			}
			else if(_unicode < 0xFFFF) // three byte character
			{
				result += ((_unicode >> 12) & 0xFF) | 0xE0;
				result += ((_unicode >>  6) & 0x3F) | 0x80;
				result += ((_unicode      ) & 0x3F) | 0x80;
			}
			else if(_unicode <= 0x1fffff) // four byte character
			{
				result += ((_unicode >> 18) & 0xFF) | 0xF0;
				result += ((_unicode >> 12) & 0x3F) | 0x80;
				result += ((_unicode >>  6) & 0x3F) | 0x80;
				result += ((_unicode      ) & 0x3F) | 0x80;
			}
			else
			{
				// error, invalid unicode
				result += '?';
			}

			return result;

		} // unicode2Chars

//////////////////////////////////////////////////////////////////////////

		size_t nextCursor(const std::string& _string, const size_t _cursor)
		{
			size_t result = _cursor;

			while(result < _string.length())
			{
				++result;

				if((_string[result] & 0xC0) != 0x80) // break if current character is not 10xxxxxx
					break;
			}

			return result;

		} // nextCursor

//////////////////////////////////////////////////////////////////////////

		size_t prevCursor(const std::string& _string, const size_t _cursor)
		{
			size_t result = _cursor;

			while(result > 0)
			{
				--result;

				if((_string[result] & 0xC0) != 0x80) // break if current character is not 10xxxxxx
					break;
			}

			return result;

		} // prevCursor

//////////////////////////////////////////////////////////////////////////

		size_t moveCursor(const std::string& _string, const size_t _cursor, const int _amount)
		{
			size_t result = _cursor;

			if(_amount > 0)
			{
				for(int i = 0; i < _amount; ++i)
					result = nextCursor(_string, result);
			}
			else if(_amount < 0)
			{
				for(int i = _amount; i < 0; ++i)
					result = prevCursor(_string, result);
			}

			return result;

		} // moveCursor

//////////////////////////////////////////////////////////////////////////

		std::string toLower(const std::string& _string)
		{
			std::string string;

			for(size_t i = 0; i < _string.length(); ++i)
				string += (char)tolower(_string[i]);

			return string;

		} // toLower

//////////////////////////////////////////////////////////////////////////

		std::string toUpper(const std::string& _string)
		{
			std::string string;

			for(size_t i = 0; i < _string.length(); ++i)
				string += (char)toupper(_string[i]);

			return string;

		} // toUpper

//////////////////////////////////////////////////////////////////////////

		std::string trim(const std::string& _string)
		{
			const size_t strBegin = _string.find_first_not_of(" \t");
			const size_t strEnd   = _string.find_last_not_of(" \t");

			if(strBegin == std::string::npos)
				return "";

			return _string.substr(strBegin, strEnd - strBegin + 1);

		} // trim

//////////////////////////////////////////////////////////////////////////

		std::string replace(const std::string& _string, const std::string& _replace, const std::string& _with)
		{
			std::string string = _string;
			size_t      pos;

			while((pos = string.find(_replace)) != std::string::npos)
				string = string.replace(pos, _replace.length(), _with.c_str(), _with.length());

			return string;

		} // replace

//////////////////////////////////////////////////////////////////////////

		bool startsWith(const std::string& _string, const std::string& _start)
		{
			return (_string.find(_start) == 0);

		} // startsWith

//////////////////////////////////////////////////////////////////////////

		bool endsWith(const std::string& _string, const std::string& _end)
		{
			return (_string.find(_end) == (_string.size() - _end.size()));

		} // endsWith

//////////////////////////////////////////////////////////////////////////

		std::string removeParenthesis(const std::string& _string)
		{
			static const char remove[4] = { '(', ')', '[', ']' };
			std::string       string = _string;
			size_t            start;
			size_t            end;
			bool              done = false;

			while(!done)
			{
				done = true;

				for(size_t i = 0; i < sizeof(remove); i += 2)
				{
					end   = string.find_first_of(remove[i + 1]);
					start = string.find_last_of( remove[i + 0], end);

					if((start != std::string::npos) && (end != std::string::npos))
					{
						string.erase(start, end - start + 1);
						done = false;
					}
				}
			}

			return trim(string);

		} // removeParenthesis

//////////////////////////////////////////////////////////////////////////

		stringVector delimitedStringToVector(const std::string& _string, const std::string& _delimiter, bool sort)
		{
			stringVector vector;
			size_t       start = 0;
			size_t       comma = _string.find(_delimiter);

			while(comma != std::string::npos)
			{
				vector.push_back(_string.substr(start, comma - start));
				start = comma + 1;
				comma = _string.find(_delimiter, start);
			}

			vector.push_back(_string.substr(start));
			if (sort)
				std::sort(vector.begin(), vector.end());

			return vector;

		} // delimitedStringToVector

//////////////////////////////////////////////////////////////////////////

		std::string vectorToDelimitedString(stringVector _vector, const std::string& _delimiter)
		{
			std::string string;

			std::sort(_vector.begin(), _vector.end());

			for(stringVector::const_iterator it = _vector.cbegin(); it != _vector.cend(); ++it)
				string += (string.length() ? _delimiter : "") + (*it);

			return string;

		} // vectorToDelimitedString

//////////////////////////////////////////////////////////////////////////

		std::string format(const char* _format, ...)
		{
			va_list	args;
			va_list copy;

			va_start(args, _format);

			va_copy(copy, args);
			const int length = vsnprintf(nullptr, 0, _format, copy);
			va_end(copy);

			char* buffer = new char[length + 1];
			va_copy(copy, args);
			vsnprintf(buffer, length + 1, _format, copy);
			va_end(copy);

			va_end(args);

			std::string out(buffer);
			delete[] buffer;

			return out;

		} // format

//////////////////////////////////////////////////////////////////////////

		std::string scramble(const std::string& _input, const std::string& _key)
		{
			std::string buffer = _input;

			for(size_t i = 0; i < _input.size(); ++i)
				buffer[i] = _input[i] ^ _key[i];

			return buffer;

		} // scramble

	} // String::

} // Utils::
