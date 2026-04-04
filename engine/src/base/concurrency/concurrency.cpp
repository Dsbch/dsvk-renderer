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
			std::vector<std::list<threadQueue>::iterator> availableThreads;

			for (auto e = mThreadList.begin(); e != mThreadList.end(); e++)
			{
				if (auto found = sizeMap.find(e->getThreadID()); found != sizeMap.end())
				{
					if (found->second == 0 || found->second > e->size())
						availableThreads.push_back(e);
				}
			}

			if (!availableThreads.empty())
			{
				for (auto& e : mThreadList)
				{
					if (auto found = sizeMap.find(e.getThreadID()); found != sizeMap.end() && found->second <= e.size())
					{
						e.splice(tasksToReschedule);
					}
				}

				size_t perThread = tasksToReschedule.size() / availableThreads.size();
				size_t threadIndex = 0;
				size_t count = 0;

				for (const auto& task : tasksToReschedule)
				{
					availableThreads[threadIndex]->add(task);
					count++;

					if (count >= perThread && threadIndex + 1 < availableThreads.size())
					{
						threadIndex++;
						count = 0;
					}
				}
			}

			for (auto& e : mThreadList)
				sizeMap[e.getThreadID()] = e.size();

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}

	void threadPool::init(const std::string& name, uint32_t maxThreads)
	{
		mName = name;

		if (maxThreads == 0)
			mMaxThreads = std::thread::hardware_concurrency() / 2;
		else
			mMaxThreads = maxThreads;

		mWatchThread = nullptr;
		mRunning = true;

		for (uint32_t i = 0; i < mMaxThreads; i++)
		{
			mThreadList.emplace_back();
			mThreadList.back().start();
		}

		mWatchThread = std::make_unique<std::thread>(&threadPool::watchPool, this);

		LOGDEBUG("thread pool {} started: threads launched: {}", mName, mMaxThreads);
	}

	void threadPool::destroy()
	{
		LOGDEBUG("destroying thread pool {}", mName);

		mRunning = false;

		if (mWatchThread.get() && mWatchThread->joinable())
			mWatchThread->join();

		LOGDEBUG("threadPool {} watch thread was joined", mName);

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
				if (mQueue.empty())
				{
					mMutex.unlock();
					break;
				}

				auto func = mQueue.front();
				mQueue.pop_front();
				mMutex.unlock();

				try
				{
					func();
				}
				catch (...)
				{
					LOGERROR("exception was caught in threadQueue::run");
				}
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
		{
			LOGDEBUG("thread {} is joining", getRedeableThreadID());
			mThread->join();
		}
	}

	void threadQueue::start()
	{
		mThread = std::make_unique<std::thread>(&threadQueue::run, this);
		LOGDEBUG("threadQueue {} started", getRedeableThreadID());
	}

	std::thread::id threadQueue::getThreadID() const
	{
		return mThread->get_id();
	}

	size_t threadQueue::getRedeableThreadID() const
	{
		static std::hash<std::thread::id> hasher{};

		return hasher(mThread->get_id());
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