#include "concurrency.h"

namespace core
{
	uint32_t threadPool::maxThreads = std::thread::hardware_concurrency();
	std::list<threadQueue> threadPool::threadQueue;

	void threadPool::start(std::function<void()> f)
	{
		if (maxThreads == threadQueue.size())
		{
			auto smallest = threadQueue.begin();
			for (auto crnt = threadQueue.begin(); crnt != threadQueue.end(); crnt++)
			{
				if (crnt->size() > smallest->size())
				{
					smallest = crnt;
				}
			}

			smallest->add(f);
		}
		else
		{
			for (auto crnt = threadQueue.begin(); crnt != threadQueue.end(); crnt++)
			{
				if (crnt->size() == 0)
				{
					crnt->add(f);
					return;
				}
			}

			threadQueue.emplace_back();
			threadQueue.back().start();
			threadQueue.back().add(f);
		}
	}

	void threadQueue::run()
	{
		while (mRunning)
		{
			std::unique_lock l{ mMutex };
			mCv.wait(l, [&] { return (!mQueue.empty() && mRunning) || !mRunning; });

			if (!mQueue.empty() && mRunning)
			{
				auto func = mQueue.front();
				mQueue.pop();

				l.unlock();

				func();
			}
		}
	}

	threadQueue::threadQueue() : mRunning(true), mThread(std::make_unique<std::thread>())
	{
	}

	threadQueue::~threadQueue()
	{
		{
			std::lock_guard<std::mutex> mu{ mMutex };
			mRunning = false;
		}

		mCv.notify_one();

		if (mThread->joinable())
			mThread->join();
	}

	void threadQueue::start()
	{
		mThread = std::make_unique<std::thread>(&threadQueue::run, this);
	}

	void threadQueue::add(std::function<void()> f)
	{
		{
			std::lock_guard<std::mutex> mu{ mMutex };
			mQueue.push(f);
		}

		mCv.notify_one();
	}

	size_t threadQueue::size() const
	{
		return mQueue.size();
	}
}