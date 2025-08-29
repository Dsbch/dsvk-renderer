#include <pch.h>

#include "skyboxRender.h"
#include "platform/renderer/rendererFactory.h"
#include "core/scene/components.h"

namespace engine
{
	skyboxRender::skyboxRender(std::shared_ptr<context> ctx)
		:
		system(ctx)
	{
	}

	error skyboxRender::checkError()
	{
		return {};
	}

	void skyboxRender::onUpdate(entt::registry& registry)
	{
	}

	void skyboxRender::onRender(entt::registry& registry)
	{
	}

	void skyboxRender::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
	}
}