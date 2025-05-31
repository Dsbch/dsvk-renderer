#pragma once

#include <pch.h>
#include "base/config/config.h"
#include "base/concurrency/concurrency.h"
#include "base/timer/timer.h"
#include "core/events/dispatcher.h"
#include "core/amanager/aManager.h"

namespace engine
{
	struct context
	{
		timer timer;
		std::unique_ptr<threadPool> mThreadPool;
		std::unique_ptr<aManager> mAmanager;
		std::unique_ptr<eventDispatcher> mEventDispatcher;
		cfg<main> config;
		
		context(cfg<main> config);
	};
}