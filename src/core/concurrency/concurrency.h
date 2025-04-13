#pragma once

#include <pch.h>
#include <queue>

namespace core {
	class threadQueue
	{
	private:
		std::unique_ptr<std::thread> mThread;
		std::queue<std::function<void()>> mQueue;
		std::condition_variable mCv;
		std::mutex mMutex;
		bool mRunning;

		void run();
	public:
		threadQueue(const threadQueue&) = delete;
		threadQueue& operator=(const threadQueue&) = delete;

		threadQueue();
		~threadQueue();

		void start();
		void add(std::function<void()>);
		uint32_t size() const;
	};

	class threadPool {
	private:
		static uint32_t maxThreads;
		static std::list<core::threadQueue> threadQueue;
	public:
		static void start(std::function<void()>);
	};
}
