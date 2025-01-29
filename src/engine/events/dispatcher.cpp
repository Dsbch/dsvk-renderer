#include <pch.h>
#include "dispatcher.h"
#include "events.h"

Engine::EventDispatcher::EventDispatcher(const EventDispatcher& other) : mU()
{
}

Engine::EventDispatcher& Engine::EventDispatcher::operator=(const EventDispatcher& other)
{
	if (this != &other)
	{
		auto toDestroy(other);
	}

	return *this;
}
