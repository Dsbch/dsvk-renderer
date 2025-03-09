#pragma once

#include <pch.h>
#include "events.h"

namespace engine {
	class eventDispatcher {
	private:
		static std::mutex mU;
		template <typename event>
		static std::map<std::string, std::vector<std::function<void(const event&)>>> mEventMap;
	public:
		eventDispatcher() = delete;
		
		template<class event>
		static void addHandler(const event& e, std::function<void(const event&)>);
		
		template<class event>
		static void dispatch(const event&);
	};

	template <class event>
	std::map<std::string, std::vector<std::function<void(const event&)>>> eventDispatcher::mEventMap;

	template<class event>
	inline void eventDispatcher::addHandler(const event& e, std::function<void(const event&)> handler)
	{
		std::lock_guard<std::mutex> lock(mU);

		mEventMap<event>[e.eventIdentifier()].push_back(handler);
	}

	template<class event>
	inline void eventDispatcher::dispatch(const event& e)
	{
		std::lock_guard<std::mutex> lock(mU);

		auto& handlers = mEventMap<event>;
		auto it = handlers.find(e.eventIdentifier());
		if (it == handlers.end()) return;

		for (auto& handler : it->second) {
			handler(e);
		}
	}
}