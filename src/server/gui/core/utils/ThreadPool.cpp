#include "ThreadPool.h"

#if WIN32
#include <Windows.h>
#endif

namespace Utils
{
	ThreadPool::ThreadPool() : mRunning(true), mWaiting(false), mNumWork(0)
	{
		size_t num_threads = std::thread::hardware_concurrency() - 1;

		auto doWork = [&](size_t id)
		{
#if WIN32
			auto mask = (static_cast<DWORD_PTR>(1) << id);
			SetThreadAffinityMask(GetCurrentThread(), mask);
#endif

			while (mRunning)
			{
				_mutex.lock();
				if (!mWorkQueue.empty())
				{
					auto work = mWorkQueue.front();
					mWorkQueue.pop();
					_mutex.unlock();

					try
					{
						work();
					}
					catch (...) {}

					mNumWork--;
				}
				else
				{
					_mutex.unlock();

					// Extra code : Exit finished threads
					if (mWaiting)
						return;

					std::this_thread::yield();
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
		};

		mThreads.reserve(num_threads);

		for (size_t i = 0; i < num_threads; i++)
			mThreads.push_back(std::thread(doWork, i));
	}

	ThreadPool::~ThreadPool()
	{
		mRunning = false;

		for (std::thread& t : mThreads)
			if (t.joinable())
				t.join();
	}

	void ThreadPool::queueWorkItem(work_function work)
	{
		_mutex.lock();
		mWorkQueue.push(work);
		mNumWork++;
		_mutex.unlock();
	}

	void ThreadPool::wait()
	{
		mWaiting = true;
		while (mNumWork.load() > 0)
			std::this_thread::yield();
	}

	void ThreadPool::wait(work_function work, int delay)
	{
		mWaiting = true;

		while (mNumWork.load() > 0)
		{
			work();

			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		}
	}
}