#include <pch.h>
#include "concurrency.h"

namespace engine
{
	void cond::notifyOne()
	{
		mCv.notify_one();
	}

	void cond::notifyAll()
	{
		mCv.notify_all();
	}

	std::mutex threadPool::mMutex;

	threadPool::threadPool() : maxThreads(std::thread::hardware_concurrency() - 1)
	{
	}

	void threadQueue::run()
	{
		while (mRunning)
		{
			mCond.wait([&] { return (!mQueue.empty() && mRunning) || !mRunning; });

			while (!mQueue.empty() && mRunning)
			{
				mMutex.lock();
				auto func = mQueue.front();
				mQueue.pop();
				mMutex.unlock();

				func();
			}
		}
	}

	threadQueue::threadQueue() : mRunning(true), mThread(std::make_unique<std::thread>())
	{
	}

	threadQueue::~threadQueue()
	{
		mRunning = false;
		mCond.notifyOne();

		if (mThread->joinable())
			mThread->join();
	}

	void threadQueue::start()
	{
		mThread = std::make_unique<std::thread>(&threadQueue::run, this);
	}

	size_t threadQueue::size()
	{
		std::lock_guard<std::mutex> mu{ mMutex };
		return mQueue.size();
	}
}