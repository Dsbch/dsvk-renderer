#pragma once

#include <pch.h>
#include <queue>
#include "events.h"

namespace engine 
{
	class eventQueue
	{
	public:
		void queueEvent(std::shared_ptr<baseEvent>);
		std::shared_ptr<baseEvent> getEvent();
		bool hasEvents();
		std::queue<std::shared_ptr<baseEvent>> purgeAndGet();
	private:
		co::mutex mQueueMU;
		std::queue<std::shared_ptr<baseEvent>> mQueue;
	};
}
