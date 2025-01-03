#pragma once

#include <pch.h>
#include "events.h"

class EventDispatcher {
private:
	std::mutex mU;
	template <typename Event>
	static std::map<std::string, std::vector<std::function<void(const Event&)>>> mEventMap;
public:
	EventDispatcher() = default;
	~EventDispatcher() = default;
	EventDispatcher(const EventDispatcher& other);
	EventDispatcher& operator=(const EventDispatcher&);
	EventDispatcher(EventDispatcher&& other) = default;
	EventDispatcher& operator=(EventDispatcher&&) = default;
	template<class Event>
	void Register(const Event& e, std::function<void(const Event&)>);
	template<class Event>
	void Dispatch(const Event&);
};

template <class Event>
std::map<std::string, std::vector<std::function<void(const Event&)>>> EventDispatcher::mEventMap;

template<class Event>
inline void EventDispatcher::Register(const Event& e, std::function<void(const Event&)> handler)
{
	std::lock_guard<std::mutex> lock(mU);

	mEventMap<Event>[e.EventIdentifier()].push_back(handler);
}

template<class Event>
inline void EventDispatcher::Dispatch(const Event& e)
{
	std::lock_guard<std::mutex> lock(mU);

	auto& handlers = mEventMap<Event>;
	auto it = handlers.find(e.EventIdentifier());
	if (it == handlers.end()) return; 

	for (auto& handler : it->second) {
		handler(e);
	}
}