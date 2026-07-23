#pragma once

#include <pch.h>
#include <queue>
#include "events.h"

namespace engine 
{
	class eventDispatcher 
	{
	public:
		void addHandler(eventType, std::function<void(std::shared_ptr<baseEvent>)>);
		void dispatch(std::shared_ptr<baseEvent>);
		void queueEvent(std::shared_ptr<baseEvent>);
		std::shared_ptr<baseEvent> getEvent();
		bool hasEvents();
	private:
		co::mutex mDispatchMU;
		co::mutex mQueueMU;

		std::map<eventType, std::list<std::function<void(std::shared_ptr<baseEvent>)>>> mEventMap;
		std::queue<std::shared_ptr<baseEvent>> mQueue;
	};
}
