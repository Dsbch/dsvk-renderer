#pragma once

#include <pch.h>
#include <thread>
#include <queue>

namespace engine {
	class cond
	{
	public:
		cond() = default;
		template<class predicate>
		void wait(predicate&& f);
		void notifyOne();
		void notifyAll();
	private:
		std::mutex mMutex;
		std::condition_variable mCv;
	};

	class threadPool;

	class threadQueue
	{
	private:
		std::mutex mMutex;
		std::queue<std::function<void()>> mQueue;
		
		cond mCond;
		
		std::unique_ptr<std::thread> mThread;
		bool mRunning;

		void run();
	public:
		threadQueue(const threadQueue&) = delete;
		threadQueue& operator=(const threadQueue&) = delete;

		threadQueue();
		~threadQueue();

		void start();

		template<class T>
		void add(T&&);
		size_t size();

		friend class threadPool;
	};

	class threadPool
	{
	private:
		static std::mutex mMutex;
		uint32_t maxThreads;
		std::list<threadQueue> threadQueue;
		
		//void rearrange();
	public:
		threadPool();
		template<class T>
		void start(T&&);
	};

	template<class predicate>
	inline void cond::wait(predicate&& f)
	{
		std::unique_lock l{ mMutex };
		mCv.wait(l, std::forward<predicate>(f));
	}

	template<class T>
	inline void threadQueue::add(T&& f)
	{
		{
			std::lock_guard<std::mutex> mu{ mMutex };
			mQueue.push(std::forward<T>(f));
		}

		mCond.notifyOne();
	}

	template<class T>
	inline void threadPool::start(T&& f)
	{
		std::lock_guard<std::mutex> l{ mMutex };

		if (maxThreads == threadQueue.size())
		{
			auto smallest = threadQueue.begin();
			for (auto crnt = threadQueue.begin(); crnt != threadQueue.end(); crnt++)
			{
				if (smallest->size() > crnt->size())
				{
					smallest = crnt;
				}
			}

			smallest->add(std::forward<T>(f));
		}
		else
		{
			for (auto crnt = threadQueue.begin(); crnt != threadQueue.end(); crnt++)
			{
				if (crnt->size() == 0)
				{
					crnt->add(std::forward<T>(f));
					return;
				}
			}

			threadQueue.emplace_back();
			threadQueue.back().start();
			threadQueue.back().add(std::forward<T>(f));
		}
	}
}
