#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <functional>

namespace Utils
{
	class ThreadPool
	{
	public:
		typedef std::function<void(void)> work_function;

		ThreadPool();
		~ThreadPool();

		void queueWorkItem(work_function work);
		void wait();
		void wait(work_function work, int delay = 50);

	private:
		bool mRunning;
		bool mWaiting;
		std::queue<work_function> mWorkQueue;
		std::atomic<size_t> mNumWork;
		std::mutex _mutex;
		std::vector<std::thread> mThreads;

	};
}