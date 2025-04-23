#pragma once

#include <pch.h>
#include "config/config.h"
#include "timer/timer.h"
#include "concurrency/concurrency.h"
#include "events/dispatcher.h"

namespace engine
{
	class context
	{
	private:
		core::timer mTimer;
		core::threadPool mThreadPool;
		std::shared_ptr<eventDispatcher> mDispatcher;
	public:
		context();
		core::timer getTimer();
		std::shared_ptr<eventDispatcher> getDispatcher();
		core::threadPool getThreadPool();
	};
}