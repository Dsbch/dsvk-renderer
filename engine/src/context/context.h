#pragma once

#include <pch.h>
#include "config/config.h"
#include "events/dispatcher.h"
#include "concurrency/concurrency.h"
#include "timer/timer.h"
#include "amanager/amanager.h"

namespace engine
{
	class context
	{
	private:
		timer mTimer;
		threadPool mThreadPool;
		std::shared_ptr<aManager> mAmanager;
		std::shared_ptr<eventDispatcher> mDispatcher;
	public:
		context();
		
		timer getTimer();
		std::shared_ptr<eventDispatcher> getDispatcher();
		std::shared_ptr<aManager> getAManager();
		threadPool getThreadPool();
	};
}