#pragma once

#include <pch.h>
#include <queue>
#include "events.h"

namespace engine 
{
	class eventDispatcher 
	{
	private:
		std::mutex mU;
		std::map<eventType, std::list<std::function<void(std::shared_ptr<baseEvent>)>>> mEventMap;
		std::queue<std::shared_ptr<baseEvent>> mQueue;
	public:
		void addHandler(eventType, std::function<void(std::shared_ptr<baseEvent>)>);
		void dispatch(std::shared_ptr<baseEvent>);
		void queueEvent(std::shared_ptr<baseEvent>);
		std::shared_ptr<baseEvent> getEvent();
		bool hasEvents() const;
	};
}
