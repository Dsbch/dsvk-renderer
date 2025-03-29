#pragma once

#include <pch.h>
#include <typeindex>
#include "events.h"
#include "../context/context.h"

namespace engine {
	class eventDispatcher {
	private:
		static std::mutex mU;
		template <class event>
		static std::map<std::type_index, std::vector<std::function<void(const event&)>>> mEventMap;
	
		engine::context mCtx;
	public:
		eventDispatcher(engine::context ctx);
		
		template<class event>
		void addHandler(std::function<void(const event&)>);
		
		template<class event>
		void dispatch(const event&);
	};

	template <class event>
	std::map<std::type_index, std::vector<std::function<void(const event&)>>> eventDispatcher::mEventMap;

	template<class event>
	inline void eventDispatcher::addHandler(std::function<void(const event&)> handler)
	{
		std::lock_guard<std::mutex> lock(mU);

		mEventMap<event>[std::type_index(typeid(event))].push_back(handler);
	}

	template<class event>
	inline void eventDispatcher::dispatch(const event& e)
	{
		std::lock_guard<std::mutex> lock(mU);

		auto& handlers = mEventMap<event>;
		auto it = handlers.find(std::type_index(typeid(event)));
		if (it == handlers.end())
		{
			return;
		}

		for (auto& handler : it->second) {
			handler(e);
		}
	}
}