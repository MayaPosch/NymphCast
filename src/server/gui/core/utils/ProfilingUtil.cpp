
#if defined(USE_PROFILING)

#include "utils/ProfilingUtil.h"

#include "math/Misc.h"
#include "Log.h"

#include <algorithm>

#if defined(_WIN32)
// because windows...
#include <Windows.h>
#define snprintf _snprintf
#else // _WIN32
#include <sys/time.h>
#endif // !_WIN32

//////////////////////////////////////////////////////////////////////////

namespace Utils
{
	namespace Profiling
	{
		std::vector<Profile*>     profiles;
		std::vector<ThreadStack*> threadStacks;
		std::recursive_mutex      mutex;
		unsigned int              counter = 0;

//////////////////////////////////////////////////////////////////////////

		static double getFrequency( void )
		{
#if defined(_WIN32)
			uint64_t qpFrequency;
			QueryPerformanceFrequency((LARGE_INTEGER*)&qpFrequency);
			return 1.0 / qpFrequency;
#else // _WIN32
			return 1.0 / 1000000.0;
#endif // _WIN32
		} // getFrequency

//////////////////////////////////////////////////////////////////////////

	static uint64_t getCounter( void )
	{
#if defined(_WIN32)
			uint64_t qpCounter;
			QueryPerformanceCounter((LARGE_INTEGER*)&qpCounter);
			return qpCounter;
#else // _WIN32
			timeval tv;
			gettimeofday(&tv, nullptr);
			return static_cast<uint64_t>(tv.tv_sec) * 1000000u + static_cast<uint64_t>(tv.tv_usec);
#endif // !_WIN32
	} // getCounter

//////////////////////////////////////////////////////////////////////////

		static double getTime(void)
		{
			return getCounter() * getFrequency();

		} // getTime

//////////////////////////////////////////////////////////////////////////

		static bool _sortProfiles(const Profile* _a, const Profile* _b)
		{
			return _a->message < _b->message;

		} // _sortProfiles

//////////////////////////////////////////////////////////////////////////

		unsigned int _generateIndex(void)
		{
			mutex.lock();
			unsigned int index = counter++;
			mutex.unlock();

			return index;

		} // _generateIndex

//////////////////////////////////////////////////////////////////////////

		void _begin(const unsigned int _index, const std::string& _message)
		{
			const std::thread::id threadId    = std::this_thread::get_id();
			ThreadStack*          threadStack = nullptr;
			Profile*              profile     = nullptr;

			mutex.lock();

			for( ThreadStack* threadStackIt : threadStacks )
				if(threadStackIt->id == threadId)
					threadStack = threadStackIt;

			if(!threadStack)
			{
				threadStack     = new ThreadStack;
				threadStack->id = threadId;

				threadStacks.push_back(threadStack);
			}

			while(profiles.size() < counter)
			{
				profile               = new Profile;
				profile->message      = "";
				profile->timeBegin    = 0.0;
				profile->timeTotal    = 0.0;
				profile->timeExternal = 0.0;
				profile->timeMin      = 999999999.0;
				profile->timeMax      = 0.0;
				profile->callCount    = 0;

				profiles.push_back(profile);
			}

			profile = profiles[_index];

			threadStack->stack.push(profile);

			profile->message   = _message;
			profile->timeBegin = getTime();

			mutex.unlock();

		} // _begin

//////////////////////////////////////////////////////////////////////////

		int _end(void)
		{
			const double          timeEnd     = getTime();
			const std::thread::id threadId    = std::this_thread::get_id();
			ThreadStack*          threadStack = nullptr;
			Profile*              profile     = nullptr;

			mutex.lock();

			for( ThreadStack* threadStackIt : threadStacks )
				if(threadStackIt->id == threadId)
					threadStack = threadStackIt;

			profile = threadStack->stack.top();
			threadStack->stack.pop();

			// timer wrapped (~24 days)
			if(timeEnd < profile->timeBegin)
			{
				mutex.unlock();
				return 0;
			}

			const double timeElapsed = timeEnd - profile->timeBegin;

			profile->timeTotal += timeElapsed;
			profile->timeMin    = (profile->timeMin < timeElapsed) ? profile->timeMin : timeElapsed;
			profile->timeMax    = (profile->timeMax > timeElapsed) ? profile->timeMax : timeElapsed;
			profile->callCount++;

			if(!threadStack->stack.empty())
				threadStack->stack.top()->timeExternal += timeElapsed;

			mutex.unlock();

			return timeElapsed;

		} // _end

//////////////////////////////////////////////////////////////////////////

		void _dump(void)
		{
			std::sort(profiles.begin(), profiles.end(), _sortProfiles);

			if(!profiles.empty())
			{
				char buffer[1024];
				int  longestMessage = 0;

				for(Profile* profile : profiles)
					longestMessage = Math::max(longestMessage, (int)profile->message.length());

				char format1[1024];
				snprintf(format1, 1024, "%%-%ds\t%%12s\t%%12s\t%%12s\t%%12s\t%%12s\t%%20s\t%%20s", longestMessage);

				snprintf(buffer, 1024, format1, "Message", "Calls", "Total Time", "Avg Time", "Min Time", "Max Time", "Internal Total Time", "Internal Avg Time");
				LOG(LogDebug) << buffer;

				char format2[1024];
				snprintf(format2, 1024, "%%-%ds\t%%12d\t%%12.6f\t%%12.6f\t%%12.6f\t%%12.6f\t%%20.6f\t%%20.6f", longestMessage);

				for(Profile* profile : profiles)
				{
					if(profile->message.length())
					{
						snprintf(buffer, 1024, format2, profile->message.c_str(), profile->callCount, profile->timeTotal, profile->timeTotal / profile->callCount, profile->timeMin, profile->timeMax, profile->timeTotal - profile->timeExternal, (profile->timeTotal - profile->timeExternal) / profile->callCount);
						LOG(LogDebug) << buffer;
					}

					delete profile;
				}

				profiles.clear();
			}

			if(!threadStacks.empty())
			{
				for(ThreadStack* threadStack : threadStacks)
					delete threadStack;

				threadStacks.clear();
			}

		} // _dump

	} // Profiling::

} // Utils::

#endif // USE_PROFILING
