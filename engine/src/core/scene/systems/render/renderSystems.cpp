#include <pch.h>
#include "renderSystems.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	renderSystems::renderSystems(std::shared_ptr<context> ctx, std::shared_ptr<window> wnd)
		:
		system(ctx),
		mRenderer(makeRenderer(ctx, wnd))
	{
	}

	error renderSystems::checkError()
	{
		return mRenderer->checkError();
	}

	error renderSystems::onUpdate(entt::registry& registry)
	{
		return {};
	}

	error renderSystems::onRender(entt::registry& registry)
	{
		return mRenderer->render();
	}

	error renderSystems::onEvent(entt::registry& registry, std::shared_ptr<baseEvent> e)
	{
		if (e->getEventType() == eventType::windowResize)
		{
			auto resizeEvent = static_cast<windowFrameBufferResizeEvent*>(e.get());

			return mRenderer->changeViewPort(resizeEvent->getWidth(), resizeEvent->getHeight());
		}

		return {};
	}
}