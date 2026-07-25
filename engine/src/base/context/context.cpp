#include <pch.h>
#include "context.h"

namespace engine
{
	context::context(cfg<mainCfg> config)
		:
		mGameEventQueue(std::make_unique<eventQueue>()), mRenderEventQueue(std::make_unique<eventQueue>()), mApplicationEventQueue(std::make_unique<eventQueue>()), mAmanager(std::make_unique<aManager>()), config(config)
	{
	}

	context::~context()
	{
	}
}