#include <pch.h>
#include "dispatcher.h"
#include "events.h"

engine::eventDispatcher::eventDispatcher(const eventDispatcher& other) : mU()
{
}

engine::eventDispatcher& engine::eventDispatcher::operator=(const eventDispatcher& other)
{
	if (this != &other)
	{
		auto toDestroy(other);
	}

	return *this;
}
