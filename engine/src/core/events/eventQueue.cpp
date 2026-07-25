#include <pch.h>
#include "eventQueue.h"
#include "events.h"

namespace engine
{
	void eventQueue::queueEvent(std::shared_ptr<baseEvent> e)
	{
		co::mutex_guard lock(mQueueMU);

		mQueue.emplace(e);
	}

	std::shared_ptr<baseEvent> eventQueue::getEvent()
	{
		co::mutex_guard lock(mQueueMU);

		auto e = mQueue.front();
		mQueue.pop();

		return e;
	}

	bool eventQueue::hasEvents()
	{
		co::mutex_guard lock(mQueueMU);

		return !mQueue.empty();
	}

	std::queue<std::shared_ptr<baseEvent>> eventQueue::purgeAndGet()
	{
		co::mutex_guard lock(mQueueMU);
		
		std::queue<std::shared_ptr<baseEvent>> result{};

		std::swap(mQueue, result);

		return result;
	}
}