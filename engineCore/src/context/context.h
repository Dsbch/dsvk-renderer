#pragma once

#include <pch.h>
#include "config/config.h"
#include "timer/timer.h"
#include "concurrency/concurrency.h"
#include "events/dispatcher.h"

namespace engineCore
{
	class context
	{
	private:
		engineCore::timer mTimer;
		engineCore::threadPool mThreadPool;
		std::shared_ptr<eventDispatcher> mDispatcher;
	public:
		context();
		engineCore::timer getTimer();
		std::shared_ptr<eventDispatcher> getDispatcher();
		engineCore::threadPool getThreadPool();
	};
}