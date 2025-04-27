#include <pch.h>
#include "context.h"

namespace engine
{
	context::context()
		:
		mDispatcher(std::make_shared<eventDispatcher>()), mAmanager(std::make_shared<aManager>()), mThreadPool(std::make_shared<threadPool>())
	{};

	timer context::getTimer() const
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

	std::shared_ptr<threadPool> context::getThreadPool() const
	{
		return mThreadPool;
	}
}