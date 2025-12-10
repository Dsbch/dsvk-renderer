#include <pch.h>
#include "renderSystem.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	renderSystem::renderSystem(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		system(ctx),
		mRenderer(makeRenderer(ctx, wnd))
	{
	}

	error renderSystem::checkError()
	{
		return mRenderer->checkError();
	}

	error renderSystem::onUpdate(entt::registry& registry)
	{
		return {};
	}

	error renderSystem::onRender(entt::registry& registry)
	{
		return mRenderer->render();
	}

	error renderSystem::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowFrameBufferResizeEvent*>(e.get());

			return mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
		}

		return {};
	}
}