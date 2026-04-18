#include <pch.h>
#include "dispatcher.h"
#include "events.h"

namespace engine
{
	void eventDispatcher::addHandler(eventType t, std::function<void(std::shared_ptr<baseEvent>)> f)
	{
		co::mutex_guard lock(mDispatchMU);

		mEventMap[t].push_back(f);
	}

	void eventDispatcher::dispatch(std::shared_ptr<baseEvent> e)
	{
		co::mutex_guard lock(mDispatchMU);

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
		co::mutex_guard lock(mQueueMU);

		mQueue.emplace(e);
	}

	std::shared_ptr<baseEvent> eventDispatcher::getEvent()
	{
		co::mutex_guard lock(mQueueMU);

		auto e = mQueue.front();
		mQueue.pop();

		return e;
	}

	bool eventDispatcher::hasEvents()
	{
		co::mutex_guard lock(mQueueMU);

		return !mQueue.empty();
	}
}