#include <pch.h>
#include "context.h"

namespace engine
{
	context::context() : mDispatcher(std::make_shared<eventDispatcher>()), mAmanager(std::make_shared<aManager>()) {};

	engine::timer context::getTimer()
	{
		return mTimer;
	}

	std::shared_ptr<eventDispatcher> context::getDispatcher()
	{
		return mDispatcher;
	}

	std::shared_ptr<aManager> context::getAManager()
	{
		return mAmanager;
	}

	engine::threadPool context::getThreadPool()
	{
		return mThreadPool;
	}
}