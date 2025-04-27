#pragma once

#include <pch.h>
#include "core/events/dispatcher.h"
#include "core/amanager/aManager.h"
#include "base/config/config.h"
#include "base/concurrency/concurrency.h"
#include "base/timer/timer.h"

namespace engine
{
	class context
	{
	private:
		timer mTimer;
		std::shared_ptr<threadPool> mThreadPool;
		std::shared_ptr<aManager> mAmanager;
		std::shared_ptr<eventDispatcher> mDispatcher;
	public:
		context();

		timer getTimer() const;
		std::shared_ptr<eventDispatcher> getDispatcher();
		std::shared_ptr<aManager> getAManager();
		std::shared_ptr<threadPool> getThreadPool() const;
	};
}