#include <pch.h>
#include "dispatcher.h"
#include "events.h"

namespace engine
{
	void eventDispatcher::addHandler(eventType t, std::function<void(std::shared_ptr<baseEvent>)> f)
	{
		std::lock_guard<std::mutex> lock(mDispatchMU);

		mEventMap[t].push_back(f);
	}

	void eventDispatcher::dispatch(std::shared_ptr<baseEvent> e)
	{
		std::lock_guard<std::mutex> lock(mDispatchMU);

		auto handlers = mEventMap.find(e->getEventType());
		if (handlers == mEventMap.end())
		{
			return;
		}

		for (auto& handler : handlers->second) 
		{
			handler(e);
		}
	}

	void eventDispatcher::queueEvent(std::shared_ptr<baseEvent> e)
	{
		std::lock_guard<std::mutex> lock(mQueueMU);

		mQueue.emplace(e);
	}

	std::shared_ptr<baseEvent> eventDispatcher::getEvent()
	{
		std::lock_guard<std::mutex> lock(mQueueMU);

		auto e = mQueue.front();
		mQueue.pop();

		return e;
	}

	bool eventDispatcher::hasEvents()
	{
		std::lock_guard<std::mutex> lock(mQueueMU);

		return !mQueue.empty();
	}
}