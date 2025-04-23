#include <pch.h>
#include "context.h"

namespace engine
{
	context::context()
		: mDispatcher(std::make_shared<eventDispatcher>())
	{
	}

	core::timer context::getTimer()
	{
		return mTimer;
	}

	std::shared_ptr<eventDispatcher> context::getDispatcher()
	{
		return mDispatcher;
	}

	core::threadPool context::getThreadPool()
	{
		return mThreadPool;
	}
}