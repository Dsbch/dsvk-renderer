#include <pch.h>
#include "context.h"

engine::context::context()
	: mDispatcher(std::make_shared<engine::eventDispatcher>())
{
}

core::timer engine::context::getTimer()
{
	return mTimer;
}

std::shared_ptr<engine::eventDispatcher> engine::context::getDispatcher()
{
	return mDispatcher;
}
