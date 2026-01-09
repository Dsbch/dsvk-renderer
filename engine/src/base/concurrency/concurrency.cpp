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

	void threadPool::watchPool()
	{
		static std::map<std::thread::id, size_t> sizeMap;

		while (mRunning)
		{
			std::list<std::function<void()>> tasksToReschedule;
			std::vector<std::list<threadQueue>::iterator> goodThreads;

			for (auto e = mThreadList.begin(); e != mThreadList.end(); e++)
			{
				if (auto found = sizeMap.find(e->getThreadID()); found != sizeMap.end())
				{
					if (found->second == 0 || found->second > e->size())
						goodThreads.push_back(e);
				}
			}

			if (!goodThreads.empty())
			{
				for (auto& e : mThreadList)
				{
					if (auto found = sizeMap.find(e.getThreadID()); found != sizeMap.end() && found->second <= e.size())
					{
						e.splice(tasksToReschedule);
					}
				}

				size_t perThread = tasksToReschedule.size() / goodThreads.size();
				size_t threadIndex = 0;
				size_t count = 0;

				for (const auto& task : tasksToReschedule)
				{
					goodThreads[threadIndex]->add(task);
					count++;

					if (count >= perThread && threadIndex + 1 < goodThreads.size())
					{
						threadIndex++;
						count = 0;
					}
				}
			}

			for (auto& e : mThreadList)
			{
				sizeMap[e.getThreadID()] = e.size();
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	void threadPool::init()
	{
		mMaxThreads = std::thread::hardware_concurrency() / 2;
		mWatchThread = nullptr;
		mRunning = true;

		for (uint32_t i = 0; i < mMaxThreads; i++)
		{
			mThreadList.emplace_back();
			mThreadList.back().start();
		}

		mWatchThread = std::make_unique<std::thread>(&threadPool::watchPool, this);
	}

	void threadPool::destroy()
	{
		mRunning = false;

		if (mWatchThread.get() && mWatchThread->joinable())
			mWatchThread->join();

		mThreadList.clear();
	}

	bool threadPool::isThreadPoolRunning()
	{
		return mRunning;
	}

	void threadQueue::run()
	{
		while (mRunning)
		{
			mCond.wait([&] { return (!mQueue.empty() && mRunning) || !mRunning; });

			while (!mQueue.empty())
			{
				mMutex.lock();
				auto func = mQueue.front();
				mQueue.pop_front();
				mMutex.unlock();

				func();
			}
		}
	}

	threadQueue::threadQueue() : mRunning(true), mThread(nullptr)
	{
	}

	threadQueue::~threadQueue()
	{
		mRunning = false;
		mCond.notifyOne();

		if (mThread.get() && mThread->joinable())
			mThread->join();
	}

	void threadQueue::start()
	{
		mThread = std::make_unique<std::thread>(&threadQueue::run, this);
	}

	std::thread::id threadQueue::getThreadID() const
	{
		return mThread->get_id();
	}

	void threadQueue::splice(std::list<std::function<void()>>& out)
	{
		std::lock_guard<std::mutex> mu{ mMutex };
		out.splice(out.end(), mQueue);
	}

	size_t threadQueue::size()
	{
		std::lock_guard<std::mutex> mu{ mMutex };
		return mQueue.size();
	}
}