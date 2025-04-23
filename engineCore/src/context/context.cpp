#include <pch.h>
#include "context.h"

namespace engineCore
{
	context::context()
		: mDispatcher(std::make_shared<eventDispatcher>())
	{
	}

	engineCore::timer context::getTimer()
	{
		return mTimer;
	}

	std::shared_ptr<eventDispatcher> context::getDispatcher()
	{
		return mDispatcher;
	}

	engineCore::threadPool context::getThreadPool()
	{
		return mThreadPool;
	}
}