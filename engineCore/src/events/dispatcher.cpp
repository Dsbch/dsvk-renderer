#include <pch.h>
#include "dispatcher.h"
#include "events.h"

namespace engineCore
{
	void eventDispatcher::addHandler(eventType t, std::function<void(std::shared_ptr<baseEvent>)> f)
	{
		std::lock_guard<std::mutex> lock(mU);

		mEventMap[t].push_back(f);
	}

	void eventDispatcher::dispatch(std::shared_ptr<baseEvent> e)
	{
		std::lock_guard<std::mutex> lock(mU);

		auto handlers = mEventMap.find(e->getEventType());
		if (handlers == mEventMap.end())
		{
			return;
		}

		for (auto& handler : handlers->second) {
			handler(e);
		}
	}

	void eventDispatcher::dipatchQueue()
	{
		while (!mQueue.empty())
		{
			dispatch(mQueue.front());
			mQueue.pop();
		}
	}

	void eventDispatcher::queueEvent(std::shared_ptr<baseEvent> e)
	{
		mQueue.emplace(e);
	}
}