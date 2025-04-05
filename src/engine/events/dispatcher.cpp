#include <pch.h>
#include "dispatcher.h"
#include "events.h"

void engine::eventDispatcher::addHandler(engine::eventType t, std::function<void(const engine::baseEvent&)> f)
{
	std::lock_guard<std::mutex> lock(mU);

	mEventMap[t].push_back(f);
}

void engine::eventDispatcher::dispatch(const engine::baseEvent& e)
{
	std::lock_guard<std::mutex> lock(mU);

	auto handlers = mEventMap.find(e.getEventType());
	if (handlers == mEventMap.end())
	{
		return;
	}

	for (auto& handler : handlers->second) {
		handler(e);
	}
}

void engine::eventDispatcher::dipatchQueue()
{
	while (!mQueue.empty())
	{
		dispatch(mQueue.front());
		mQueue.pop();
	}
}

void engine::eventDispatcher::queueEvent(const engine::baseEvent& e)
{
	mQueue.push(e);
}
