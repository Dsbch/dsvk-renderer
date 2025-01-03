#include <pch.h>
#include "dispatcher.h"
#include "events.h"

EventDispatcher::EventDispatcher(const EventDispatcher& other) : mU()
{
}

EventDispatcher& EventDispatcher::operator=(const EventDispatcher& other)
{
	if (this != &other)
	{
		auto toDestroy(other);
	}

	return *this;
}
