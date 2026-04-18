#include <pch.h>
#include "context.h"

namespace engine
{
	context::context(cfg<mainCfg> config)
		:
		mEventDispatcher(std::make_unique<eventDispatcher>()), mAmanager(std::make_unique<aManager>()), config(config)
	{
	}

	context::~context()
	{
	}
}