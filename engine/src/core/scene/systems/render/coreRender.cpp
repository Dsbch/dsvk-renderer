#include <pch.h>

#include "coreRender.h"

namespace engine
{
	coreRender::coreRender(std::shared_ptr<context> ctx)
		:
		system(ctx)
	{
	}

	error coreRender::checkError()
	{
		return {};
	}

	void coreRender::onUpdate(entt::registry& registry)
	{
	}

	void coreRender::onRender(entt::registry& registry)
	{
	}

	void coreRender::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
	}
}