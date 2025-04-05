#pragma once

#include <pch.h>
#include <queue>
#include "events.h"

namespace engine {
	class eventDispatcher {
	private:
		std::mutex mU;
		std::map<engine::eventType, std::list<std::function<void(const engine::baseEvent&)>>> mEventMap;
		std::queue<engine::baseEvent> mQueue;
	public:
		void addHandler(engine::eventType, std::function<void(const engine::baseEvent&)>);
		void dispatch(const engine::baseEvent&);
		void dipatchQueue();
		void queueEvent(const engine::baseEvent&);
	};
}
