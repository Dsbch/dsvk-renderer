#pragma once

#include <pch.h>
#include "base/config/config.h"
#include "base/timer/timer.h"
#include "core/events/dispatcher.h"
#include "core/amanager/aManager.h"

namespace engine
{
	struct context
	{
		bool isRunning = true;
		timer appTimer;
		std::unique_ptr<aManager> mAmanager;
		std::unique_ptr<eventDispatcher> mEventDispatcher;
		cfg<mainCfg> config;

		context(cfg<mainCfg> config);
		~context();
	};
}