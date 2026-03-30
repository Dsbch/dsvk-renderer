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

		class threadQueue
		{
		private:
			std::mutex mMutex;
			std::list<std::function<void()>> mQueue;
			std::unique_ptr<std::thread> mThread;
			cond mCond;
			std::atomic<bool> mRunning;

			void run();
		public:
			threadQueue(const threadQueue&) = delete;
			threadQueue& operator=(const threadQueue&) = delete;
			threadQueue();
			~threadQueue();

			void start();
			std::thread::id getThreadID() const;
			size_t getRedeableThreadID() const;
			void splice(std::list<std::function<void()>>& out);

			template<class T>
			void add(T&&);
			size_t size();
		};

		struct threadPool
		{
		private:
			std::mutex mMutex;
			std::list<threadQueue> mThreadList;
			std::unique_ptr<std::thread> mWatchThread;
			std::atomic<bool> mRunning;
			void watchPool();

			uint32_t mMaxThreads;
		public:
			void init();
			void destroy();

			bool isThreadPoolRunning();

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
				mQueue.push_back(std::forward<T>(f));
			}

			mCond.notifyOne();
		}

		template<class T>
		inline void threadPool::start(T&& f)
		{
			std::lock_guard<std::mutex> l{ mMutex };

			auto smallest = mThreadList.begin();
			for (auto crnt = mThreadList.begin(); crnt != mThreadList.end(); crnt++)
			{
				if (smallest->size() > crnt->size())
				{
					smallest = crnt;
				}
			}

			smallest->add(std::forward<T>(f));
		}
	}
