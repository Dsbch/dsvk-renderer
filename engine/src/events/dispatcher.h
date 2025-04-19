#pragma once

#include <pch.h>
#include <queue>
#include "events.h"

namespace engine {
	class eventDispatcher {
	private:
		std::mutex mU;
		std::map<engine::eventType, std::list<std::function<void(std::shared_ptr<engine::baseEvent>)>>> mEventMap;
		std::queue<std::shared_ptr<engine::baseEvent>> mQueue;
	public:
		void addHandler(engine::eventType, std::function<void(std::shared_ptr<engine::baseEvent>)>);
		void dispatch(std::shared_ptr<engine::baseEvent>);
		void dipatchQueue();
		void queueEvent(std::shared_ptr<engine::baseEvent>);
	};
}
