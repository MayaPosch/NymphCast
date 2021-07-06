#pragma once
#ifndef ES_CORE_UTILS_PROFILING_UTIL_H
#define ES_CORE_UTILS_PROFILING_UTIL_H

#if defined(USE_PROFILING)

#include <mutex>
#include <stack>
#include <string>
#include <thread>
#include <vector>

namespace Utils
{
	namespace Profiling
	{
		struct Profile
		{
			std::string  message;
			double       timeBegin;
			double       timeTotal;
			double       timeExternal;
			double       timeMin;
			double       timeMax;
			unsigned int callCount;

		}; // Profile

		struct ThreadStack
		{
			std::stack<Profile*> stack;
			std::thread::id      id;

		}; // ThreadStack

		extern std::vector<Profile*>     profiles;
		extern std::vector<ThreadStack*> threadStacks;
		extern std::recursive_mutex      mutex;
		extern unsigned int              counter;

//////////////////////////////////////////////////////////////////////////

		unsigned int _generateIndex(void);
		void         _begin        (const unsigned int _index, const std::string& _message);
		int          _end          (void);
		void         _dump         (void);

//////////////////////////////////////////////////////////////////////////

		class Scope
		{
		public:

			 Scope(const unsigned int _index, const std::string& _message) { _begin(_index, _message); }
			~Scope(void)                                                   { _end(); }

		}; // Scope

	}; // Profiling::

} // Utils::

#define _profilingUnique(_name, _line) _name ## _line
#define _profilingUniqueIndex(_line)   _profilingUnique(uniqueIndex, _line)
#define _profilingUniqueScope(_line)   _profilingUnique(uniqueScope, _line)
#define __profilingUniqueIndex         _profilingUniqueIndex(__LINE__)
#define __profilingUniqueScope         _profilingUniqueScope(__LINE__)

#define ProfileBegin(_message) static const unsigned int __profilingUniqueIndex = Utils::Profiling::_generateIndex(); Utils::Profiling::_begin(__profilingUniqueIndex, _message)
#define ProfileEnd()           Utils::Profiling::_end()
#define ProfileScope(_message) static const unsigned int __profilingUniqueIndex = Utils::Profiling::_generateIndex(); const Utils::Profiling::Scope __profilingUniqueScope(__profilingUniqueIndex, _message)
#define ProfileDump()          Utils::Profiling::_dump()

#else // USE_PROFILING

#define ProfileBegin(_message)
#define ProfileEnd()
#define ProfileScope(_message)
#define ProfileDump()

#endif // !USE_PROFILING

#if !defined(__PRETTY_FUNCTION__)
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif // !__PRETTY_FUNCTION__

#endif // ES_CORE_UTILS_TIME_UTIL_H
