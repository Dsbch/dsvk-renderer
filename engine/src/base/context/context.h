#pragma once

#include <pch.h>
#include "base/config/config.h"
#include "base/timer/timer.h"
#include "core/events/eventQueue.h"
#include "core/amanager/aManager.h"

namespace engine
{
	struct context
	{
		timer appTimer;
		std::unique_ptr<aManager> mAmanager;
		// Events are consumed by game logic.
		std::unique_ptr<eventQueue> mGameEventQueue;
		// Events are consumed by application logic.
		std::unique_ptr<eventQueue> mApplicationEventQueue;
		// Events are consumed by renderThread.
		std::unique_ptr<eventQueue> mRenderEventQueue;
		cfg<mainCfg> config;

		context(cfg<mainCfg> config);
		~context();
	};
}